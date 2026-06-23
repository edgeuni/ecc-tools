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

  propagateArrival(database);
  propagateRequired(database);

  std::size_t loop_vertex_num = database.get_pin_map().size() - database.get_timing_order_list().size();
  STALOG.info(Loc::current(), "Propagate iSTA timing: timing_order=", database.get_timing_order_list().size(), " loop_vertices=",
              loop_vertex_num);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphPropagator* GraphPropagator::_gp_instance = nullptr;

void GraphPropagator::propagateArrival(Database& database)
{
  initTimingPointList(database);
  seedStartPointList(database);

  for (std::string& pin_name : database.get_timing_order_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      propagateArrivalArc(database, arc_idx);
    }
  }
}

void GraphPropagator::initTimingPointList(Database& database)
{
  for (auto& timing_pair : database.get_timing_point_map()) {
    timing_pair.second.set_arrival(-std::numeric_limits<double>::infinity());
    timing_pair.second.set_required(std::numeric_limits<double>::infinity());
    timing_pair.second.set_slack(0.0);
    timing_pair.second.get_predecessor().clear();
    timing_pair.second.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
  }
}

void GraphPropagator::seedStartPointList(Database& database)
{
  for (std::string& start_point : database.get_start_point_list()) {
    database.get_timing_point_map()[start_point].set_arrival(0.0);
  }
}

void GraphPropagator::propagateArrivalArc(Database& database, std::size_t arc_idx)
{
  const double kEpsilon = 1e-9;
  Arc& arc = database.get_arc_list()[arc_idx];
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (isFinite(source_point.get_arrival())) {
    const double candidate_arrival = source_point.get_arrival() + arc.get_delay();
    if (!isFinite(sink_point.get_arrival()) || candidate_arrival > sink_point.get_arrival() + kEpsilon) {
      sink_point.set_arrival(candidate_arrival);
      sink_point.set_predecessor(arc.get_source_pin());
      sink_point.set_predecessor_arc_idx(arc_idx);
    }
  }
}

bool GraphPropagator::isFinite(double value)
{
  return std::isfinite(value);
}

void GraphPropagator::propagateRequired(Database& database)
{
  const double required_time = resolveRequiredTime(database);

  seedEndPointRequired(database, required_time);

  for (auto iter = database.get_timing_order_list().rbegin(); iter != database.get_timing_order_list().rend(); ++iter) {
    std::string& pin_name = *iter;
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      propagateRequiredArc(database, database.get_arc_list()[arc_idx]);
    }
  }

  updateSlack(database);
}

double GraphPropagator::resolveRequiredTime(Database& database)
{
  double worst_arrival = 0.0;
  for (std::string& end_point : database.get_end_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[end_point];
    if (isFinite(timing_point.get_arrival())) {
      worst_arrival = std::max(worst_arrival, timing_point.get_arrival());
    }
  }
  return worst_arrival;
}

void GraphPropagator::seedEndPointRequired(Database& database, double required_time)
{
  for (std::string& end_point : database.get_end_point_list()) {
    database.get_timing_point_map()[end_point].set_required(required_time);
  }
}

void GraphPropagator::propagateRequiredArc(Database& database, Arc& arc)
{
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (isFinite(sink_point.get_required())) {
    source_point.set_required(std::min(source_point.get_required(), sink_point.get_required() - arc.get_delay()));
  }
}

void GraphPropagator::updateSlack(Database& database)
{
  for (auto& timing_pair : database.get_timing_point_map()) {
    TimingPoint& timing_point = timing_pair.second;
    if (isFinite(timing_point.get_arrival()) && isFinite(timing_point.get_required())) {
      timing_point.set_slack(timing_point.get_required() - timing_point.get_arrival());
    }
  }
}

}  // namespace ista
