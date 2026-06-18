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

namespace ista {

// public

void DataManager::initInst()
{
  if (_data_manager_instance == nullptr) {
    _data_manager_instance = new DataManager();
  }
}

DataManager& DataManager::getInst()
{
  if (_data_manager_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_data_manager_instance;
}

void DataManager::destroyInst()
{
  if (_data_manager_instance != nullptr) {
    delete _data_manager_instance;
    _data_manager_instance = nullptr;
  }
}

// function

void DataManager::input(std::map<std::string, std::any>& config_map)
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  STAI.input(config_map);
  buildConfig(config_map);
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

void DataManager::reset(Database& database)
{
  database.get_design_name().clear();
  database.get_report_directory().clear();
  database.get_instance_map().clear();
  database.get_pin_map().clear();
  database.get_net_map().clear();
  clearGraph(database);
}

void DataManager::clearGraph(Database& database)
{
  database.get_arc_list().clear();
  database.get_outgoing_arc_list_map().clear();
  database.get_incoming_arc_list_map().clear();
  database.get_startpoint_list().clear();
  database.get_endpoint_list().clear();
  clearTiming(database);
}

void DataManager::clearTiming(Database& database)
{
  database.get_timing_point_map().clear();
  database.set_summary(Summary());
}

// private

DataManager* DataManager::_data_manager_instance = nullptr;

#if 1  // build

void DataManager::buildConfig(std::map<std::string, std::any>& config_map)
{
  _config = Config();
  _config.set_option_num(config_map.size());

  const std::array<std::string, 4> report_directory_key_list = {"-report_directory", "-report_directory_path", "report_directory",
                                                               "report_directory_path"};
  for (const std::string& key : report_directory_key_list) {
    auto report_directory_iter = config_map.find(key);
    if (report_directory_iter != config_map.end() && report_directory_iter->second.type() == typeid(std::string)) {
      _config.set_report_directory(std::any_cast<std::string>(report_directory_iter->second));
      break;
    }
  }

  _database.set_report_directory(_config.get_report_directory());
}

void DataManager::buildDatabase()
{
  buildInstanceList(_database);
  buildNetList(_database);
  buildSummary(_database);
}

void DataManager::buildInstanceList(Database& database)
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
      buildUniqueName(instance_iter->second.get_pin_name_list(), pin_name);
    }
  }
}

void DataManager::buildUniqueName(std::vector<std::string>& list, const std::string& value)
{
  if (value.empty()) {
    return;
  }
  if (std::find(list.begin(), list.end(), value) == list.end()) {
    list.push_back(value);
  }
}

void DataManager::buildNetList(Database& database)
{
  for (auto& pin_pair : database.get_pin_map()) {
    pin_pair.second.get_net_name().clear();
  }

  for (auto& [net_name, net] : database.get_net_map()) {
    buildNet(database, net_name, net);
  }
}

void DataManager::buildNet(Database& database, const std::string& net_name, Net& net)
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
    if (net.get_driver_pin().empty() && buildDriverPin(pin)) {
      net.set_driver_pin(pin_name);
    }
  }

  if (net.get_driver_pin().empty() && !net.get_pin_name_list().empty()) {
    net.set_driver_pin(net.get_pin_name_list().front());
  }

  for (const std::string& pin_name : net.get_pin_name_list()) {
    if (pin_name != net.get_driver_pin()) {
      buildUniqueName(net.get_load_pin_list(), pin_name);
    }
  }
}

bool DataManager::buildDriverPin(const Pin& pin)
{
  if (pin.get_is_port()) {
    return pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout;
  }
  return buildOutputLikeDirection(pin.get_direction());
}

bool DataManager::buildOutputLikeDirection(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

void DataManager::buildSummary(Database& database)
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
  STALOG.info(Loc::current(), "STA_CONFIG");
  STALOG.info(Loc::current(), "  option_num=", _config.get_option_num());
  STALOG.info(Loc::current(), "  report_directory=", _config.get_report_directory());
}

void DataManager::printDatabase()
{
  const Summary& summary = _database.get_summary();
  STALOG.info(Loc::current(), "STA_DATABASE");
  STALOG.info(Loc::current(), "  design_name=", _database.get_design_name());
  STALOG.info(Loc::current(), "  instance_num=", summary.get_instance_num(), " port_num=", summary.get_port_num(),
              " pin_num=", summary.get_pin_num(), " net_num=", summary.get_net_num());
  STALOG.info(Loc::current(), "  arc_num=", summary.get_arc_num(), " startpoint_num=", summary.get_startpoint_num(),
              " endpoint_num=", summary.get_endpoint_num());
}

#endif

}  // namespace ista
