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

  for (const auto& [net_name, expected_net] : expected_netlist.net_map) {
    auto physical_net_iter = physical_netlist.net_map.find(net_name);
    if (physical_net_iter == physical_netlist.net_map.end()) {
      result.missing_net_num++;
      result.open_net_num++;
      result.missing_terminal_num += expected_net.terminal_list.size();
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
  }
  for (const auto& [net_name, physical_net] : physical_netlist.net_map) {
    (void) physical_net;
    if (!expected_netlist.net_map.contains(net_name)) {
      result.unexpected_net_num++;
    }
  }
  return result;
}

}  // namespace ilvs
