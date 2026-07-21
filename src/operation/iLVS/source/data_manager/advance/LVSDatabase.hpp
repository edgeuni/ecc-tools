// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ilvs {

struct LVSNet
{
  std::string name;
  std::vector<std::string> terminal_list;
  uint64_t wire_segment_num = 0;
  uint64_t via_num = 0;
  uint64_t terminal_component_num = 0;
  uint64_t floating_terminal_num = 0;
};

struct LVSPhysicalGraph
{
  uint64_t node_num = 0;
  uint64_t edge_num = 0;
  uint64_t component_num = 0;
  uint64_t short_component_num = 0;
  uint64_t power_port_num = 0;
  uint64_t floating_power_port_num = 0;
  uint64_t ground_port_num = 0;
  uint64_t floating_ground_port_num = 0;
  std::unordered_map<uint64_t, std::vector<std::string>> component_terminal_map;
  std::unordered_map<uint64_t, std::vector<std::string>> component_net_map;
  std::unordered_map<std::string, uint64_t> terminal_component_map;
};

struct LVSViolation
{
  std::string type;
  std::string net_name;
  std::vector<std::string> terminal_list;
  std::vector<uint64_t> component_id_list;
};

struct LVSInstanceNode
{
  std::string name;
  std::vector<std::string> pin_list;
};

struct LVSLogicalGraph
{
  std::unordered_map<std::string, LVSInstanceNode> instance_map;
  std::vector<std::string> io_pin_list;
  uint64_t net_edge_num = 0;
};

struct LVSNetlist
{
  std::string design_name;
  std::unordered_map<std::string, LVSNet> net_map;
  LVSLogicalGraph logical_graph;
  LVSPhysicalGraph physical_graph;
};

struct LVSCheckResult
{
  uint64_t expected_net_num = 0;
  uint64_t physical_net_num = 0;
  uint64_t missing_net_num = 0;
  uint64_t unexpected_net_num = 0;
  uint64_t open_net_num = 0;
  uint64_t missing_terminal_num = 0;
  uint64_t unrouted_net_num = 0;
  uint64_t short_component_num = 0;
  uint64_t floating_power_port_num = 0;
  uint64_t floating_ground_port_num = 0;
  std::vector<LVSViolation> violation_list;
};

class LVSDatabase
{
 public:
  LVSNetlist& getExpectedNetlist() { return _expected_netlist; }
  const LVSNetlist& getExpectedNetlist() const { return _expected_netlist; }
  LVSNetlist& getPhysicalNetlist() { return _physical_netlist; }
  const LVSNetlist& getPhysicalNetlist() const { return _physical_netlist; }
  LVSCheckResult& getCheckResult() { return _check_result; }
  const LVSCheckResult& getCheckResult() const { return _check_result; }

  void reset()
  {
    _expected_netlist = {};
    _physical_netlist = {};
    _check_result = {};
  }

 private:
  LVSNetlist _expected_netlist;
  LVSNetlist _physical_netlist;
  LVSCheckResult _check_result;
};

}  // namespace ilvs
