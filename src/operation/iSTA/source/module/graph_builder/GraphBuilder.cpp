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
#include "GraphBuilder.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ista {

// public

void GraphBuilder::initInst()
{
  if (_gb_instance == nullptr) {
    _gb_instance = new GraphBuilder();
  }
}

GraphBuilder& GraphBuilder::getInst()
{
  if (_gb_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_gb_instance;
}

void GraphBuilder::destroyInst()
{
  if (_gb_instance != nullptr) {
    delete _gb_instance;
    _gb_instance = nullptr;
  }
}

// function

bool GraphBuilder::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  buildTimingPointList(database);
  buildCellArcs(database);
  buildNetArcs(database);
  buildStartEndPointList(database);

  STALOG.info(Loc::current(), "Build iSTA graph: pins=", database.get_pin_map().size(), " arcs=", database.get_arc_list().size(),
              " start_points=", database.get_start_point_list().size(), " end_points=", database.get_end_point_list().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphBuilder* GraphBuilder::_gb_instance = nullptr;

void GraphBuilder::buildTimingPointList(Database& database)
{
  for (auto& pin_pair : database.get_pin_map()) {
    database.get_timing_point_map()[pin_pair.first] = TimingPoint();
  }
}

void GraphBuilder::buildCellArcs(Database& database)
{
  for (auto& [instance_name, instance] : database.get_instance_map()) {
    if (buildLibraryCellArcs(database, instance)) {
      continue;
    }
    std::vector<std::string> input_pin_list = collectInputPins(database, instance);
    std::vector<std::string> output_pin_list = collectOutputPins(database, instance);
    if (input_pin_list.empty() || output_pin_list.empty()) {
      continue;
    }
    for (std::string& input_pin : input_pin_list) {
      for (std::string& output_pin : output_pin_list) {
        if (input_pin == output_pin) {
          continue;
        }
        addArc(database, input_pin, output_pin, ArcType::kCell, instance_name);
      }
    }
  }
}

bool GraphBuilder::buildLibraryCellArcs(Database& database, Instance& instance)
{
  auto& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return false;
  }

  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  if (timing_cell.get_cell_arc_list().empty()) {
    return false;
  }

  for (TimingCellArc& timing_cell_arc : timing_cell.get_cell_arc_list()) {
    if (timing_cell_arc.get_is_clock_arc()) {
      continue;
    }
    addCellArc(database, instance, timing_cell_arc);
  }
  return true;
}

std::string GraphBuilder::getInstancePinName(Instance& instance, std::string& port_name)
{
  return instance.get_instance_name() + ":" + port_name;
}

void GraphBuilder::addCellArc(Database& database, Instance& instance, TimingCellArc& timing_cell_arc)
{
  std::string source_pin = getInstancePinName(instance, timing_cell_arc.get_source_port());
  std::string sink_pin = getInstancePinName(instance, timing_cell_arc.get_sink_port());
  if (database.get_pin_map().count(source_pin) == 0 || database.get_pin_map().count(sink_pin) == 0) {
    return;
  }
  addArc(database, source_pin, sink_pin, ArcType::kCell, instance.get_instance_name(), timing_cell_arc.get_source_port(),
         timing_cell_arc.get_sink_port(), timing_cell_arc.get_is_clock_arc());
}

void GraphBuilder::addArc(Database& database, const std::string& source_pin, const std::string& sink_pin, ArcType type,
                          const std::string& owner_name, const std::string& library_source_port,
                          const std::string& library_sink_port, bool is_clock_arc)
{
  Arc arc;
  arc.set_arc_name(owner_name + ":" + source_pin + "->" + sink_pin);
  arc.set_source_pin(source_pin);
  arc.set_sink_pin(sink_pin);
  arc.set_owner_name(owner_name);
  arc.set_library_source_port(library_source_port);
  arc.set_library_sink_port(library_sink_port);
  arc.set_type(type);
  arc.set_is_clock_arc(is_clock_arc);

  database.get_arc_list().push_back(arc);
  const std::size_t arc_idx = database.get_arc_list().size() - 1;
  database.get_outgoing_arc_list_map()[source_pin].push_back(arc_idx);
  database.get_incoming_arc_list_map()[sink_pin].push_back(arc_idx);
}

std::vector<std::string> GraphBuilder::collectInputPins(Database& database, Instance& instance)
{
  std::vector<std::string> input_pin_list;
  for (std::string& pin_name : instance.get_pin_name_list()) {
    if (isInputLike(database.get_pin_map()[pin_name].get_direction())) {
      input_pin_list.push_back(pin_name);
    }
  }
  return input_pin_list;
}

bool GraphBuilder::isInputLike(PinDirection direction)
{
  return direction == PinDirection::kInput || direction == PinDirection::kInout;
}

std::vector<std::string> GraphBuilder::collectOutputPins(Database& database, Instance& instance)
{
  std::vector<std::string> output_pin_list;
  for (std::string& pin_name : instance.get_pin_name_list()) {
    if (isOutputLike(database.get_pin_map()[pin_name].get_direction())) {
      output_pin_list.push_back(pin_name);
    }
  }
  return output_pin_list;
}

bool GraphBuilder::isOutputLike(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

void GraphBuilder::buildNetArcs(Database& database)
{
  for (auto& [net_name, net] : database.get_net_map()) {
    if (net.get_driver_pin().empty()) {
      continue;
    }
    for (std::string& load_pin : net.get_load_pin_list()) {
      if (load_pin == net.get_driver_pin()) {
        continue;
      }
      addArc(database, net.get_driver_pin(), load_pin, ArcType::kNet, net_name);
    }
  }
}

void GraphBuilder::addArc(Database& database, const std::string& source_pin, const std::string& sink_pin, ArcType type,
                          const std::string& owner_name)
{
  Arc arc;
  arc.set_arc_name(owner_name + ":" + source_pin + "->" + sink_pin);
  arc.set_source_pin(source_pin);
  arc.set_sink_pin(sink_pin);
  arc.set_owner_name(owner_name);
  arc.set_type(type);

  database.get_arc_list().push_back(arc);
  const std::size_t arc_idx = database.get_arc_list().size() - 1;
  database.get_outgoing_arc_list_map()[source_pin].push_back(arc_idx);
  database.get_incoming_arc_list_map()[sink_pin].push_back(arc_idx);
}

void GraphBuilder::buildStartEndPointList(Database& database)
{
  for (auto& [pin_name, pin] : database.get_pin_map()) {
    if (isStartPoint(database, pin_name, pin)) {
      appendUnique(database.get_start_point_list(), pin_name);
    }
    if (isEndPoint(database, pin_name, pin)) {
      appendUnique(database.get_end_point_list(), pin_name);
    }
  }
}

bool GraphBuilder::isStartPoint(Database& database, const std::string& pin_name, Pin& pin)
{
  if (isClockPin(database, pin_name, pin) || isClockSource(database, pin_name)) {
    return false;
  }
  return !hasIncomingArc(database, pin_name) || isStartPort(pin);
}

bool GraphBuilder::isClockPin(Database& database, const std::string& pin_name, Pin& pin)
{
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  return pin_name == database.get_instance_map()[pin.get_instance_name()].get_clock_pin_name();
}

bool GraphBuilder::isClockSource(Database& database, const std::string& pin_name)
{
  for (auto& [clock_name, timing_clock] : database.get_timing_constraint().get_clock_map()) {
    if (STAUTIL.exist(timing_clock.get_source_list(), pin_name)) {
      return true;
    }
  }
  return false;
}

bool GraphBuilder::hasIncomingArc(Database& database, const std::string& pin_name)
{
  return database.get_incoming_arc_list_map().count(pin_name) > 0 && !database.get_incoming_arc_list_map()[pin_name].empty();
}

bool GraphBuilder::isStartPort(Pin& pin)
{
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout);
}

bool GraphBuilder::isEndPoint(Database& database, const std::string& pin_name, Pin& pin)
{
  if (isClockPin(database, pin_name, pin) || isClockSource(database, pin_name)) {
    return false;
  }
  return !hasOutgoingArc(database, pin_name) || isEndPort(pin);
}

bool GraphBuilder::hasOutgoingArc(Database& database, const std::string& pin_name)
{
  return database.get_outgoing_arc_list_map().count(pin_name) > 0 && !database.get_outgoing_arc_list_map()[pin_name].empty();
}

bool GraphBuilder::isEndPort(Pin& pin)
{
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kOutput || pin.get_direction() == PinDirection::kInout);
}

void GraphBuilder::appendUnique(std::vector<std::string>& list, const std::string& value)
{
  if (!STAUTIL.exist(list, value)) {
    list.push_back(value);
  }
}

}  // namespace ista
