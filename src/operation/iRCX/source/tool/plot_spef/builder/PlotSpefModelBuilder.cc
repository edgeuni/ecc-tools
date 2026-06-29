// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "builder/PlotSpefModelBuilder.hh"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "StringUtils.hh"
#include "Types.hh"
#include "log/Log.hh"
#include "resolver/PlotSpefCapResolver.hh"

namespace ircx::plot_spef {
namespace {

enum class ScanSection
{
  kNone,
  kConn,
  kCap,
  kRes
};

struct ScanGeometry
{
  double length = 0.0;
  double width = 0.0;
  int layer = 0;
  int direction = -1;
  int llx = 0;
  int lly = 0;
  int urx = 0;
  int ury = 0;
  bool has_length = false;
  bool has_width = false;
  bool has_layer = false;
  bool has_direction = false;
  bool has_box = false;
};

auto hasCoord(const spef::Coord& coord) -> bool
{
  return coord.x >= 0.0 && coord.y >= 0.0;
}

auto expandScanName(const spef::Exchange& exchange, std::string_view name) -> std::string
{
  return spef::removeEscapes(spef::stripQuotes(spef::expandName(exchange, std::string{name})));
}

auto extractGeometry(std::string_view text, int dbu) -> ScanGeometry
{
  ScanGeometry geometry;
  bool has_llx = false;
  bool has_lly = false;
  bool has_urx = false;
  bool has_ury = false;

  while (true) {
    const auto token = string::take_token(text);
    if (token.empty()) {
      break;
    }

    if (auto llx = string::parse_double_after_prefix(token, "$llx=")) {
      geometry.llx = unit::to_dbu(*llx, dbu);
      has_llx = true;
    } else if (auto lly = string::parse_double_after_prefix(token, "$lly=")) {
      geometry.lly = unit::to_dbu(*lly, dbu);
      has_lly = true;
    } else if (auto urx = string::parse_double_after_prefix(token, "$urx=")) {
      geometry.urx = unit::to_dbu(*urx, dbu);
      has_urx = true;
    } else if (auto ury = string::parse_double_after_prefix(token, "$ury=")) {
      geometry.ury = unit::to_dbu(*ury, dbu);
      has_ury = true;
    } else if (auto layer = string::parse_int_after_prefix(token, "$lvl=")) {
      geometry.layer = *layer;
      geometry.has_layer = true;
    } else if (auto length = string::parse_double_after_prefix(token, "$l=")) {
      geometry.length = *length;
      geometry.has_length = true;
    } else if (auto width = string::parse_double_after_prefix(token, "$w=")) {
      geometry.width = *width;
      geometry.has_width = true;
    } else if (auto direction = string::parse_int_after_prefix(token, "$dir=")) {
      geometry.direction = *direction;
      geometry.has_direction = true;
    }
  }

  geometry.has_box = has_llx && has_lly && has_urx && has_ury && geometry.llx != geometry.urx && geometry.lly != geometry.ury;
  return geometry;
}

auto getDesignName(const spef::Exchange& exchange, const std::string& spef_file) -> std::string
{
  for (const auto& header : exchange.header) {
    if (header.key == "*DESIGN" && !header.value.empty()) {
      return header.value;
    }
  }

  const auto stem = std::filesystem::path(spef_file).stem().string();
  return stem.empty() ? std::string{"plot_spef"} : stem;
}

auto getHeaderValue(const spef::Exchange& exchange, const std::string& key) -> std::string
{
  for (const auto& header : exchange.header) {
    if (header.key == key) {
      return header.value;
    }
  }
  return {};
}

auto normalizeNetFilterName(const spef::Exchange& exchange, const std::string& name) -> std::string
{
  return spef::removeEscapes(spef::stripQuotes(spef::expandName(exchange, name)));
}

auto owningNetName(const std::string& node_name) -> std::string
{
  const auto delimiter = node_name.find(':');
  return delimiter == std::string::npos ? node_name : node_name.substr(0, delimiter);
}

auto fallbackBox(Node& node) -> void
{
  constexpr int kHalfSize = 2;
  node.llx = node.x - kHalfSize;
  node.lly = node.y - kHalfSize;
  node.urx = node.x + kHalfSize;
  node.ury = node.y + kHalfSize;
  node.has_box = true;
}

auto buildNode(const spef::ConnEntry& conn, int dbu) -> Node
{
  Node node;
  node.name = conn.pin_port_name;
  node.layer = std::max(conn.layer, 0);
  if (hasCoord(conn.coordinate)) {
    node.x = unit::to_dbu(conn.coordinate.x, dbu);
    node.y = unit::to_dbu(conn.coordinate.y, dbu);
    node.has_point = true;
  }
  if (hasCoord(conn.ll_coordinate) && hasCoord(conn.ur_coordinate)) {
    node.llx = unit::to_dbu(conn.ll_coordinate.x, dbu);
    node.lly = unit::to_dbu(conn.ll_coordinate.y, dbu);
    node.urx = unit::to_dbu(conn.ur_coordinate.x, dbu);
    node.ury = unit::to_dbu(conn.ur_coordinate.y, dbu);
    node.has_box = node.llx != node.urx && node.lly != node.ury;
  }
  if (!node.has_box && node.has_point) {
    fallbackBox(node);
  }
  return node;
}

auto applyGeometry(Node& node, const ScanGeometry& geometry) -> void
{
  if (geometry.has_layer) {
    node.layer = geometry.layer;
  }
  if (geometry.has_box) {
    node.llx = geometry.llx;
    node.lly = geometry.lly;
    node.urx = geometry.urx;
    node.ury = geometry.ury;
    node.has_box = true;
  }
}

auto applyGeometry(Resistor& resistor, const ScanGeometry& geometry) -> void
{
  if (geometry.has_length) {
    resistor.length = geometry.length;
    resistor.has_length = true;
  }
  if (geometry.has_width) {
    resistor.width = geometry.width;
    resistor.has_width = true;
  }
  if (geometry.has_layer) {
    resistor.layer = geometry.layer;
    resistor.has_layer = true;
  }
  if (geometry.has_direction) {
    resistor.direction = geometry.direction;
    resistor.has_direction = true;
  }
  if (geometry.has_box) {
    resistor.llx = geometry.llx;
    resistor.lly = geometry.lly;
    resistor.urx = geometry.urx;
    resistor.ury = geometry.ury;
    resistor.has_box = true;
  }
}

auto applyNodeGeometry(Model& model, Net& net, const std::string& node_name, const ScanGeometry& geometry) -> void
{
  auto local_node_it = net.nodes_by_name.find(node_name);
  if (local_node_it != net.nodes_by_name.end()) {
    applyGeometry(*local_node_it->second, geometry);
    return;
  }

  auto node_it = model.nodes_by_name.find(node_name);
  if (node_it != model.nodes_by_name.end()) {
    applyGeometry(*node_it->second, geometry);
  }
}

auto parseLayerMapLine(std::string_view line, Model& model) -> void
{
  auto payload = string::trim_view(line.substr(2));
  const auto index = string::take_token(payload);
  const auto name = string::take_token(payload);
  auto layer_id = string::parse_prefixed_index<int>(index);
  if (!layer_id.has_value() || name.empty()) {
    return;
  }
  model.layer_names[*layer_id] = spef::removeEscapes(spef::stripQuotes(std::string{name}));
}

auto buildNetMap(Model& model) -> std::unordered_map<std::string, Net*>
{
  std::unordered_map<std::string, Net*> net_map;
  net_map.reserve(model.nets.size());
  for (auto& net : model.nets) {
    net_map[net.name] = &net;
  }
  return net_map;
}

// StarRC-style SPEF comments carry geometry and layer-map details that the
// parser intentionally keeps out of the normalized Exchange model.
auto augmentModelFromSpefText(const spef::Exchange& exchange, Model& model, const Config& config) -> void
{
  std::ifstream file(config.spef_file);
  if (!file.is_open()) {
    return;
  }

  auto net_map = buildNetMap(model);
  Net* current_net = nullptr;
  ScanSection section = ScanSection::kNone;
  bool in_layer_map = false;
  std::size_t res_index = 0;

  std::string line;
  while (std::getline(file, line)) {
    const auto stripped = string::trim_view(line);
    if (stripped.empty()) {
      continue;
    }

    if (string::starts_with(stripped, "//")) {
      if (string::starts_with(stripped, "// *LAYER_MAP")) {
        in_layer_map = true;
        continue;
      }
      if (in_layer_map) {
        parseLayerMapLine(stripped, model);
      }
      continue;
    }
    in_layer_map = false;

    const auto comment_pos = line.find("//");
    const auto content
        = string::trim_view(comment_pos == std::string::npos ? std::string_view{line} : std::string_view{line}.substr(0, comment_pos));
    const auto comment = comment_pos == std::string::npos ? std::string_view{} : std::string_view{line}.substr(comment_pos + 2);
    if (content.empty()) {
      continue;
    }

    if (string::starts_with(content, "*D_NET")) {
      auto content_tail = content;
      static_cast<void>(string::take_token(content_tail));
      auto net_name_token = string::take_token(content_tail);
      if (net_name_token.empty()) {
        current_net = nullptr;
        section = ScanSection::kNone;
        continue;
      }
      std::string net_name = expandScanName(exchange, net_name_token);
      const auto net_it = net_map.find(net_name);
      current_net = net_it == net_map.end() ? nullptr : net_it->second;
      section = ScanSection::kNone;
      res_index = 0;
      continue;
    }
    if (string::starts_with(content, "*END")) {
      current_net = nullptr;
      section = ScanSection::kNone;
      continue;
    }
    if (string::starts_with(content, "*CONN")) {
      section = ScanSection::kConn;
      continue;
    }
    if (string::starts_with(content, "*CAP")) {
      section = ScanSection::kCap;
      continue;
    }
    if (string::starts_with(content, "*RES")) {
      section = ScanSection::kRes;
      res_index = 0;
      continue;
    }
    if (current_net == nullptr) {
      continue;
    }

    if (section == ScanSection::kConn
        && (string::starts_with(content, "*I") || string::starts_with(content, "*P") || string::starts_with(content, "*N"))) {
      auto content_tail = content;
      static_cast<void>(string::take_token(content_tail));
      const auto node_name_token = string::take_token(content_tail);
      if (!node_name_token.empty()) {
        const std::string node_name = expandScanName(exchange, node_name_token);
        applyNodeGeometry(model, *current_net, node_name, extractGeometry(comment, config.dbu));
      }
      continue;
    }

    if (section == ScanSection::kRes && !content.empty() && std::isdigit(static_cast<unsigned char>(content.front()))) {
      if (res_index < current_net->resistors.size()) {
        applyGeometry(current_net->resistors[res_index], extractGeometry(comment, config.dbu));
      }
      ++res_index;
    }
  }
}

// A focused net plot still needs directly coupled neighbors so Cc shapes can be
// resolved and drawn with useful context.
auto collectVisibleNetNames(const spef::Exchange& exchange, const Config& config) -> std::optional<std::unordered_set<std::string>>
{
  if (!config.hasNetFilter()) {
    return std::nullopt;
  }

  const std::string target_net = normalizeNetFilterName(exchange, config.net_name);
  std::unordered_set<std::string> visible_nets;
  visible_nets.insert(target_net);

  for (const auto& net : exchange.nets) {
    for (const auto& cap : net.caps) {
      if (cap.node2.empty()) {
        continue;
      }
      const std::string net1 = owningNetName(cap.node1);
      const std::string net2 = owningNetName(cap.node2);
      if (net1 == target_net) {
        visible_nets.insert(net2);
      }
      if (net2 == target_net) {
        visible_nets.insert(net1);
      }
    }
  }
  return visible_nets;
}

auto shouldBuildNet(const std::optional<std::unordered_set<std::string>>& visible_net_names, const std::string& net_name) -> bool
{
  return !visible_net_names.has_value() || visible_net_names->contains(net_name);
}

auto applyNetFilter(const spef::Exchange& exchange, Model& model, const Config& config) -> void
{
  if (!config.hasNetFilter()) {
    for (auto& net : model.nets) {
      net.visible = true;
    }
    return;
  }

  const std::string target_net = normalizeNetFilterName(exchange, config.net_name);
  std::unordered_set<std::string> visible_nets;
  visible_nets.insert(target_net);

  for (const auto& net : model.nets) {
    for (const auto& cap : net.coupling_caps) {
      const std::string net1 = owningNetName(cap.node1);
      const std::string net2 = owningNetName(cap.node2);
      if (net1 == target_net) {
        visible_nets.insert(net2);
      }
      if (net2 == target_net) {
        visible_nets.insert(net1);
      }
    }
  }

  bool found_target = false;
  for (auto& net : model.nets) {
    net.visible = visible_nets.contains(net.name);
    found_target = found_target || net.name == target_net;
  }
  if (!found_target) {
    LOG_ERROR << "plot_spef warning: target net not found: " << config.net_name;
  }
}

// Convert the normalized SPEF model into the plotting model. Geometry from
// vendor comments is applied later by augmentModelFromSpefText().
auto reserveCapacitors(Net& net, const spef::Net& spef_net, const Config& config, bool need_coupling_caps) -> void
{
  if (!need_coupling_caps && !config.plotGroundCap()) {
    return;
  }

  std::size_t coupling_cap_count = 0;
  std::size_t ground_cap_count = 0;
  for (const auto& cap : spef_net.caps) {
    if (cap.node2.empty()) {
      ground_cap_count++;
    } else {
      coupling_cap_count++;
    }
  }
  if (need_coupling_caps) {
    net.coupling_caps.reserve(coupling_cap_count);
  }
  if (config.plotGroundCap()) {
    net.ground_caps.reserve(ground_cap_count);
  }
}

auto appendCapacitors(Net& net, const spef::Net& spef_net, const Config& config, bool need_coupling_caps) -> void
{
  if (!need_coupling_caps && !config.plotGroundCap()) {
    return;
  }

  for (const auto& cap : spef_net.caps) {
    Capacitor capacitor{.node1 = cap.node1, .node2 = cap.node2, .value = cap.res_or_cap};
    if (cap.node2.empty()) {
      if (config.plotGroundCap()) {
        net.ground_caps.push_back(std::move(capacitor));
      }
    } else if (need_coupling_caps) {
      net.coupling_caps.push_back(std::move(capacitor));
    }
  }
}

auto appendNodes(Net& net, const spef::Net& spef_net, int dbu) -> void
{
  for (const auto& conn : spef_net.conns) {
    net.nodes.push_back(buildNode(conn, dbu));
  }
}

auto indexNetNodes(Model& model, Net& net) -> void
{
  for (auto& node : net.nodes) {
    model.nodes_by_name[node.name] = &node;
    net.nodes_by_name[node.name] = &node;
  }
}

auto appendResistors(Net& net, const spef::Net& spef_net) -> void
{
  for (std::size_t res_index = 0; res_index < spef_net.ress.size(); ++res_index) {
    const auto& res = spef_net.ress[res_index];
    net.resistors.push_back(Resistor{.node1 = res.node1, .node2 = res.node2, .value = res.res_or_cap, .index = res_index});
  }
}

auto buildNet(const spef::Net& spef_net, const Config& config) -> Net
{
  Net net;
  net.name = spef_net.name;
  net.nodes.reserve(spef_net.conns.size());
  net.nodes_by_name.reserve(spef_net.conns.size());

  const bool need_resistors = config.plotResistance() || config.plotCouplingCap() || config.plotGroundCap();
  const bool need_coupling_caps = config.plotCouplingCap() || config.plotGroundCap() || config.hasNetFilter();
  if (need_resistors) {
    net.resistors.reserve(spef_net.ress.size());
  }
  reserveCapacitors(net, spef_net, config, need_coupling_caps);

  appendNodes(net, spef_net, config.dbu);
  if (need_resistors) {
    appendResistors(net, spef_net);
  }
  appendCapacitors(net, spef_net, config, need_coupling_caps);
  return net;
}

auto reserveModel(Model& model, const spef::Exchange& exchange, const std::optional<std::unordered_set<std::string>>& visible_net_names) -> void
{
  model.nets.reserve(visible_net_names.has_value() ? std::min(exchange.nets.size(), visible_net_names->size()) : exchange.nets.size());

  std::size_t node_count = 0;
  for (const auto& spef_net : exchange.nets) {
    if (shouldBuildNet(visible_net_names, spef_net.name)) {
      node_count += spef_net.conns.size();
    }
  }
  model.nodes_by_name.reserve(node_count);
}

auto initModelMetadata(const spef::Exchange& exchange, const Config& config) -> Model
{
  Model model;
  model.design_name = getDesignName(exchange, config.spef_file);
  model.vendor_name = getHeaderValue(exchange, "*VENDOR");
  model.program_name = getHeaderValue(exchange, "*PROGRAM");
  model.cap_unit = spef::getSpefCapUnit(exchange);
  model.res_unit = spef::getSpefResUnit(exchange);
  model.dbu = config.dbu;
  return model;
}

}  // namespace

auto ModelBuilder::build(const spef::Exchange& exchange, const Config& config) const -> Model
{
  Model model = initModelMetadata(exchange, config);
  const auto visible_net_names = collectVisibleNetNames(exchange, config);
  reserveModel(model, exchange, visible_net_names);

  for (const auto& spef_net : exchange.nets) {
    if (!shouldBuildNet(visible_net_names, spef_net.name)) {
      continue;
    }
    model.nets.push_back(buildNet(spef_net, config));
    indexNetNodes(model, model.nets.back());
  }

  augmentModelFromSpefText(exchange, model, config);
  resolveCapacitorEdges(model, config);
  applyNetFilter(exchange, model, config);
  return model;
}

}  // namespace ircx::plot_spef
