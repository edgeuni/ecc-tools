// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#include "LVSChecker.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ilvs {

namespace {

constexpr size_t kInvalidShapeIndex = std::numeric_limits<size_t>::max();

class DisjointSet
{
 public:
  explicit DisjointSet(size_t size) : _parent(size), _rank(size, 0)
  {
    for (size_t idx = 0; idx < size; idx++) {
      _parent[idx] = idx;
    }
  }

  size_t find(size_t node)
  {
    if (_parent[node] != node) {
      _parent[node] = find(_parent[node]);
    }
    return _parent[node];
  }

  void unite(size_t first, size_t second)
  {
    first = find(first);
    second = find(second);
    if (first == second) {
      return;
    }
    if (_rank[first] < _rank[second]) {
      std::swap(first, second);
    }
    _parent[second] = first;
    if (_rank[first] == _rank[second]) {
      _rank[first]++;
    }
  }

 private:
  std::vector<size_t> _parent;
  std::vector<uint8_t> _rank;
};

std::vector<std::string> getSortedUniqueStringList(std::vector<std::string> string_list)
{
  std::sort(string_list.begin(), string_list.end());
  string_list.erase(std::unique(string_list.begin(), string_list.end()), string_list.end());
  return string_list;
}

template <typename TValue>
std::vector<std::string> getSortedKeyList(const std::unordered_map<std::string, TValue>& value_map)
{
  std::vector<std::string> key_list;
  key_list.reserve(value_map.size());
  for (const auto& [key, value] : value_map) {
    (void) value;
    key_list.push_back(key);
  }
  return getSortedUniqueStringList(std::move(key_list));
}

std::vector<std::string> getDifference(const std::vector<std::string>& first_list, const std::vector<std::string>& second_list)
{
  std::vector<std::string> difference_list;
  std::set_difference(first_list.begin(), first_list.end(), second_list.begin(), second_list.end(), std::back_inserter(difference_list));
  return difference_list;
}

bool isPowerGroundIO(const std::string& io_pin_name, const LVSPhysicalGraph& physical_graph)
{
  constexpr char kPinPrefix[] = "PIN/";
  const std::string net_name = io_pin_name.rfind(kPinPrefix, 0) == 0 ? io_pin_name.substr(sizeof(kPinPrefix) - 1) : io_pin_name;
  return physical_graph.power_net_set.contains(net_name) || physical_graph.ground_net_set.contains(net_name);
}

std::vector<std::string> getComparedIOList(const std::vector<std::string>& io_pin_list, const LVSPhysicalGraph& physical_graph,
                                           uint64_t& power_ground_io_num)
{
  power_ground_io_num = 0;
  std::vector<std::string> compared_io_list;
  for (const std::string& io_pin_name : getSortedUniqueStringList(io_pin_list)) {
    if (isPowerGroundIO(io_pin_name, physical_graph)) {
      power_ground_io_num++;
    } else {
      compared_io_list.push_back(io_pin_name);
    }
  }
  return compared_io_list;
}

bool isIntersected(const LVSShapeLocation& first_shape, const LVSShapeLocation& second_shape)
{
  return first_shape.ll_x <= second_shape.ur_x && second_shape.ll_x <= first_shape.ur_x && first_shape.ll_y <= second_shape.ur_y
         && second_shape.ll_y <= first_shape.ur_y;
}

struct RoutingCheck
{
  std::string net_name;
  std::string driver_terminal_name;
  std::vector<std::string> disconnected_terminal_list;
  std::vector<LVSShapeLocation> disconnected_shape_list;
  bool missing_driver = false;
  bool connected = false;
};

size_t getTerminalRoot(const LVSNetRoutingGraph& routing_graph, const std::string& terminal_name, DisjointSet& graph)
{
  auto terminal_iter = routing_graph.terminal_shape_map.find(terminal_name);
  if (terminal_iter == routing_graph.terminal_shape_map.end()) {
    return kInvalidShapeIndex;
  }
  for (uint64_t shape_idx : terminal_iter->second) {
    if (shape_idx < routing_graph.shape_list.size()) {
      return graph.find(static_cast<size_t>(shape_idx));
    }
  }
  return kInvalidShapeIndex;
}

RoutingCheck checkNetRoutingConnectivity(const LVSNet& net, const LVSNetRoutingGraph* routing_graph)
{
  RoutingCheck routing_check;
  routing_check.net_name = net.name;
  if (net.terminal_list.size() <= 1) {
    routing_check.connected = true;
    return routing_check;
  }
  if (routing_graph == nullptr) {
    routing_check.disconnected_terminal_list = net.terminal_list;
    return routing_check;
  }

  routing_check.driver_terminal_name = routing_graph->driver_terminal_name;
  DisjointSet graph(routing_graph->shape_list.size());
  std::unordered_map<int32_t, std::vector<size_t>> layer_shape_map;
  for (size_t shape_idx = 0; shape_idx < routing_graph->shape_list.size(); shape_idx++) {
    layer_shape_map[routing_graph->shape_list[shape_idx].layer_id].push_back(shape_idx);
  }
  for (auto& [layer_id, shape_index_list] : layer_shape_map) {
    (void) layer_id;
    std::sort(shape_index_list.begin(), shape_index_list.end(), [&routing_graph](size_t first_idx, size_t second_idx) {
      const LVSShapeLocation& first_shape = routing_graph->shape_list[first_idx];
      const LVSShapeLocation& second_shape = routing_graph->shape_list[second_idx];
      if (first_shape.ll_x != second_shape.ll_x) {
        return first_shape.ll_x < second_shape.ll_x;
      }
      if (first_shape.ll_y != second_shape.ll_y) {
        return first_shape.ll_y < second_shape.ll_y;
      }
      return first_idx < second_idx;
    });
    std::vector<size_t> active_shape_index_list;
    for (size_t shape_idx : shape_index_list) {
      const LVSShapeLocation& shape = routing_graph->shape_list[shape_idx];
      active_shape_index_list.erase(
          std::remove_if(active_shape_index_list.begin(), active_shape_index_list.end(), [&routing_graph, &shape](size_t active_shape_idx) {
            return routing_graph->shape_list[active_shape_idx].ur_x < shape.ll_x;
          }),
          active_shape_index_list.end());
      for (size_t active_shape_idx : active_shape_index_list) {
        if (isIntersected(routing_graph->shape_list[active_shape_idx], shape)) {
          graph.unite(active_shape_idx, shape_idx);
        }
      }
      active_shape_index_list.push_back(shape_idx);
    }
  }
  for (const auto& [bottom_shape_idx, top_shape_idx] : routing_graph->via_shape_pair_list) {
    if (bottom_shape_idx < routing_graph->shape_list.size() && top_shape_idx < routing_graph->shape_list.size()) {
      graph.unite(static_cast<size_t>(bottom_shape_idx), static_cast<size_t>(top_shape_idx));
    }
  }
  for (const auto& [terminal_name, shape_index_list] : routing_graph->terminal_shape_map) {
    (void) terminal_name;
    size_t first_shape_idx = kInvalidShapeIndex;
    for (uint64_t shape_idx : shape_index_list) {
      if (shape_idx >= routing_graph->shape_list.size()) {
        continue;
      }
      if (first_shape_idx == kInvalidShapeIndex) {
        first_shape_idx = static_cast<size_t>(shape_idx);
      } else {
        graph.unite(first_shape_idx, static_cast<size_t>(shape_idx));
      }
    }
  }

  const size_t driver_root = getTerminalRoot(*routing_graph, routing_check.driver_terminal_name, graph);
  if (routing_check.driver_terminal_name.empty() || driver_root == kInvalidShapeIndex) {
    routing_check.missing_driver = true;
    routing_check.disconnected_terminal_list = net.terminal_list;
    routing_check.disconnected_shape_list = routing_graph->shape_list;
    return routing_check;
  }

  std::unordered_set<size_t> disconnected_root_set;
  for (const std::string& terminal_name : net.terminal_list) {
    if (terminal_name == routing_check.driver_terminal_name) {
      continue;
    }
    const size_t terminal_root = getTerminalRoot(*routing_graph, terminal_name, graph);
    if (terminal_root == kInvalidShapeIndex || terminal_root != driver_root) {
      routing_check.disconnected_terminal_list.push_back(terminal_name);
      if (terminal_root != kInvalidShapeIndex) {
        disconnected_root_set.insert(terminal_root);
      }
    }
  }
  if (routing_check.disconnected_terminal_list.empty()) {
    routing_check.connected = true;
    return routing_check;
  }
  for (size_t shape_idx = 0; shape_idx < routing_graph->shape_list.size(); shape_idx++) {
    if (disconnected_root_set.contains(graph.find(shape_idx))) {
      routing_check.disconnected_shape_list.push_back(routing_graph->shape_list[shape_idx]);
    }
  }
  return routing_check;
}

}  // namespace

