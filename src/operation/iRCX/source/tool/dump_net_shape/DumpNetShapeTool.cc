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
#include "DumpNetShapeTool.hh"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#include "Geometry.hh"
#include "LayerTable.hh"
#include "LayoutData.hh"
#include "RCXData.hh"
#include "log/Log.hh"

namespace ircx {
namespace {

enum class ShapeCode : std::size_t
{
  kSegment = 0,
  kPatch,
  kViaNonCut,
  kViaCut,
  kPinNonCut,
  kPinCut,
};

constexpr std::array<char, 6> kShapeCodeNames = {'A', 'B', 'C', 'D', 'E', 'F'};

auto shapeIndex(ShapeCode code) -> std::size_t
{
  return static_cast<std::size_t>(code);
}

auto shapeCodeName(ShapeCode code) -> char
{
  return kShapeCodeNames[shapeIndex(code)];
}

auto validLayer(Size layer_id) -> bool
{
  return layer_id != kMaxSize;
}

auto layerName(const LayoutData& layout, const LayerTable& layer_table, Size layer_id) -> Str
{
  if (auto it = layout.routing_layers.find(layer_id); it != layout.routing_layers.end()) {
    return it->second.layer_name();
  }

  try {
    return layer_table.design_name(layer_id);
  } catch (const std::out_of_range&) {
    return "UNKNOWN";
  }
}

void writeQuoted(std::ostream& os, const Str& value)
{
  os << '"';
  for (char ch : value) {
    if (ch == '\\' || ch == '"') {
      os << '\\' << ch;
    } else if (ch == '\n') {
      os << "\\n";
    } else {
      os << ch;
    }
  }
  os << '"';
}

void writeBool(std::ostream& os, bool value)
{
  os << (value ? "true" : "false");
}

void writeSize(std::ostream& os, Size value)
{
  if (value == kMaxSize) {
    os << "NA";
    return;
  }
  os << value;
}

void writePoint(std::ostream& os, const GtlPointI& point)
{
  os << '[' << geom::x(point) << ' ' << geom::y(point) << ']';
}

void writeRect(std::ostream& os, const GtlRectI& rect)
{
  os << '[' << geom::min_x(rect) << ' ' << geom::min_y(rect) << ' ' << geom::max_x(rect) << ' ' << geom::max_y(rect) << ']';
}

auto isCutLayer(Size layer_id, const std::set<Size>& cut_layer_ids) -> bool
{
  return cut_layer_ids.find(layer_id) != cut_layer_ids.end();
}

void collectCutLayers(const Net& net, std::set<Size>& cut_layer_ids)
{
  for (const Via& via : net.vias) {
    if (validLayer(via.layer_rect_cut.first)) {
      cut_layer_ids.insert(via.layer_rect_cut.first);
    }
  }
}

auto buildLayerOrder(const LayoutData& layout) -> std::vector<Size>
{
  std::set<Size> cut_layer_ids;
  std::map<std::pair<Size, Size>, Size> cut_by_routing_pair;
  std::set<Size> layer_ids_with_shapes;

  auto remember_via = [&](const Via& via) {
    if (validLayer(via.layer_rect_btm.first)) {
      layer_ids_with_shapes.insert(via.layer_rect_btm.first);
    }
    if (validLayer(via.layer_rect_top.first)) {
      layer_ids_with_shapes.insert(via.layer_rect_top.first);
    }
    if (validLayer(via.layer_rect_cut.first)) {
      layer_ids_with_shapes.insert(via.layer_rect_cut.first);
      cut_layer_ids.insert(via.layer_rect_cut.first);
    }

    const Size lower = std::min(via.layer_rect_btm.first, via.layer_rect_top.first);
    const Size upper = std::max(via.layer_rect_btm.first, via.layer_rect_top.first);
    if (validLayer(lower) && validLayer(upper) && validLayer(via.layer_rect_cut.first)) {
      cut_by_routing_pair[{lower, upper}] = via.layer_rect_cut.first;
    }
  };

  auto remember_net = [&](const Net& net) {
    for (const Segment& segment : net.segments) {
      if (validLayer(segment.layer_id)) {
        layer_ids_with_shapes.insert(segment.layer_id);
      }
    }
    for (const Patch& patch : net.patches) {
      if (validLayer(patch.layer_id)) {
        layer_ids_with_shapes.insert(patch.layer_id);
      }
    }
    for (const Via& via : net.vias) {
      remember_via(via);
    }
    for (const Pin& pin : net.pins) {
      for (const auto& [layer_id, rect] : pin.layer_id_rects) {
        (void) rect;
        if (validLayer(layer_id)) {
          layer_ids_with_shapes.insert(layer_id);
        }
      }
    }
  };

  for (const Net& net : layout.net_vec) {
    remember_net(net);
  }
  remember_net(layout.special_net);

  std::vector<Size> routing_layer_ids;
  routing_layer_ids.reserve(layout.routing_layers.size());
  for (const auto& [layer_id, routing_layer] : layout.routing_layers) {
    (void) routing_layer;
    routing_layer_ids.push_back(layer_id);
    layer_ids_with_shapes.insert(layer_id);
  }
  std::sort(routing_layer_ids.begin(), routing_layer_ids.end());

  const Size first_cut_layer_id = routing_layer_ids.empty() ? kMaxSize : routing_layer_ids.back() + 1;
  for (std::size_t i = 0; i + 1 < routing_layer_ids.size(); ++i) {
    const Size cut_layer_id = first_cut_layer_id + i;
    cut_layer_ids.insert(cut_layer_id);
    layer_ids_with_shapes.insert(cut_layer_id);
    cut_by_routing_pair.try_emplace({routing_layer_ids[i], routing_layer_ids[i + 1]}, cut_layer_id);
  }

  std::vector<Size> ordered;
  ordered.reserve(layer_ids_with_shapes.size());
  std::set<Size> emitted;
  auto emit = [&](Size layer_id) {
    if (validLayer(layer_id) && emitted.insert(layer_id).second) {
      ordered.push_back(layer_id);
    }
  };

  for (std::size_t i = 0; i < routing_layer_ids.size(); ++i) {
    emit(routing_layer_ids[i]);
    if (i + 1 >= routing_layer_ids.size()) {
      continue;
    }
    const auto it = cut_by_routing_pair.find({routing_layer_ids[i], routing_layer_ids[i + 1]});
    if (it != cut_by_routing_pair.end()) {
      emit(it->second);
    }
  }

  for (Size layer_id : layer_ids_with_shapes) {
    if (cut_layer_ids.find(layer_id) == cut_layer_ids.end()) {
      emit(layer_id);
    }
  }
  for (Size layer_id : cut_layer_ids) {
    emit(layer_id);
  }

  return ordered;
}

auto outputFileName(const LayoutData& layout) -> Str
{
  return (layout.design_name.empty() ? "design" : layout.design_name) + ".shape";
}

void writeHeader(std::ostream& os, const LayoutData& layout, const LayerTable& layer_table, const std::vector<Size>& layer_order)
{
  os << "# dump_net_shape\n";
  os << "# format_version: 2\n";
  os << "# design_name: ";
  writeQuoted(os, layout.design_name);
  os << '\n';
  os << "# dbu_per_micron: " << layout.dbu_per_micron << '\n';
  os << "# purpose: AI-readable dump of DEF/LEF-derived raw net shapes; compare with StarRC topology from 8_spef_topology_starrc.py\n";
  os << "# shape_code: A=Segment B=Patch C=Via_non_cut_layer D=Via_cut_layer E=Pin_non_cut_layer F=Pin_cut_layer\n";
  os << "# shape_code_note: C/E are non-cut shape types on the current layer; they do not imply upper/lower layer direction\n";
  os << "# shape_code_note: D/F are cut-layer shape types on the current layer\n";
  os << "# coordinate_format: point=[x y], rect=[llx lly urx ury], all in DBU\n";
  os << "# records:\n";
  os << "#   NET <id|NA> <name> <regular|special> <segment_count> <patch_count> <via_count> <pin_count>\n";
  os << "#   S  <id> <layer> <rect>\n";
  os << "#   P  <id> <layer> <rect>\n";
  os << "#   V  <id> <name> <point>\n";
  os << "#   VS <via_id> <shape_id> <code:C|D> <layer> <rect>\n";
  os << "#   PN <id> <name> <is_port> <is_driver> <is_input> <is_output>\n";
  os << "#   PS <pin_id> <shape_id> <code:E|F> <layer> <rect>\n";
  os << "# layer_order low_to_high: index layer_id design_layer_name\n";
  for (std::size_t i = 0; i < layer_order.size(); ++i) {
    os << "# L " << i << ' ' << layer_order[i] << ' ';
    writeQuoted(os, layerName(layout, layer_table, layer_order[i]));
    os << '\n';
  }
}

void writeNetShapes(std::ostream& os, const Net& net, const std::set<Size>& cut_layer_ids)
{
  for (std::size_t i = 0; i < net.segments.size(); ++i) {
    const Segment& segment = net.segments[i];
    os << "S " << i << ' ';
    writeSize(os, segment.layer_id);
    os << ' ';
    writeRect(os, segment.rect);
    os << '\n';
  }

  for (std::size_t i = 0; i < net.patches.size(); ++i) {
    const Patch& patch = net.patches[i];
    os << "P " << i << ' ';
    writeSize(os, patch.layer_id);
    os << ' ';
    writeRect(os, patch.rect);
    os << '\n';
  }

  for (std::size_t i = 0; i < net.vias.size(); ++i) {
    const Via& via = net.vias[i];
    os << "V " << i << ' ';
    writeQuoted(os, via.name);
    os << ' ';
    writePoint(os, via.point);
    os << '\n';

    auto write_via_shape = [&](std::size_t shape_id, ShapeCode code, const std::pair<Size, GtlRectI>& layer_rect) {
      os << "VS " << i << ' ' << shape_id << ' ' << shapeCodeName(code) << ' ';
      writeSize(os, layer_rect.first);
      os << ' ';
      writeRect(os, layer_rect.second);
      os << '\n';
    };
    write_via_shape(0, ShapeCode::kViaNonCut, via.layer_rect_btm);
    write_via_shape(1, ShapeCode::kViaCut, via.layer_rect_cut);
    write_via_shape(2, ShapeCode::kViaNonCut, via.layer_rect_top);
  }

  for (std::size_t i = 0; i < net.pins.size(); ++i) {
    const Pin& pin = net.pins[i];
    os << "PN " << i << ' ';
    writeQuoted(os, pin.name);
    os << ' ';
    writeBool(os, pin.is_port());
    os << ' ';
    writeBool(os, pin.is_driver);
    os << ' ';
    writeBool(os, pin.is_input);
    os << ' ';
    writeBool(os, pin.is_output);
    os << '\n';

    for (std::size_t shape_id = 0; shape_id < pin.layer_id_rects.size(); ++shape_id) {
      const auto& [layer_id, rect] = pin.layer_id_rects[shape_id];
      const ShapeCode code = isCutLayer(layer_id, cut_layer_ids) ? ShapeCode::kPinCut : ShapeCode::kPinNonCut;
      os << "PS " << i << ' ' << shape_id << ' ' << shapeCodeName(code) << ' ';
      writeSize(os, layer_id);
      os << ' ';
      writeRect(os, rect);
      os << '\n';
    }
  }
}

void writeRegularNet(std::ostream& os, const Net& net, const std::set<Size>& cut_layer_ids)
{
  os << "NET " << net.id << ' ';
  writeQuoted(os, net.name);
  os << " regular " << net.segments.size() << ' ' << net.patches.size() << ' ' << net.vias.size() << ' ' << net.pins.size() << '\n';
  writeNetShapes(os, net, cut_layer_ids);
  os << "END_NET\n";
}

void writeSpecialNet(std::ostream& os, const Net& net, const std::set<Size>& cut_layer_ids)
{
  os << "NET NA \"__SPECIAL_NET__\" special " << net.segments.size() << ' ' << net.patches.size() << ' ' << net.vias.size() << ' '
     << net.pins.size() << '\n';
  writeNetShapes(os, net, cut_layer_ids);
  os << "END_NET\n";
}

auto hasSpecialNetShapes(const Net& net) -> bool
{
  return !net.segments.empty() || !net.patches.empty() || !net.vias.empty() || !net.pins.empty();
}

}  // namespace

auto DumpNetShapeTool::run() -> bool
{
  const RCXData& data = RCX_DATA_INST;
  const LayoutData& layout = data.layout();
  if (layout.empty()) {
    LOG_ERROR << "dump_net_shape failed: layout data is empty. Run init_rcx/adapt DB before dumping shapes.";
    return false;
  }

  const Str output_file = outputFileName(layout);
  std::ofstream ofs(output_file);
  if (!ofs) {
    LOG_ERROR << "dump_net_shape failed: cannot open output file " << output_file;
    return false;
  }

  std::set<Size> cut_layer_ids;
  for (const Net& net : layout.net_vec) {
    collectCutLayers(net, cut_layer_ids);
  }
  collectCutLayers(layout.special_net, cut_layer_ids);

  const std::vector<Size> layer_order = buildLayerOrder(layout);
  writeHeader(ofs, layout, data.layer_table(), layer_order);

  for (const Net& net : layout.net_vec) {
    writeRegularNet(ofs, net, cut_layer_ids);
  }

  if (hasSpecialNetShapes(layout.special_net)) {
    writeSpecialNet(ofs, layout.special_net, cut_layer_ids);
  }
  if (!ofs) {
    LOG_ERROR << "dump_net_shape failed: cannot write output file " << output_file;
    return false;
  }

  LOG_INFO << "dump_net_shape wrote " << output_file;
  return true;
}

}  // namespace ircx
