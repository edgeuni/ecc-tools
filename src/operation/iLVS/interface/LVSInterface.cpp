#include "LVSInterface.hpp"

#include "DataManager.hpp"
#include "LVSChecker.hpp"
#include "LVSReporter.hpp"
#include "NetlistExtractor.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"
#include "idm.h"

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

void LVSInterface::initLVS(std::map<std::string, std::any> config_map)
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
  LVSDM.input(config_map);

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
  LVSReporter::report(check_result, expected_netlist, physical_netlist, database.getReportDirectoryPath());

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
    netlist_summary_table << "Instance" << expected_netlist.logical_graph.instance_map.size() << "-" << fort::endr;
    netlist_summary_table << "Net Edge" << expected_netlist.logical_graph.net_edge_num << "-" << fort::endr;
    netlist_summary_table << "Terminal" << expected_terminal_num << physical_terminal_num << fort::endr;
    netlist_summary_table << "Wire Segment" << "-" << physical_wire_segment_num << fort::endr;
    netlist_summary_table << "Via" << "-" << physical_via_num << fort::endr;
    netlist_summary_table << "Graph Node" << "-" << physical_netlist.physical_graph.node_num << fort::endr;
    netlist_summary_table << "Graph Edge" << "-" << physical_netlist.physical_graph.edge_num << fort::endr;
    netlist_summary_table << "Graph Component" << "-" << physical_netlist.physical_graph.component_num << fort::endr;
    netlist_summary_table << "Graph Candidate Pair" << "-" << physical_netlist.physical_graph.candidate_pair_num << fort::endr;
    netlist_summary_table << "Graph Max Active" << "-" << physical_netlist.physical_graph.max_active_shape_num << fort::endr;
  }

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
    check_summary_table << "Power/Ground Short" << check_result.power_ground_short_num << fort::endr;
    check_summary_table << "Floating Power Port" << check_result.floating_power_port_num << fort::endr;
    check_summary_table << "Floating Ground Port" << check_result.floating_ground_port_num << fort::endr;
    check_summary_table << "Floating Power Pin" << check_result.floating_power_pin_num << fort::endr;
    check_summary_table << "Floating Ground Pin" << check_result.floating_ground_pin_num << fort::endr;
    check_summary_table << fort::header << "Total" << check_result.violation_list.size() << fort::endr;
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

#endif

#endif

#if 1  // LVS调用外部的API

#if 1  // TopData

#if 1  // input

void LVSInterface::input(std::map<std::string, std::any>& config_map)
{
  wrapConfig(config_map);
}

void LVSInterface::wrapConfig(std::map<std::string, std::any>& config_map)
{
  LVSDM.getConfig().temp_directory_path = LVSUTIL.getConfigValue<std::string>(config_map, "-temp_directory_path", "./lvs_temp_directory");
  LVSDM.getConfig().thread_number = LVSUTIL.getConfigValue<int32_t>(config_map, "-thread_number", 128);
  omp_set_num_threads(std::max(LVSDM.getConfig().thread_number, 1));
}

void LVSInterface::wrapDatabase()
{
  const std::string& verilog_path = dmInst->get_config().get_verilog_path();
  if (verilog_path.empty()) {
    LVSLOG.error(Loc::current(), "The Verilog path is not initialized before init_lvs!");
  }

  // IDB keeps one current design view; preserve the Verilog view before reloading DEF.
  LVSNetlist expected_netlist = NetlistExtractor::extract(dmInst->get_idb_design());
  LVSDM.getDatabase().getExpectedNetlist() = std::move(expected_netlist);
  LVSDM.getDatabase().getExpectedNetlist().physical_graph = {};

  const std::string& def_path = dmInst->get_config().get_def_path();
  if (def_path.empty()) {
    LVSLOG.error(Loc::current(), "The DEF path is not initialized before init_lvs!");
  }
  if (!dmInst->readDef(def_path)) {
    LVSLOG.error(Loc::current(), "Failed to reload DEF '", def_path, "' through IDB!");
  }

  LVSNetlist physical_netlist = NetlistExtractor::extract(dmInst->get_idb_design());
  LVSDM.getDatabase().getPhysicalNetlist() = std::move(physical_netlist);
  LVSDM.getDatabase().getPhysicalNetlist().logical_graph = {};
}

#endif

#if 1  // output

void LVSInterface::output()
{
}

#endif

#endif

#endif

// private

LVSInterface* LVSInterface::_lvs_interface_instance = nullptr;

}  // namespace ilvs
