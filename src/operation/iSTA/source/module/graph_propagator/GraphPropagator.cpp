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
    timing_pair.second.set_launch_time(0.0);
    timing_pair.second.get_predecessor().clear();
    timing_pair.second.get_clock_name().clear();
    timing_pair.second.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
    timing_pair.second.set_is_clock_point(false);
  }
}

void GraphPropagator::seedStartPointList(Database& database)
{
  for (std::string& start_point : database.get_start_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[start_point];
    timing_point.set_arrival(getStartPointArrival(database, start_point));
    timing_point.set_launch_time(0.0);
    timing_point.set_clock_name(getClockName(database, start_point));
  }
}

double GraphPropagator::getStartPointArrival(Database& database, std::string& start_point)
{
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port()) {
    auto& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (port_constraint_map.count(start_point) > 0 && port_constraint_map[start_point].get_has_input_delay_max()) {
      return port_constraint_map[start_point].get_input_delay_max();
    }
    return 0.0;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (instance.get_is_sequential() && start_point == instance.get_output_pin_name()) {
    return instance.get_clock_to_q_delay();
  }
  return 0.0;
}

std::string GraphPropagator::getClockName(Database& database, std::string& pin_name)
{
  Pin& pin = database.get_pin_map()[pin_name];
  auto& clock_map = database.get_timing_constraint().get_clock_map();
  if (pin.get_is_port()) {
    auto& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (port_constraint_map.count(pin_name) > 0 && !port_constraint_map[pin_name].get_clock_name().empty()) {
      return port_constraint_map[pin_name].get_clock_name();
    }
  }
  if (!clock_map.empty()) {
    return clock_map.begin()->first;
  }
  return "clk";
}

double GraphPropagator::getClockPeriod(Database& database, std::string& clock_name)
{
  auto& clock_map = database.get_timing_constraint().get_clock_map();
  if (clock_map.count(clock_name) > 0) {
    return clock_map[clock_name].get_period();
  }
  if (!clock_map.empty()) {
    return clock_map.begin()->second.get_period();
  }
  return 0.0;
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
      sink_point.set_launch_time(source_point.get_launch_time());
      sink_point.set_clock_name(source_point.get_clock_name());
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
    database.get_timing_point_map()[end_point].set_required(getEndPointRequired(database, end_point, required_time));
  }
}

double GraphPropagator::getEndPointRequired(Database& database, std::string& end_point, double default_required_time)
{
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port()) {
    auto& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (port_constraint_map.count(end_point) > 0 && port_constraint_map[end_point].get_has_output_delay_max()) {
      std::string clock_name = port_constraint_map[end_point].get_clock_name();
      return getClockPeriod(database, clock_name) - port_constraint_map[end_point].get_output_delay_max();
    }
    return default_required_time;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return default_required_time;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (instance.get_is_sequential() && end_point == instance.get_data_pin_name()) {
    std::string clock_name = getClockName(database, end_point);
    return getClockPeriod(database, clock_name) - instance.get_setup_time();
  }
  return default_required_time;
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
