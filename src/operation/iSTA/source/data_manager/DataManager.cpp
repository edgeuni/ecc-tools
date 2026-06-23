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
#include "idm.h"
#include "liberty/Lib.hh"

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
  // **********     SdcReader     ********** //
  _config.sr_temp_directory_path = _config.temp_directory_path + "sdc_reader/";
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
  // **********     SdcReader     ********** //
  STAUTIL.createDir(_config.sr_temp_directory_path);
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
  buildTimingLibrary();
  buildInstanceList();
  buildNetList();
  buildInstanceTimingInfo();
}

void DataManager::buildTimingLibrary()
{
  std::vector<std::unique_ptr<idb::LibLibrary>> lib_list;
  for (idb::LibertyReader& liberty_reader : dmInst->get_lib_readers()) {
    liberty_reader.linkLib();
    idb::LibBuilder* lib_builder = liberty_reader.get_library_builder();
    lib_list.push_back(lib_builder->takeLib());
    delete lib_builder;
    liberty_reader.set_library_builder(nullptr);
  }
  buildTimingCellMap(lib_list);
}

void DataManager::buildTimingCellMap(std::vector<std::unique_ptr<idb::LibLibrary>>& lib_list)
{
  _database.get_timing_library().get_cell_map().clear();
  for (std::unique_ptr<idb::LibLibrary>& lib : lib_list) {
    for (std::unique_ptr<idb::LibCell>& lib_cell : lib->get_cells()) {
      makeTimingCell(lib_cell.get());
    }
  }
}

void DataManager::makeTimingCell(idb::LibCell* lib_cell)
{
  TimingCell timing_cell;
  timing_cell.set_cell_name(lib_cell->get_cell_name());
  timing_cell.set_is_sequential(lib_cell->isSequentialCell());

  for (std::unique_ptr<idb::LibPort>& lib_port : lib_cell->get_cell_ports()) {
    makeTimingCellPort(timing_cell, lib_port.get());
  }

  for (std::unique_ptr<idb::LibArcSet>& lib_arc_set : lib_cell->get_cell_arcs()) {
    makeTimingCellArc(timing_cell, lib_arc_set.get());
  }

  updateTimingCell(timing_cell);
  _database.get_timing_library().get_cell_map()[timing_cell.get_cell_name()] = timing_cell;
}

void DataManager::makeTimingCellPort(TimingCell& timing_cell, idb::LibPort* lib_port)
{
  TimingCellPort timing_cell_port;
  timing_cell_port.set_port_name(lib_port->get_port_name());
  timing_cell_port.set_capacitance(lib_port->get_port_cap());
  timing_cell_port.set_is_input(lib_port->isInput());
  timing_cell_port.set_is_output(lib_port->isOutput());
  timing_cell_port.set_is_clock(lib_port->isClock() || lib_port->get_is_clock_pin() || lib_port->get_is_clock());
  timing_cell.get_port_map()[timing_cell_port.get_port_name()] = timing_cell_port;
}

void DataManager::makeTimingCellArc(TimingCell& timing_cell, idb::LibArcSet* lib_arc_set)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  if (lib_arc->isDelayArc()) {
    timing_cell.get_cell_arc_list().push_back(makeDelayArc(lib_arc));
  } else if (lib_arc->isSetupArc()) {
    timing_cell.get_setup_arc_list().push_back(makeSetupArc(lib_arc));
  }
}

TimingCellArc DataManager::makeDelayArc(idb::LibArc* lib_arc)
{
  TimingCellArc timing_cell_arc;
  timing_cell_arc.set_source_port(lib_arc->get_src_port());
  timing_cell_arc.set_sink_port(lib_arc->get_snk_port());
  timing_cell_arc.set_delay(lib_arc->getDelayOrConstrainCheckNs(idb::TransType::kRise, 0.0, 0.0));
  timing_cell_arc.set_is_clock_arc(lib_arc->isRisingTriggerArc() || lib_arc->isFallingTriggerArc());
  return timing_cell_arc;
}

TimingCheckArc DataManager::makeSetupArc(idb::LibArc* lib_arc)
{
  TimingCheckArc timing_check_arc;
  timing_check_arc.set_clock_port(lib_arc->get_src_port());
  timing_check_arc.set_data_port(lib_arc->get_snk_port());
  timing_check_arc.set_setup_time(lib_arc->getDelayOrConstrainCheckNs(idb::TransType::kRise, 0.0, 0.0));
  return timing_check_arc;
}

void DataManager::updateTimingCell(TimingCell& timing_cell)
{
  if (!timing_cell.get_setup_arc_list().empty()) {
    timing_cell.set_is_sequential(true);
  }
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

void DataManager::buildInstanceTimingInfo()
{
  for (auto& instance_pair : _database.get_instance_map()) {
    makeInstanceTimingInfo(instance_pair.second);
  }
}

void DataManager::makeInstanceTimingInfo(Instance& instance)
{
  auto& timing_cell_map = _database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return;
  }

  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  instance.set_is_sequential(timing_cell.get_is_sequential());
  TimingCellArc* clock_to_q_arc = findClockToQArc(timing_cell);
  if (clock_to_q_arc != nullptr) {
    instance.set_output_pin_name(getInstancePinName(instance, clock_to_q_arc->get_sink_port()));
    instance.set_clock_to_q_delay(clock_to_q_arc->get_delay());
  } else {
    instance.set_output_pin_name(findOutputPinName(instance, timing_cell));
  }
  if (timing_cell.get_setup_arc_list().empty()) {
    return;
  }

  TimingCheckArc& setup_arc = timing_cell.get_setup_arc_list().front();
  instance.set_clock_pin_name(getInstancePinName(instance, setup_arc.get_clock_port()));
  instance.set_data_pin_name(getInstancePinName(instance, setup_arc.get_data_port()));
  instance.set_setup_time(setup_arc.get_setup_time());
}

TimingCellArc* DataManager::findClockToQArc(TimingCell& timing_cell)
{
  for (TimingCellArc& timing_cell_arc : timing_cell.get_cell_arc_list()) {
    if (timing_cell_arc.get_is_clock_arc()) {
      return &timing_cell_arc;
    }
  }
  return nullptr;
}

std::string DataManager::getInstancePinName(Instance& instance, std::string& port_name)
{
  return instance.get_instance_name() + ":" + port_name;
}

std::string DataManager::findOutputPinName(Instance& instance, TimingCell& timing_cell)
{
  for (auto& [port_name, timing_cell_port] : timing_cell.get_port_map()) {
    if (timing_cell_port.get_is_output() && !timing_cell_port.get_is_clock()) {
      return getInstancePinName(instance, timing_cell_port.get_port_name());
    }
  }
  return "";
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
  // **********     SdcReader     ********** //
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(1), "SdcReader");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(2), "sr_temp_directory_path");
  STALOG.info(Loc::current(), STAUTIL.getSpaceByTabNum(3), _config.sr_temp_directory_path);
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
