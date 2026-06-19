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

  if (database.get_pin_map().empty()) {
    STALOG.warn(Loc::current(), "iSTA database has no pin, skip graph build.");
    return false;
  }

  buildNetArcs(database);
  buildCellArcs(database);
  buildEndpoints(database);

  STALOG.info(Loc::current(), "Build iSTA graph: pins=", database.get_pin_map().size(), " arcs=", database.get_arc_list().size(),
              " startpoints=", database.get_startpoint_list().size(), " endpoints=", database.get_endpoint_list().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphBuilder* GraphBuilder::_gb_instance = nullptr;

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
      addArc(database, net.get_driver_pin(), load_pin, ArcType::kNet, net_name,
             estimateNetDelay(database, net.get_driver_pin(), load_pin));
    }
  }
}

void GraphBuilder::addArc(Database& database, const std::string& source_pin, const std::string& sink_pin, ArcType type,
                          const std::string& owner_name, double delay)
{
  if (source_pin.empty() || sink_pin.empty() || database.get_pin_map().count(source_pin) == 0
      || database.get_pin_map().count(sink_pin) == 0) {
    return;
  }

  Arc arc;
  arc.set_name(owner_name + ":" + source_pin + "->" + sink_pin);
  arc.set_source_pin(source_pin);
  arc.set_sink_pin(sink_pin);
  arc.set_owner_name(owner_name);
  arc.set_type(type);
  arc.set_delay(delay);

  database.get_arc_list().push_back(arc);
  const std::size_t arc_idx = database.get_arc_list().size() - 1;
  database.get_outgoing_arc_list_map()[source_pin].push_back(arc_idx);
  database.get_incoming_arc_list_map()[sink_pin].push_back(arc_idx);
}

double GraphBuilder::estimateNetDelay(Database& database, std::string& source_pin, std::string& sink_pin)
{
  auto source_iter = database.get_pin_map().find(source_pin);
  auto sink_iter = database.get_pin_map().find(sink_pin);
  if (source_iter == database.get_pin_map().end() || sink_iter == database.get_pin_map().end()) {
    return 1.0;
  }

  const double distance = std::abs(source_iter->second.get_x() - sink_iter->second.get_x())
                          + std::abs(source_iter->second.get_y() - sink_iter->second.get_y());
  return 1.0 + distance * 0.000001;
}

void GraphBuilder::buildCellArcs(Database& database)
{
  for (auto& [instance_name, instance] : database.get_instance_map()) {
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
        addArc(database, input_pin, output_pin, ArcType::kCell, instance_name, estimateCellDelay(instance.get_cell_name()));
      }
    }
  }
}

std::vector<std::string> GraphBuilder::collectInputPins(Database& database, Instance& instance)
{
  std::vector<std::string> input_pin_list;
  for (std::string& pin_name : instance.get_pin_name_list()) {
    auto pin_iter = database.get_pin_map().find(pin_name);
    if (pin_iter != database.get_pin_map().end() && isInputLike(pin_iter->second.get_direction())) {
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
    auto pin_iter = database.get_pin_map().find(pin_name);
    if (pin_iter != database.get_pin_map().end() && isOutputLike(pin_iter->second.get_direction())) {
      output_pin_list.push_back(pin_name);
    }
  }
  return output_pin_list;
}

bool GraphBuilder::isOutputLike(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

double GraphBuilder::estimateCellDelay(std::string& cell_name)
{
  return cell_name.empty() ? 1.0 : 1.0;
}

void GraphBuilder::buildEndpoints(Database& database)
{
  for (auto& [pin_name, pin] : database.get_pin_map()) {
    database.get_timing_point_map()[pin_name] = TimingPoint();

    const bool has_incoming = database.get_incoming_arc_list_map().count(pin_name) > 0
                              && !database.get_incoming_arc_list_map()[pin_name].empty();
    const bool has_outgoing = database.get_outgoing_arc_list_map().count(pin_name) > 0
                              && !database.get_outgoing_arc_list_map()[pin_name].empty();

    if (!has_incoming
        || (pin.get_is_port() && (pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout))) {
      appendUnique(database.get_startpoint_list(), pin_name);
    }
    if (!has_outgoing
        || (pin.get_is_port() && (pin.get_direction() == PinDirection::kOutput || pin.get_direction() == PinDirection::kInout))) {
      appendUnique(database.get_endpoint_list(), pin_name);
    }
  }
}

void GraphBuilder::appendUnique(std::vector<std::string>& list, const std::string& value)
{
  if (std::find(list.begin(), list.end(), value) == list.end()) {
    list.push_back(value);
  }
}

}  // namespace ista
