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

  Database& database = STADM.getDatabase();
  if (database.get_pin_map().empty()) {
    STALOG.warn(Loc::current(), "iSTA database has no pin, skip propagation.");
    return false;
  }

  std::vector<std::string> timing_order = propagateArrival(database);
  propagateRequired(database, timing_order);

  database.get_summary().timing_order = timing_order;
  STALOG.info(Loc::current(), "Propagate iSTA timing: timing_order=", timing_order.size(), " loop_vertices=",
              database.get_summary().loop_vertex_num);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphPropagator* GraphPropagator::_gp_instance = nullptr;

std::vector<std::string> GraphPropagator::propagateArrival(Database& database)
{
  const double kEpsilon = 1e-9;
  std::unordered_map<std::string, std::size_t> indegree_map;
  std::queue<std::string> ready_queue;
  std::vector<std::string> timing_order;

  for (auto& [pin_name, timing_point] : database.get_timing_point_map()) {
    timing_point.set_arrival(-std::numeric_limits<double>::infinity());
    timing_point.set_required(std::numeric_limits<double>::infinity());
    timing_point.set_slack(0.0);
    timing_point.get_predecessor().clear();
    indegree_map[pin_name] = database.get_incoming_arc_list_map()[pin_name].size();
  }

  for (std::string& start_point : database.get_start_point_list()) {
    auto timing_iter = database.get_timing_point_map().find(start_point);
    if (timing_iter != database.get_timing_point_map().end()) {
      timing_iter->second.set_arrival(0.0);
    }
  }

  for (auto& [pin_name, timing_point] : database.get_timing_point_map()) {
    if (indegree_map[pin_name] == 0) {
      ready_queue.push(pin_name);
    }
  }

  while (!ready_queue.empty()) {
    const std::string pin_name = ready_queue.front();
    ready_queue.pop();
    timing_order.push_back(pin_name);

    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
      TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
      if (isFinite(source_point.get_arrival())) {
        const double candidate_arrival = source_point.get_arrival() + arc.get_delay();
        if (!isFinite(sink_point.get_arrival()) || candidate_arrival > sink_point.get_arrival() + kEpsilon) {
          sink_point.set_arrival(candidate_arrival);
          sink_point.set_predecessor(arc.get_source_pin());
        }
      }
      if (indegree_map[arc.get_sink_pin()] > 0) {
        --indegree_map[arc.get_sink_pin()];
      }
      if (indegree_map[arc.get_sink_pin()] == 0) {
        ready_queue.push(arc.get_sink_pin());
      }
    }
  }

  database.get_summary().loop_vertex_num = database.get_pin_map().size() - timing_order.size();
  if (database.get_summary().loop_vertex_num > 0) {
    STALOG.warn(Loc::current(), "Detected ", database.get_summary().loop_vertex_num,
                " vertex(es) in combinational loop or unresolved dependency.");
  }

  return timing_order;
}

bool GraphPropagator::isFinite(double value)
{
  return std::isfinite(value);
}

void GraphPropagator::propagateRequired(Database& database, std::vector<std::string>& timing_order)
{
  const double required_time = resolveRequiredTime(database);
  database.get_summary().required_time = required_time;

  for (std::string& end_point : database.get_end_point_list()) {
    auto timing_iter = database.get_timing_point_map().find(end_point);
    if (timing_iter != database.get_timing_point_map().end()) {
      timing_iter->second.set_required(required_time);
    }
  }

  for (auto iter = timing_order.rbegin(); iter != timing_order.rend(); ++iter) {
    std::string& pin_name = *iter;
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
      TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
      if (isFinite(sink_point.get_required())) {
        source_point.set_required(std::min(source_point.get_required(), sink_point.get_required() - arc.get_delay()));
      }
    }
  }

  for (auto& [pin_name, timing_point] : database.get_timing_point_map()) {
    if (isFinite(timing_point.get_arrival()) && isFinite(timing_point.get_required())) {
      timing_point.set_slack(timing_point.get_required() - timing_point.get_arrival());
    }
  }
}

double GraphPropagator::resolveRequiredTime(Database& database)
{
  double worst_arrival = 0.0;
  for (std::string& end_point : database.get_end_point_list()) {
    auto timing_iter = database.get_timing_point_map().find(end_point);
    if (timing_iter != database.get_timing_point_map().end() && isFinite(timing_iter->second.get_arrival())) {
      worst_arrival = std::max(worst_arrival, timing_iter->second.get_arrival());
    }
  }
  return worst_arrival;
}

}  // namespace ista
