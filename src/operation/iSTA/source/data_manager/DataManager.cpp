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
  buildDatabase();
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
  // **********   GraphBuilder    ********** //
  _config.gb_temp_directory_path = _config.temp_directory_path + "graph_builder/";
  // ********** DelayCalculator   ********** //
  _config.dc_temp_directory_path = _config.temp_directory_path + "delay_calculator/";
  // **********  GraphLevelizer   ********** //
  _config.gl_temp_directory_path = _config.temp_directory_path + "graph_levelizer/";
  // *********  GraphPropagator   ********* //
  _config.gp_temp_directory_path = _config.temp_directory_path + "graph_propagator/";
  // **********  TimingAnalyzer   ********** //
  _config.ta_temp_directory_path = _config.temp_directory_path + "timing_analyzer/";
  // **********  TimingReporter   ********** //
  _config.tr_temp_directory_path = _config.temp_directory_path + "timing_reporter/";
  /////////////////////////////////////////////
  // **********        STA        ********** //
  STAUTIL.removeDir(_config.temp_directory_path);
  STAUTIL.createDir(_config.temp_directory_path);
  STAUTIL.createDirByFile(_config.log_file_path);
  // **********    DataManager    ********** //
  STAUTIL.createDir(_config.dm_temp_directory_path);
  // **********   GraphBuilder    ********** //
  STAUTIL.createDir(_config.gb_temp_directory_path);
  // ********** DelayCalculator   ********** //
  STAUTIL.createDir(_config.dc_temp_directory_path);
  // **********  GraphLevelizer   ********** //
  STAUTIL.createDir(_config.gl_temp_directory_path);
  // *********  GraphPropagator   ********* //
  STAUTIL.createDir(_config.gp_temp_directory_path);
  // **********  TimingAnalyzer   ********** //
  STAUTIL.createDir(_config.ta_temp_directory_path);
  // **********  TimingReporter   ********** //
  STAUTIL.createDir(_config.tr_temp_directory_path);
  /////////////////////////////////////////////
  STALOG.openLogFileStream(_config.log_file_path);
}

void DataManager::buildDatabase()
{
  buildInstanceList();
  buildNetList();
}

void DataManager::buildInstanceList()
{
  makeInstanceList();
}

void DataManager::makeInstanceList()
{
  for (auto& instance_pair : _database.get_instance_map()) {
    instance_pair.second.get_pin_name_list().clear();
  }

  for (auto& pin_pair : _database.get_pin_map()) {
    Pin& pin = pin_pair.second;
    if (!isInstancePin(pin)) {
      continue;
    }

    makeUniqueName(_database.get_instance_map()[pin.get_instance_name()].get_pin_name_list(), pin_pair.first);
  }
}

bool DataManager::isInstancePin(Pin& pin)
{
  return !pin.get_is_port();
}

void DataManager::makeUniqueName(std::vector<std::string>& list, const std::string& value)
{
  if (!STAUTIL.exist(list, value)) {
    list.push_back(value);
  }
}

void DataManager::buildNetList()
{
  makeNetList();
}

void DataManager::makeNetList()
{
  for (auto& pin_pair : _database.get_pin_map()) {
    pin_pair.second.get_net_name().clear();
  }

  for (auto& net_pair : _database.get_net_map()) {
    makeNet(net_pair.first, net_pair.second);
  }
}

void DataManager::makeNet(const std::string& net_name, Net& net)
{
  net.get_driver_pin().clear();
  net.get_load_pin_list().clear();

  for (std::string& pin_name : net.get_pin_name_list()) {
    Pin& pin = _database.get_pin_map()[pin_name];
    pin.set_net_name(net_name);
    if (net.get_driver_pin().empty() && isDriverPin(pin)) {
      net.set_driver_pin(pin_name);
    }
  }

  for (std::string& pin_name : net.get_pin_name_list()) {
    if (pin_name != net.get_driver_pin()) {
      makeUniqueName(net.get_load_pin_list(), pin_name);
    }
  }
}

bool DataManager::isDriverPin(Pin& pin)
{
  if (pin.get_is_port()) {
    return pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout;
  }
  return isOutputLikeDirection(pin.get_direction());
}

bool DataManager::isOutputLikeDirection(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
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
  // **********   GraphBuilder    ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "GraphBuilder");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "gb_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.gb_temp_directory_path);
  // ********** DelayCalculator   ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "DelayCalculator");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "dc_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.dc_temp_directory_path);
  // **********  GraphLevelizer   ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "GraphLevelizer");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "gl_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.gl_temp_directory_path);
  // *********  GraphPropagator   ********* //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "GraphPropagator");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "gp_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.gp_temp_directory_path);
  // **********  TimingAnalyzer   ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "TimingAnalyzer");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "ta_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.ta_temp_directory_path);
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
