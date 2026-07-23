#include "LVSInterface.hpp"

#include "DataManager.hpp"
#include "LVSChecker.hpp"
#include "LVSReporter.hpp"
#include "LVSSnapshotIO.hpp"
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
  if (!database.hasExpectedNetlist() || !database.hasPhysicalNetlist()) {
    LVSLOG.error(Loc::current(), "run_lvs requires read_lvs to load both the netlist and DEF snapshots first!");
  }
  LVSCheckResult& check_result = database.getCheckResult();
  const LVSNetlist& expected_netlist = database.getExpectedNetlist();
  const LVSNetlist& physical_netlist = database.getPhysicalNetlist();
  check_result = LVSChecker::check(expected_netlist, physical_netlist);
  LVSReporter::report(check_result, expected_netlist, physical_netlist, database.getReportDirectoryPath());
  LVSUTIL.printTableList(LVSReporter::getSummaryTableList(check_result, expected_netlist, physical_netlist));

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::writeLVSNetlist(const std::string& file_path)
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  if (idb_design == nullptr) {
    LVSLOG.error(Loc::current(), "write_lvs_netlist requires a Verilog-backed IDB design!");
  }
  if (dmInst->get_config().get_verilog_path().empty()) {
    LVSLOG.error(Loc::current(), "write_lvs_netlist requires verilog_init before iLVS snapshot extraction!");
  }
  LVSNetlist netlist = NetlistExtractor::extractLogical(idb_design);
  std::string error_message;
  if (!LVSSnapshotIO::write(netlist, LVSSnapshotType::kLogical, file_path, error_message)) {
    LVSLOG.error(Loc::current(), "Failed to write logical iLVS snapshot '", file_path, "': ", error_message);
  }
  LVSLOG.info(Loc::current(), "Wrote logical iLVS snapshot '", file_path, "' with ", netlist.net_map.size(), " nets.");

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::writeLVSDef(const std::string& file_path)
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  if (idb_design == nullptr) {
    LVSLOG.error(Loc::current(), "write_lvs_def requires a DEF-backed IDB design!");
  }
  if (dmInst->get_config().get_def_path().empty()) {
    LVSLOG.error(Loc::current(), "write_lvs_def requires def_init before iLVS snapshot extraction!");
  }
  LVSNetlist netlist = NetlistExtractor::extractPhysical(idb_design);
  std::string error_message;
  if (!LVSSnapshotIO::write(netlist, LVSSnapshotType::kPhysical, file_path, error_message)) {
    LVSLOG.error(Loc::current(), "Failed to write physical iLVS snapshot '", file_path, "': ", error_message);
  }
  LVSLOG.info(Loc::current(), "Wrote physical iLVS snapshot '", file_path, "' with ", netlist.net_map.size(), " nets and ",
              netlist.physical_graph.node_num, " graph nodes.");

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::readLVS(const std::string& netlist_file_path, const std::string& def_file_path)
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  LVSNetlist expected_netlist;
  LVSNetlist physical_netlist;
  std::string error_message;
  if (!LVSSnapshotIO::read(netlist_file_path, LVSSnapshotType::kLogical, expected_netlist, error_message)) {
    LVSLOG.error(Loc::current(), "Failed to read logical iLVS snapshot '", netlist_file_path, "': ", error_message);
  }
  if (!LVSSnapshotIO::read(def_file_path, LVSSnapshotType::kPhysical, physical_netlist, error_message)) {
    LVSLOG.error(Loc::current(), "Failed to read physical iLVS snapshot '", def_file_path, "': ", error_message);
  }
  if (expected_netlist.design_name.empty() || physical_netlist.design_name.empty()) {
    LVSLOG.error(Loc::current(), "iLVS snapshots must both contain a design name!");
  }
  if (expected_netlist.design_name != physical_netlist.design_name) {
    LVSLOG.error(Loc::current(), "iLVS snapshot design names differ: logical='", expected_netlist.design_name, "' physical='",
                 physical_netlist.design_name, "'!");
  }

  LVSDatabase& database = LVSDM.getDatabase();
  database.setExpectedNetlist(std::move(expected_netlist));
  database.setPhysicalNetlist(std::move(physical_netlist));
  LVSLOG.info(Loc::current(), "Loaded iLVS snapshots: logical_nets=", database.getExpectedNetlist().net_map.size(), " physical_nets=",
              database.getPhysicalNetlist().net_map.size(), " physical_graph_nodes=", database.getPhysicalNetlist().physical_graph.node_num, ".");

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
