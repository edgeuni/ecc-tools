#include "DataManager.hpp"

#include "NetlistExtractor.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "idm.h"

namespace ilvs {

// public

void DataManager::initInst()
{
  if (_dm_instance == nullptr) {
    _dm_instance = new DataManager();
  }
}

DataManager& DataManager::getInst()
{
  if (_dm_instance == nullptr) {
    LVSLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_dm_instance;
}

void DataManager::destroyInst()
{
  if (_dm_instance != nullptr) {
    delete _dm_instance;
    _dm_instance = nullptr;
  }
}

// function

void DataManager::input(std::map<std::string, std::any>& config_map)
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  const auto get_path = [&config_map](const std::string& option_name, bool required) {
    auto config_iter = config_map.find(option_name);
    if (config_iter == config_map.end()) {
      if (required) {
        LVSLOG.error(Loc::current(), "Missing required option '", option_name, "'!");
      }
      return std::string();
    }
    return std::any_cast<std::string>(config_iter->second);
  };
  const std::string netlist_path = get_path("-netlist", true);
  const std::string def_path = get_path("-def", true);
  const std::string top_module = get_path("-top_module", false);
  const std::string report_directory_path = get_path("-report_directory_path", false);

  _database.reset();
  if (!report_directory_path.empty()) {
    _database.setReportDirectoryPath(report_directory_path);
  }
  if (!dmInst->readVerilog(netlist_path, top_module)) {
    LVSLOG.error(Loc::current(), "Failed to read Verilog netlist '", netlist_path, "' through IDB!");
  }
  _database.getExpectedNetlist() = NetlistExtractor::extract(dmInst->get_idb_design());

  if (!dmInst->readDef(def_path)) {
    LVSLOG.error(Loc::current(), "Failed to read DEF '", def_path, "' through IDB!");
  }
  _database.getPhysicalNetlist() = NetlistExtractor::extract(dmInst->get_idb_design());

  LVSLOG.info(Loc::current(), "Loaded IDB snapshots: expected nets = ", _database.getExpectedNetlist().net_map.size(), ", physical nets = ",
              _database.getPhysicalNetlist().net_map.size(), ".");
  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void DataManager::output()
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");
  _database.reset();
  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

DataManager* DataManager::_dm_instance = nullptr;

}  // namespace ilvs
