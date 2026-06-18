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
#include "GraphPropagator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void GraphPropagator::initInst()
{
  if (_gp_instance == nullptr) {
    _gp_instance = new GraphPropagator();
  }
}

GraphPropagator& GraphPropagator::getInst()
{
  if (_gp_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_gp_instance;
}

void GraphPropagator::destroyInst()
{
  if (_gp_instance != nullptr) {
    delete _gp_instance;
    _gp_instance = nullptr;
  }
}

// function

bool GraphPropagator::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  TimingModel& timing_model = STADM.getTimingModel();
  if (timing_model.pins.empty()) {
    STALOG.warn(Loc::current(), "iSTA timing model has no pin, skip propagation.");
    return false;
  }

  std::vector<std::string> timing_order = propagateArrival(timing_model);
  propagateRequired(timing_model, timing_order);

  timing_model.summary.timing_order = timing_order;
  STALOG.info(Loc::current(), "Propagate iSTA timing: timing_order=", timing_order.size(), " loop_vertices=",
              timing_model.summary.loop_vertex_num);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphPropagator* GraphPropagator::_gp_instance = nullptr;

std::vector<std::string> GraphPropagator::propagateArrival(TimingModel& timing_model)
{
  const double kEpsilon = 1e-9;
  std::unordered_map<std::string, std::size_t> indegree_map;
  std::queue<std::string> ready_queue;
  std::vector<std::string> timing_order;

  for (auto& [pin_name, timing_point] : timing_model.timing_points) {
    timing_point.arrival = -std::numeric_limits<double>::infinity();
    timing_point.required = std::numeric_limits<double>::infinity();
    timing_point.slack = 0.0;
    timing_point.predecessor.clear();
    indegree_map[pin_name] = timing_model.incoming_arc_list[pin_name].size();
  }

  for (const std::string& startpoint : timing_model.startpoint_list) {
    auto timing_iter = timing_model.timing_points.find(startpoint);
    if (timing_iter != timing_model.timing_points.end()) {
      timing_iter->second.arrival = 0.0;
    }
  }

  for (const auto& [pin_name, timing_point] : timing_model.timing_points) {
    if (indegree_map[pin_name] == 0) {
      ready_queue.push(pin_name);
    }
  }

  while (!ready_queue.empty()) {
    const std::string pin_name = ready_queue.front();
    ready_queue.pop();
    timing_order.push_back(pin_name);

    for (std::size_t arc_idx : timing_model.outgoing_arc_list[pin_name]) {
      const Arc& arc = timing_model.arcs[arc_idx];
      TimingPoint& source_point = timing_model.timing_points[arc.source_pin];
      TimingPoint& sink_point = timing_model.timing_points[arc.sink_pin];
      if (isFinite(source_point.arrival)) {
        const double candidate_arrival = source_point.arrival + arc.delay;
        if (!isFinite(sink_point.arrival) || candidate_arrival > sink_point.arrival + kEpsilon) {
          sink_point.arrival = candidate_arrival;
          sink_point.predecessor = arc.source_pin;
        }
      }
      if (indegree_map[arc.sink_pin] > 0) {
        --indegree_map[arc.sink_pin];
      }
      if (indegree_map[arc.sink_pin] == 0) {
        ready_queue.push(arc.sink_pin);
      }
    }
  }

  timing_model.summary.loop_vertex_num = timing_model.pins.size() - timing_order.size();
  if (timing_model.summary.loop_vertex_num > 0) {
    STALOG.warn(Loc::current(), "Detected ", timing_model.summary.loop_vertex_num,
                " vertex(es) in combinational loop or unresolved dependency.");
  }

  return timing_order;
}

bool GraphPropagator::isFinite(double value) const
{
  return std::isfinite(value);
}

void GraphPropagator::propagateRequired(TimingModel& timing_model, const std::vector<std::string>& timing_order)
{
  const double required_time = resolveRequiredTime(timing_model);
  timing_model.summary.required_time = required_time;

  for (const std::string& endpoint : timing_model.endpoint_list) {
    auto timing_iter = timing_model.timing_points.find(endpoint);
    if (timing_iter != timing_model.timing_points.end()) {
      timing_iter->second.required = required_time;
    }
  }

  for (auto iter = timing_order.rbegin(); iter != timing_order.rend(); ++iter) {
    const std::string& pin_name = *iter;
    for (std::size_t arc_idx : timing_model.outgoing_arc_list[pin_name]) {
      const Arc& arc = timing_model.arcs[arc_idx];
      TimingPoint& source_point = timing_model.timing_points[arc.source_pin];
      TimingPoint& sink_point = timing_model.timing_points[arc.sink_pin];
      if (isFinite(sink_point.required)) {
        source_point.required = std::min(source_point.required, sink_point.required - arc.delay);
      }
    }
  }

  for (auto& [pin_name, timing_point] : timing_model.timing_points) {
    if (isFinite(timing_point.arrival) && isFinite(timing_point.required)) {
      timing_point.slack = timing_point.required - timing_point.arrival;
    }
  }
}

double GraphPropagator::resolveRequiredTime(const TimingModel& timing_model) const
{
  double worst_arrival = 0.0;
  for (const std::string& endpoint : timing_model.endpoint_list) {
    auto timing_iter = timing_model.timing_points.find(endpoint);
    if (timing_iter != timing_model.timing_points.end() && isFinite(timing_iter->second.arrival)) {
      worst_arrival = std::max(worst_arrival, timing_iter->second.arrival);
    }
  }
  return worst_arrival;
}

}  // namespace ista