LVSCheckResult LVSChecker::check(const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist)
{
  LVSCheckResult result;
  const std::vector<std::string> expected_io_list = getComparedIOList(expected_netlist.logical_graph.io_pin_list, physical_netlist.physical_graph,
                                                                        result.expected_power_ground_io_num);
  const std::vector<std::string> physical_io_list = getComparedIOList(physical_netlist.physical_graph.io_pin_list, physical_netlist.physical_graph,
                                                                        result.physical_power_ground_io_num);
  const std::vector<std::string> expected_instance_name_list = getSortedKeyList(expected_netlist.logical_graph.instance_map);
  const std::vector<std::string> physical_instance_name_list = getSortedKeyList(physical_netlist.physical_graph.instance_map);
  const std::vector<std::string> expected_net_name_list = getSortedKeyList(expected_netlist.net_map);
  const std::vector<std::string> physical_net_name_list = getSortedKeyList(physical_netlist.net_map);

  result.expected_io_num = expected_io_list.size();
  result.physical_io_num = physical_io_list.size();
  result.expected_instance_num = expected_instance_name_list.size();
  result.physical_instance_num = physical_instance_name_list.size();
  result.expected_net_num = expected_net_name_list.size();
  result.physical_net_num = physical_net_name_list.size();

  const std::vector<std::string> missing_io_list = getDifference(expected_io_list, physical_io_list);
  const std::vector<std::string> unexpected_io_list = getDifference(physical_io_list, expected_io_list);
  result.missing_io_num = missing_io_list.size();
  result.unexpected_io_num = unexpected_io_list.size();
  for (const std::string& io_pin_name : missing_io_list) {
    result.violation_list.push_back({"MissingIO", "", {io_pin_name}, {}});
  }
  for (const std::string& io_pin_name : unexpected_io_list) {
    result.violation_list.push_back({"UnexpectedIO", "", {io_pin_name}, {}});
  }

  const std::vector<std::string> missing_instance_name_list = getDifference(expected_instance_name_list, physical_instance_name_list);
  const std::vector<std::string> unexpected_instance_name_list = getDifference(physical_instance_name_list, expected_instance_name_list);
  result.missing_instance_num = missing_instance_name_list.size();
  result.unexpected_instance_num = unexpected_instance_name_list.size();
  for (const std::string& instance_name : missing_instance_name_list) {
    LVSViolation violation;
    violation.type = "MissingInstance";
    violation.instance_name = instance_name;
    result.violation_list.push_back(std::move(violation));
  }
  for (const std::string& instance_name : unexpected_instance_name_list) {
    LVSViolation violation;
    violation.type = "UnexpectedInstance";
    violation.instance_name = instance_name;
    result.violation_list.push_back(std::move(violation));
  }

  const std::vector<std::string> missing_net_name_list = getDifference(expected_net_name_list, physical_net_name_list);
  const std::vector<std::string> unexpected_net_name_list = getDifference(physical_net_name_list, expected_net_name_list);
  result.missing_net_num = missing_net_name_list.size();
  result.unexpected_net_num = unexpected_net_name_list.size();
  for (const std::string& net_name : missing_net_name_list) {
    LVSViolation violation;
    violation.type = "MissingNet";
    violation.net_name = net_name;
    violation.terminal_list = expected_netlist.net_map.at(net_name).terminal_list;
    result.violation_list.push_back(std::move(violation));
  }
  for (const std::string& net_name : unexpected_net_name_list) {
    LVSViolation violation;
    violation.type = "UnexpectedNet";
    violation.net_name = net_name;
    violation.terminal_list = physical_netlist.net_map.at(net_name).terminal_list;
    result.violation_list.push_back(std::move(violation));
  }
  for (const std::string& net_name : expected_net_name_list) {
    auto physical_net_iter = physical_netlist.net_map.find(net_name);
    if (physical_net_iter == physical_netlist.net_map.end()) {
      continue;
    }
    const std::vector<std::string> expected_pin_list = getSortedUniqueStringList(expected_netlist.net_map.at(net_name).terminal_list);
    const std::vector<std::string> physical_pin_list = getSortedUniqueStringList(physical_net_iter->second.terminal_list);
    if (expected_pin_list == physical_pin_list) {
      continue;
    }
    result.net_pin_mismatch_num++;
    LVSViolation violation;
    violation.type = "NetPinMismatch";
    violation.net_name = net_name;
    for (const std::string& pin_name : getDifference(expected_pin_list, physical_pin_list)) {
      violation.terminal_list.push_back("NETLIST/" + pin_name);
    }
    for (const std::string& pin_name : getDifference(physical_pin_list, expected_pin_list)) {
      violation.terminal_list.push_back("DEF/" + pin_name);
    }
    result.violation_list.push_back(std::move(violation));
  }

  std::vector<RoutingCheck> routing_check_list(physical_net_name_list.size());
#pragma omp parallel for schedule(dynamic)
  for (int64_t net_idx = 0; net_idx < static_cast<int64_t>(physical_net_name_list.size()); net_idx++) {
    const std::string& net_name = physical_net_name_list[static_cast<size_t>(net_idx)];
    const LVSNet& net = physical_netlist.net_map.at(net_name);
    auto routing_graph_iter = physical_netlist.physical_graph.net_routing_graph_map.find(net_name);
    const LVSNetRoutingGraph* routing_graph = routing_graph_iter == physical_netlist.physical_graph.net_routing_graph_map.end()
                                                  ? nullptr
                                                  : &routing_graph_iter->second;
    routing_check_list[static_cast<size_t>(net_idx)] = checkNetRoutingConnectivity(net, routing_graph);
  }
  result.routing_checked_net_num = routing_check_list.size();
  for (RoutingCheck& routing_check : routing_check_list) {
    if (routing_check.connected) {
      result.routing_connected_net_num++;
      continue;
    }
    if (!routing_check.missing_driver) {
      result.routing_open_net_num++;
      result.routing_open_load_pin_num += routing_check.disconnected_terminal_list.size();
    }
    result.routing_missing_driver_num += routing_check.missing_driver;
    LVSViolation violation;
    violation.type = routing_check.missing_driver ? "RoutingDriverMissing" : "RoutingOpen";
    violation.net_name = std::move(routing_check.net_name);
    violation.driver_terminal_name = std::move(routing_check.driver_terminal_name);
    violation.terminal_list = std::move(routing_check.disconnected_terminal_list);
    violation.shape_list = std::move(routing_check.disconnected_shape_list);
    result.violation_list.push_back(std::move(violation));
  }

  std::vector<uint64_t> short_component_id_list;
  short_component_id_list.reserve(physical_netlist.physical_graph.component_net_map.size());
  for (const auto& [component_id, net_name_list] : physical_netlist.physical_graph.component_net_map) {
    if (getSortedUniqueStringList(net_name_list).size() > 1) {
      short_component_id_list.push_back(component_id);
    }
  }
  std::sort(short_component_id_list.begin(), short_component_id_list.end());
  for (uint64_t component_id : short_component_id_list) {
    LVSViolation violation;
    violation.type = "RoutingShort";
    violation.component_id_list.push_back(component_id);
    violation.related_net_name_list = getSortedUniqueStringList(physical_netlist.physical_graph.component_net_map.at(component_id));
    result.routing_short_component_num++;
    result.violation_list.push_back(std::move(violation));
  }
  return result;
}

}  // namespace ilvs
