#include "DataManager.hpp"

#include "LVSInterface.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

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

  LVSI.input(config_map);
  buildConfig();
  buildDatabase();
  printConfig();
  printDatabase();
  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void DataManager::output()
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");
  LVSI.output();
  _database.reset();
  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

DataManager* DataManager::_dm_instance = nullptr;

#if 1  // build

void DataManager::buildConfig()
{
  /////////////////////////////////////////////
  // **********       LVS        ********** //
  _config.temp_directory_path = std::filesystem::absolute(_config.temp_directory_path);
  _config.temp_directory_path += "/";
  _config.log_file_path = _config.temp_directory_path + "lvs.log";
  // **********   DataManager    ********** //
  _config.dm_temp_directory_path = _config.temp_directory_path + "data_manager/";
  // ******** NetlistExtractor   ********** //
  _config.ne_temp_directory_path = _config.temp_directory_path + "netlist_extractor/";
  // **********   LVSChecker     ********** //
  _config.lc_temp_directory_path = _config.temp_directory_path + "lvs_checker/";
  // **********   LVSReporter    ********** //
  _config.lr_temp_directory_path = _config.temp_directory_path + "lvs_reporter/";
  /////////////////////////////////////////////
  // **********       LVS        ********** //
  LVSUTIL.removeDir(_config.temp_directory_path);
  LVSUTIL.createDir(_config.temp_directory_path);
  LVSUTIL.createDirByFile(_config.log_file_path);
  // **********   DataManager    ********** //
  LVSUTIL.createDir(_config.dm_temp_directory_path);
  // ******** NetlistExtractor   ********** //
  LVSUTIL.createDir(_config.ne_temp_directory_path);
  // **********   LVSChecker     ********** //
  LVSUTIL.createDir(_config.lc_temp_directory_path);
  // **********   LVSReporter    ********** //
  LVSUTIL.createDir(_config.lr_temp_directory_path);
  /////////////////////////////////////////////
  _database.setReportDirectoryPath(_config.lr_temp_directory_path);
  LVSLOG.openLogFileStream(_config.log_file_path);
}

void DataManager::buildDatabase()
{
  _database.reset();
}

void DataManager::printConfig()
{
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(0), "LVS_CONFIG_INPUT");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "temp_directory_path");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.temp_directory_path);
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "thread_number");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.thread_number);
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(0), "LVS_CONFIG_BUILD");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "log_file_path");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.log_file_path);
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "DataManager");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.dm_temp_directory_path);
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "NetlistExtractor");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.ne_temp_directory_path);
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "LVSChecker");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.lc_temp_directory_path);
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "LVSReporter");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _config.lr_temp_directory_path);
}

void DataManager::printDatabase()
{
  LVSLOG.info(Loc::current(), "LVS_DATABASE");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "expected_net_num");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _database.getExpectedNetlist().net_map.size());
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(1), "physical_net_num");
  LVSLOG.info(Loc::current(), LVSUTIL.getSpaceByTabNum(2), _database.getPhysicalNetlist().net_map.size());
}

#endif

}  // namespace ilvs
