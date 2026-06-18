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

#include <algorithm>
#include <cmath>

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

  TimingModel& timing_model = STADM.getTimingModel();
  timing_model.clearGraph();

  if (timing_model.pins.empty()) {
    STALOG.warn(Loc::current(), "iSTA timing model has no pin, skip graph build.");
    return false;
  }

  buildNetArcs(timing_model);
  buildCellArcs(timing_model);
  buildEndpoints(timing_model);

  STALOG.info(Loc::current(), "Build iSTA graph: pins=", timing_model.pins.size(), " arcs=", timing_model.arcs.size(),
              " startpoints=", timing_model.startpoint_list.size(), " endpoints=", timing_model.endpoint_list.size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphBuilder* GraphBuilder::_gb_instance = nullptr;

void GraphBuilder::buildNetArcs(TimingModel& timing_model)
{
  for (const auto& [net_name, net] : timing_model.nets) {
    if (net.driver_pin.empty()) {
      continue;
    }
    for (const std::string& load_pin : net.load_pins) {
      if (load_pin == net.driver_pin) {
        continue;
      }
      addArc(timing_model, net.driver_pin, load_pin, ArcType::kNet, net_name,
             estimateNetDelay(timing_model, net.driver_pin, load_pin));
    }
  }
}

void GraphBuilder::addArc(TimingModel& timing_model, const std::string& source_pin, const std::string& sink_pin, ArcType type,
                          const std::string& owner_name, double delay)
{
  if (source_pin.empty() || sink_pin.empty() || timing_model.pins.count(source_pin) == 0 || timing_model.pins.count(sink_pin) == 0) {
    return;
  }

  Arc arc;
  arc.name = owner_name + ":" + source_pin + "->" + sink_pin;
  arc.source_pin = source_pin;
  arc.sink_pin = sink_pin;
  arc.owner_name = owner_name;
  arc.type = type;
  arc.delay = delay;

  timing_model.arcs.push_back(arc);
  const std::size_t arc_idx = timing_model.arcs.size() - 1;
  timing_model.outgoing_arc_list[source_pin].push_back(arc_idx);
  timing_model.incoming_arc_list[sink_pin].push_back(arc_idx);
}

double GraphBuilder::estimateNetDelay(const TimingModel& timing_model, const std::string& source_pin, const std::string& sink_pin) const
{
  auto source_iter = timing_model.pins.find(source_pin);
  auto sink_iter = timing_model.pins.find(sink_pin);
  if (source_iter == timing_model.pins.end() || sink_iter == timing_model.pins.end()) {
    return 1.0;
  }

  const double distance = std::abs(source_iter->second.x - sink_iter->second.x) + std::abs(source_iter->second.y - sink_iter->second.y);
  return 1.0 + distance * 0.000001;
}

void GraphBuilder::buildCellArcs(TimingModel& timing_model)
{
  for (const auto& [instance_name, instance] : timing_model.instances) {
    const std::vector<std::string> input_pin_list = collectInputPins(timing_model, instance);
    const std::vector<std::string> output_pin_list = collectOutputPins(timing_model, instance);
    if (input_pin_list.empty() || output_pin_list.empty()) {
      continue;
    }
    for (const std::string& input_pin : input_pin_list) {
      for (const std::string& output_pin : output_pin_list) {
        if (input_pin == output_pin) {
          continue;
        }
        addArc(timing_model, input_pin, output_pin, ArcType::kCell, instance_name, estimateCellDelay(instance.cell_name));
      }
    }
  }
}

std::vector<std::string> GraphBuilder::collectInputPins(const TimingModel& timing_model, const Instance& instance) const
{
  std::vector<std::string> input_pin_list;
  for (const std::string& pin_name : instance.pin_names) {
    auto pin_iter = timing_model.pins.find(pin_name);
    if (pin_iter != timing_model.pins.end() && isInputLike(pin_iter->second.direction)) {
      input_pin_list.push_back(pin_name);
    }
  }
  return input_pin_list;
}

bool GraphBuilder::isInputLike(PinDirection direction) const
{
  return direction == PinDirection::kInput || direction == PinDirection::kInout;
}

std::vector<std::string> GraphBuilder::collectOutputPins(const TimingModel& timing_model, const Instance& instance) const
{
  std::vector<std::string> output_pin_list;
  for (const std::string& pin_name : instance.pin_names) {
    auto pin_iter = timing_model.pins.find(pin_name);
    if (pin_iter != timing_model.pins.end() && isOutputLike(pin_iter->second.direction)) {
      output_pin_list.push_back(pin_name);
    }
  }
  return output_pin_list;
}

bool GraphBuilder::isOutputLike(PinDirection direction) const
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

double GraphBuilder::estimateCellDelay(const std::string& cell_name) const
{
  return cell_name.empty() ? 1.0 : 1.0;
}

void GraphBuilder::buildEndpoints(TimingModel& timing_model)
{
  for (const auto& [pin_name, pin] : timing_model.pins) {
    timing_model.timing_points[pin_name] = TimingPoint();

    const bool has_incoming = timing_model.incoming_arc_list.count(pin_name) > 0 && !timing_model.incoming_arc_list[pin_name].empty();
    const bool has_outgoing = timing_model.outgoing_arc_list.count(pin_name) > 0 && !timing_model.outgoing_arc_list[pin_name].empty();

    if (!has_incoming || (pin.is_port && (pin.direction == PinDirection::kInput || pin.direction == PinDirection::kInout))) {
      appendUnique(timing_model.startpoint_list, pin_name);
    }
    if (!has_outgoing || (pin.is_port && (pin.direction == PinDirection::kOutput || pin.direction == PinDirection::kInout))) {
      appendUnique(timing_model.endpoint_list, pin_name);
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
