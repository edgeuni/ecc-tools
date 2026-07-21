// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#include "LVSChecker.hpp"

#include <unordered_set>

namespace ilvs {

LVSCheckResult LVSChecker::check(const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist)
{
  LVSCheckResult result;
  result.expected_net_num = expected_netlist.net_map.size();
  result.physical_net_num = physical_netlist.net_map.size();
  std::unordered_map<uint64_t, std::unordered_set<std::string>> logical_component_net_map;

  for (const auto& [net_name, expected_net] : expected_netlist.net_map) {
    LVSViolation violation;
    violation.net_name = net_name;
    std::unordered_set<uint64_t> component_id_set;
    for (const std::string& terminal_name : expected_net.terminal_list) {
      auto component_iter = physical_netlist.physical_graph.terminal_component_map.find(terminal_name);
      if (component_iter == physical_netlist.physical_graph.terminal_component_map.end()) {
        violation.terminal_list.push_back(terminal_name);
      } else {
        component_id_set.insert(component_iter->second);
        logical_component_net_map[component_iter->second].insert(net_name);
      }
    }
    violation.component_id_list.assign(component_id_set.begin(), component_id_set.end());
    auto physical_net_iter = physical_netlist.net_map.find(net_name);
    if (physical_net_iter == physical_netlist.net_map.end()) {
      result.missing_net_num++;
      result.open_net_num++;
      result.missing_terminal_num += expected_net.terminal_list.size();
      violation.type = "MissingNet";
      violation.terminal_list = expected_net.terminal_list;
      result.violation_list.push_back(std::move(violation));
      continue;
    }

    const LVSNet& physical_net = physical_net_iter->second;
    std::unordered_set<std::string> physical_terminal_set(physical_net.terminal_list.begin(), physical_net.terminal_list.end());
    uint64_t missing_terminal_num = 0;
    for (const std::string& terminal_name : expected_net.terminal_list) {
      if (!physical_terminal_set.contains(terminal_name)) {
        missing_terminal_num++;
      }
    }
    if (missing_terminal_num > 0) {
      result.open_net_num++;
      result.missing_terminal_num += missing_terminal_num;
    }
    if (expected_net.terminal_list.size() > 1 && physical_net.wire_segment_num == 0) {
      result.unrouted_net_num++;
    }
    if (component_id_set.size() > 1 || physical_net.floating_terminal_num > 0 || !violation.terminal_list.empty()) {
      result.open_net_num++;
      violation.type = "Open";
      result.violation_list.push_back(std::move(violation));
    }
    if (physical_net.floating_terminal_num > 0) {
      result.missing_terminal_num += physical_net.floating_terminal_num;
    }
  }
  for (const auto& [net_name, physical_net] : physical_netlist.net_map) {
    (void) physical_net;
    if (!expected_netlist.net_map.contains(net_name)) {
      result.unexpected_net_num++;
    }
  }
  std::unordered_set<uint64_t> reported_short_component_set;
  for (const auto& [component_id, net_name_set] : logical_component_net_map) {
    if (net_name_set.size() > 1) {
      LVSViolation violation;
      violation.type = "Short";
      violation.component_id_list.push_back(component_id);
      violation.terminal_list.assign(net_name_set.begin(), net_name_set.end());
      result.violation_list.push_back(std::move(violation));
      reported_short_component_set.insert(component_id);
    }
  }
  for (const auto& [component_id, net_name_list] : physical_netlist.physical_graph.component_net_map) {
    if (net_name_list.size() > 1 && !reported_short_component_set.contains(component_id)) {
      LVSViolation violation;
      bool has_power_net = false;
      bool has_ground_net = false;
      for (const std::string& net_name : net_name_list) {
        has_power_net = has_power_net || physical_netlist.physical_graph.power_net_set.contains(net_name);
        has_ground_net = has_ground_net || physical_netlist.physical_graph.ground_net_set.contains(net_name);
      }
      violation.type = has_power_net && has_ground_net ? "PowerGroundShort" : "Short";
      result.power_ground_short_num += has_power_net && has_ground_net;
      violation.component_id_list.push_back(component_id);
      violation.terminal_list = net_name_list;
      result.violation_list.push_back(std::move(violation));
      reported_short_component_set.insert(component_id);
    }
  }
  result.short_component_num = reported_short_component_set.size();
  result.floating_power_port_num = physical_netlist.physical_graph.floating_power_port_num;
  result.floating_ground_port_num = physical_netlist.physical_graph.floating_ground_port_num;
  result.floating_power_pin_num = physical_netlist.physical_graph.floating_power_pin_num;
  result.floating_ground_pin_num = physical_netlist.physical_graph.floating_ground_pin_num;
  if (!physical_netlist.physical_graph.floating_power_port_list.empty()) {
    result.violation_list.push_back({"FloatingPowerPort", "", physical_netlist.physical_graph.floating_power_port_list, {}});
  }
  if (!physical_netlist.physical_graph.floating_ground_port_list.empty()) {
    result.violation_list.push_back({"FloatingGroundPort", "", physical_netlist.physical_graph.floating_ground_port_list, {}});
  }
  if (!physical_netlist.physical_graph.floating_power_pin_list.empty()) {
    result.violation_list.push_back({"FloatingPowerPin", "", physical_netlist.physical_graph.floating_power_pin_list, {}});
  }
  if (!physical_netlist.physical_graph.floating_ground_pin_list.empty()) {
    result.violation_list.push_back({"FloatingGroundPin", "", physical_netlist.physical_graph.floating_ground_pin_list, {}});
  }
  return result;
}

}  // namespace ilvs
