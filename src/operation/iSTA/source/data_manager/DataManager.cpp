// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "DataManager.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"
#include "STAInterface.hpp"
#include "Utility.hpp"

namespace ista {

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
    STALOG.error(Loc::current(), "The instance not initialized!");
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
  STALOG.info(Loc::current(), "Starting...");
  STAI.input(config_map);
  buildConfig();
  printConfig();
  printDatabase();
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void DataManager::output()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");
  STAI.output();
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

DataManager* DataManager::_dm_instance = nullptr;

#if 1  // build

void DataManager::buildConfig()
{
  /////////////////////////////////////////////
  // **********        STA        ********** //
  _config.temp_directory_path = std::filesystem::absolute(_config.temp_directory_path);
  _config.temp_directory_path += "/";
  _config.log_file_path = _config.temp_directory_path + "sta.log";
  _config.path_report_number = 1000;
  // **********    DataManager    ********** //
  _config.dm_temp_directory_path = _config.temp_directory_path + "data_manager/";
  // **********   DesignLoader    ********** //
  _config.dl_temp_directory_path = _config.temp_directory_path + "design_loader/";
  // ********* ConstraintManager  ********** //
  _config.cm_temp_directory_path = _config.temp_directory_path + "constraint_manager/";
  // **********   GraphBuilder    ********** //
  _config.gb_temp_directory_path = _config.temp_directory_path + "graph_builder/";
  // ********** DelayCalculator   ********** //
  _config.dc_temp_directory_path = _config.temp_directory_path + "delay_calculator/";
  // ********* TimingPropagator   ********* //
  _config.tp_temp_directory_path = _config.temp_directory_path + "timing_propagator/";
  // **********  TimingReporter   ********** //
  _config.tr_temp_directory_path = _config.temp_directory_path + "timing_reporter/";
  /////////////////////////////////////////////
  // **********        STA        ********** //
  STAUTIL.removeDir(_config.temp_directory_path);
  STAUTIL.createDir(_config.temp_directory_path);
  STAUTIL.createDirByFile(_config.log_file_path);
  // **********    DataManager    ********** //
  STAUTIL.createDir(_config.dm_temp_directory_path);
  // **********   DesignLoader    ********** //
  STAUTIL.createDir(_config.dl_temp_directory_path);
  // ********* ConstraintManager  ********** //
  STAUTIL.createDir(_config.cm_temp_directory_path);
  // **********   GraphBuilder    ********** //
  STAUTIL.createDir(_config.gb_temp_directory_path);
  // ********** DelayCalculator   ********** //
  STAUTIL.createDir(_config.dc_temp_directory_path);
  // ********* TimingPropagator   ********* //
  STAUTIL.createDir(_config.tp_temp_directory_path);
  // **********  TimingReporter   ********** //
  STAUTIL.createDir(_config.tr_temp_directory_path);
  /////////////////////////////////////////////
  STALOG.openLogFileStream(_config.log_file_path);
}

void DataManager::printConfig()
{
  /////////////////////////////////////////////
  // **********        STA        ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(0), "STA_CONFIG_INPUT");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _config.temp_directory_path);
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "thread_number");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _config.thread_number);
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "path_report_number");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _config.path_report_number);
  // **********        STA        ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(0), "STA_CONFIG_BUILD");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "log_file_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _config.log_file_path);
  // **********    DataManager    ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "DataManager");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "dm_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.dm_temp_directory_path);
  // **********   DesignLoader    ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "DesignLoader");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "dl_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.dl_temp_directory_path);
  // ********* ConstraintManager  ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "ConstraintManager");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "cm_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.cm_temp_directory_path);
  // **********   GraphBuilder    ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "GraphBuilder");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "gb_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.gb_temp_directory_path);
  // ********** DelayCalculator   ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "DelayCalculator");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "dc_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.dc_temp_directory_path);
  // ********* TimingPropagator   ********* //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "TimingPropagator");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "tp_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.tp_temp_directory_path);
  // **********  TimingReporter   ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "TimingReporter");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "tr_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.tr_temp_directory_path);
  /////////////////////////////////////////////
}

void DataManager::printDatabase()
{
  std::size_t port_num = 0;
  for (auto& [pin_name, pin] : _database.get_pin_map()) {
    if (pin.get_is_port()) {
      port_num++;
    }
  }
  /////////////////////////////////////////////
  // **********        STA        ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(0), "STA_DATABASE");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "design_name");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _database.get_design_name());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "instance_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _database.get_instance_map().size());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "port_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), port_num);
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "pin_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _database.get_pin_map().size());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "net_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _database.get_net_map().size());
  /////////////////////////////////////////////
}

#endif

}  // namespace ista
