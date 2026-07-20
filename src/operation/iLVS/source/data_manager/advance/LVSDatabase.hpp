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
};

struct LVSNetlist
{
  std::string design_name;
  std::unordered_map<std::string, LVSNet> net_map;
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
