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
  // **********    DataManager    ********** //
  _config.dm_temp_directory_path = _config.temp_directory_path + "data_manager/";
  // **********   GraphBuilder    ********** //
  _config.gb_temp_directory_path = _config.temp_directory_path + "graph_builder/";
  // *********  GraphPropagator   ********* //
  _config.gp_temp_directory_path = _config.temp_directory_path + "graph_propagator/";
  // **********  TimingAnalyzer   ********** //
  _config.ta_temp_directory_path = _config.temp_directory_path + "timing_analyzer/";
  /////////////////////////////////////////////
  // **********        STA        ********** //
  STAUTIL.removeDir(_config.temp_directory_path);
  STAUTIL.createDir(_config.temp_directory_path);
  STAUTIL.createDirByFile(_config.log_file_path);
  // **********    DataManager    ********** //
  STAUTIL.createDir(_config.dm_temp_directory_path);
  // **********   GraphBuilder    ********** //
  STAUTIL.createDir(_config.gb_temp_directory_path);
  // *********  GraphPropagator   ********* //
  STAUTIL.createDir(_config.gp_temp_directory_path);
  // **********  TimingAnalyzer   ********** //
  STAUTIL.createDir(_config.ta_temp_directory_path);
  /////////////////////////////////////////////
  STALOG.openLogFileStream(_config.log_file_path);
}

void DataManager::buildDatabase()
{
  buildInstanceList(_database);
  buildNetList(_database);
  buildSummary(_database);
}

void DataManager::buildInstanceList(Database& database)
{
  makeInstanceList(database);
  checkInstanceList(database);
}

void DataManager::makeInstanceList(Database& database)
{
  for (auto& instance_pair : database.get_instance_map()) {
    instance_pair.second.get_pin_name_list().clear();
  }

  for (const auto& [pin_name, pin] : database.get_pin_map()) {
    if (pin.get_instance_name().empty()) {
      continue;
    }

    auto instance_iter = database.get_instance_map().find(pin.get_instance_name());
    if (instance_iter != database.get_instance_map().end()) {
      makeUniqueName(instance_iter->second.get_pin_name_list(), pin_name);
    }
  }
}

void DataManager::makeUniqueName(std::vector<std::string>& list, const std::string& value)
{
  if (value.empty()) {
    return;
  }
  if (std::find(list.begin(), list.end(), value) == list.end()) {
    list.push_back(value);
  }
}

void DataManager::checkInstanceList(Database& database)
{
  for (const auto& [pin_name, pin] : database.get_pin_map()) {
    if (pin.get_instance_name().empty()) {
      continue;
    }
    if (database.get_instance_map().find(pin.get_instance_name()) == database.get_instance_map().end()) {
      STALOG.error(Loc::current(), "The instance '", pin.get_instance_name(), "' of pin '", pin_name, "' is not found!");
    }
  }
  for (const auto& [instance_name, instance] : database.get_instance_map()) {
    for (const std::string& pin_name : instance.get_pin_name_list()) {
      if (database.get_pin_map().find(pin_name) == database.get_pin_map().end()) {
        STALOG.error(Loc::current(), "The pin '", pin_name, "' of instance '", instance_name, "' is not found!");
      }
    }
  }
}

void DataManager::buildNetList(Database& database)
{
  makeNetList(database);
  checkNetList(database);
}

void DataManager::makeNetList(Database& database)
{
  for (auto& pin_pair : database.get_pin_map()) {
    pin_pair.second.get_net_name().clear();
  }

  for (auto& [net_name, net] : database.get_net_map()) {
    makeNet(database, net_name, net);
  }
}

void DataManager::makeNet(Database& database, const std::string& net_name, Net& net)
{
  net.get_driver_pin().clear();
  net.get_load_pin_list().clear();

  for (const std::string& pin_name : net.get_pin_name_list()) {
    auto pin_iter = database.get_pin_map().find(pin_name);
    if (pin_iter == database.get_pin_map().end()) {
      continue;
    }

    Pin& pin = pin_iter->second;
    pin.set_net_name(net_name);
    if (net.get_driver_pin().empty() && isDriverPin(pin)) {
      net.set_driver_pin(pin_name);
    }
  }

  if (net.get_driver_pin().empty() && !net.get_pin_name_list().empty()) {
    net.set_driver_pin(net.get_pin_name_list().front());
  }

  for (const std::string& pin_name : net.get_pin_name_list()) {
    if (pin_name != net.get_driver_pin()) {
      makeUniqueName(net.get_load_pin_list(), pin_name);
    }
  }
}

bool DataManager::isDriverPin(const Pin& pin)
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

void DataManager::checkNetList(Database& database)
{
  for (const auto& [net_name, net] : database.get_net_map()) {
    checkNet(database, net_name, net);
  }
}

void DataManager::checkNet(Database& database, const std::string& net_name, const Net& net)
{
  if (!net.get_driver_pin().empty() && database.get_pin_map().find(net.get_driver_pin()) == database.get_pin_map().end()) {
    STALOG.error(Loc::current(), "The driver pin '", net.get_driver_pin(), "' of net '", net_name, "' is not found!");
  }
  for (const std::string& pin_name : net.get_pin_name_list()) {
    auto pin_iter = database.get_pin_map().find(pin_name);
    if (pin_iter == database.get_pin_map().end()) {
      STALOG.error(Loc::current(), "The pin '", pin_name, "' of net '", net_name, "' is not found!");
    }
    if (pin_iter->second.get_net_name() != net_name) {
      STALOG.error(Loc::current(), "The net name of pin '", pin_name, "' is not '", net_name, "'!");
    }
  }
  for (const std::string& pin_name : net.get_load_pin_list()) {
    if (pin_name == net.get_driver_pin()) {
      STALOG.error(Loc::current(), "The driver pin '", pin_name, "' of net '", net_name, "' appears in load pin list!");
    }
    if (database.get_pin_map().find(pin_name) == database.get_pin_map().end()) {
      STALOG.error(Loc::current(), "The load pin '", pin_name, "' of net '", net_name, "' is not found!");
    }
  }
}

void DataManager::buildSummary(Database& database)
{
  makeSummary(database);
}

void DataManager::makeSummary(Database& database)
{
  Summary& summary = database.get_summary();
  summary.set_instance_num(database.get_instance_map().size());
  summary.set_pin_num(database.get_pin_map().size());
  summary.set_net_num(database.get_net_map().size());
  summary.set_arc_num(database.get_arc_list().size());
  summary.set_startpoint_num(database.get_startpoint_list().size());
  summary.set_endpoint_num(database.get_endpoint_list().size());
  summary.set_port_num(0);
  for (const auto& pin_pair : database.get_pin_map()) {
    if (pin_pair.second.get_is_port()) {
      summary.set_port_num(summary.get_port_num() + 1);
    }
  }
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
  // *********  GraphPropagator   ********* //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "GraphPropagator");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "gp_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.gp_temp_directory_path);
  // **********  TimingAnalyzer   ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "TimingAnalyzer");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "ta_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.ta_temp_directory_path);
  /////////////////////////////////////////////
}

void DataManager::printDatabase()
{
  const Summary& summary = _database.get_summary();
  /////////////////////////////////////////////
  // **********        STA        ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(0), "STA_DATABASE");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "design_name");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), _database.get_design_name());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "instance_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_instance_num());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "port_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_port_num());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "pin_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_pin_num());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "net_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_net_num());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "arc_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_arc_num());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "startpoint_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_startpoint_num());
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "endpoint_num");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), summary.get_endpoint_num());
  /////////////////////////////////////////////
}

#endif

}  // namespace ista
