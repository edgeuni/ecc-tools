#include "LVSInterface.hpp"

#include "DataManager.hpp"
#include "LVSChecker.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ilvs {

LVSInterface& LVSInterface::getInst()
{
  if (_lvs_interface_instance == nullptr) {
    _lvs_interface_instance = new LVSInterface();
  }
  return *_lvs_interface_instance;
}

void LVSInterface::destroyInst()
{
  if (_lvs_interface_instance != nullptr) {
    delete _lvs_interface_instance;
    _lvs_interface_instance = nullptr;
  }
}

#if 1  // 外部调用LVS的API

#if 1  // iLVS

void LVSInterface::initLVS(const std::map<std::string, std::any>& config_map)
{
  Logger::initInst();
  // clang-format off
  LVSLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  LVSLOG.info(Loc::current(), "______________    _________    _____________________________________  ");
  LVSLOG.info(Loc::current(), "___(_)__  /__ |  / /_  ___/    __  ___/__  __/__    |__  __ \\__  __/ ");
  LVSLOG.info(Loc::current(), "__  /__  / __ | / /_____ \\     _____ \\__  /  __  /| |_  /_/ /_  /   ");
  LVSLOG.info(Loc::current(), "_  / _  /____ |/ / ____/ /     ____/ /_  /   _  ___ |  _, _/_  /      ");
  LVSLOG.info(Loc::current(), "/_/  /_____/____/  /____/      /____/ /_/    /_/  |_/_/ |_| /_/       ");
  LVSLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // clang-format on
  LVSLOG.printLogFilePath();
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  DataManager::initInst();
  std::map<std::string, std::any> mutable_config_map = config_map;
  LVSDM.input(mutable_config_map);

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::runLVS()
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  LVSDatabase& database = LVSDM.getDatabase();
  LVSCheckResult& check_result = database.getCheckResult();
  const LVSNetlist& expected_netlist = database.getExpectedNetlist();
  const LVSNetlist& physical_netlist = database.getPhysicalNetlist();
  check_result = LVSChecker::check(expected_netlist, physical_netlist);

  uint64_t expected_terminal_num = 0;
  for (const auto& [net_name, net] : expected_netlist.net_map) {
    (void) net_name;
    expected_terminal_num += net.terminal_list.size();
  }
  uint64_t physical_terminal_num = 0;
  uint64_t physical_wire_segment_num = 0;
  uint64_t physical_via_num = 0;
  for (const auto& [net_name, net] : physical_netlist.net_map) {
    (void) net_name;
    physical_terminal_num += net.terminal_list.size();
    physical_wire_segment_num += net.wire_segment_num;
    physical_via_num += net.via_num;
  }

  fort::char_table netlist_summary_table;
  {
    netlist_summary_table.set_cell_text_align(fort::text_align::right);
    netlist_summary_table << fort::header << "Netlist Summary"
                          << "Expected"
                          << "Physical" << fort::endr;
    netlist_summary_table << "Net" << check_result.expected_net_num << check_result.physical_net_num << fort::endr;
    netlist_summary_table << "Instance" << expected_netlist.logical_graph.instance_map.size() << physical_netlist.logical_graph.instance_map.size()
                          << fort::endr;
    netlist_summary_table << "Net Edge" << expected_netlist.logical_graph.net_edge_num << physical_netlist.logical_graph.net_edge_num << fort::endr;
    netlist_summary_table << "Terminal" << expected_terminal_num << physical_terminal_num << fort::endr;
    netlist_summary_table << "Wire Segment" << "-" << physical_wire_segment_num << fort::endr;
    netlist_summary_table << "Via" << "-" << physical_via_num << fort::endr;
    netlist_summary_table << "Graph Node" << "-" << physical_netlist.physical_graph.node_num << fort::endr;
    netlist_summary_table << "Graph Component" << "-" << physical_netlist.physical_graph.component_num << fort::endr;
  }

  uint64_t total_mismatch_num = 0;
  fort::char_table check_summary_table;
  {
    check_summary_table.set_cell_text_align(fort::text_align::right);
    check_summary_table << fort::header << "Check Summary"
                        << "Count" << fort::endr;
    check_summary_table << "Missing Net" << check_result.missing_net_num << fort::endr;
    check_summary_table << "Unexpected Net" << check_result.unexpected_net_num << fort::endr;
    check_summary_table << "Open Net" << check_result.open_net_num << fort::endr;
    check_summary_table << "Missing Terminal" << check_result.missing_terminal_num << fort::endr;
    check_summary_table << "Unrouted Net" << check_result.unrouted_net_num << fort::endr;
    check_summary_table << "Short Component" << check_result.short_component_num << fort::endr;
    check_summary_table << "Floating Power Port" << check_result.floating_power_port_num << fort::endr;
    check_summary_table << "Floating Ground Port" << check_result.floating_ground_port_num << fort::endr;
    total_mismatch_num = check_result.missing_net_num + check_result.unexpected_net_num + check_result.open_net_num
                         + check_result.missing_terminal_num + check_result.unrouted_net_num + check_result.short_component_num
                         + check_result.floating_power_port_num + check_result.floating_ground_port_num;
    check_summary_table << fort::header << "Total" << total_mismatch_num << fort::endr;
  }
  LVSUTIL.printTableList({netlist_summary_table, check_summary_table});

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::destroyLVS()
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  LVSDM.output();
  DataManager::destroyInst();

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());

  LVSLOG.printLogFilePath();
  // clang-format off
  LVSLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  LVSLOG.info(Loc::current(), "______________    _________    _____________________   _____________________  __  ");
  LVSLOG.info(Loc::current(), "___(_)__  /__ |  / /_  ___/    ___  ____/___  _/__  | / /___  _/_  ___/__  / / /  ");
  LVSLOG.info(Loc::current(), "__  /__  / __ | / /_____ \\     __  /_    __  / __   |/ / __  / _____ \\__  /_/ / ");
  LVSLOG.info(Loc::current(), "_  / _  /____ |/ / ____/ /     _  __/   __/ /  _  /|  / __/ /  ____/ /_  __  /    ");
  LVSLOG.info(Loc::current(), "/_/  /_____/____/  /____/      /_/      /___/  /_/ |_/  /___/  /____/ /_/ /_/     ");
  LVSLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // clang-format on
  Logger::destroyInst();
}

// private

LVSInterface* LVSInterface::_lvs_interface_instance = nullptr;

#endif

#endif

}  // namespace ilvs
