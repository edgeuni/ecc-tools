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
#include "TimingPropagator.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void TimingPropagator::initInst()
{
  if (_tp_instance == nullptr) {
    _tp_instance = new TimingPropagator();
  }
}

TimingPropagator& TimingPropagator::getInst()
{
  if (_tp_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tp_instance;
}

void TimingPropagator::destroyInst()
{
  if (_tp_instance != nullptr) {
    delete _tp_instance;
    _tp_instance = nullptr;
  }
}

// function

bool TimingPropagator::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  propagateArrival(database);
  propagateRequired(database);
  analyzeEndPointList(database);

  std::size_t loop_vertex_num = database.get_pin_map().size() - database.get_timing_order_list().size();
  STALOG.info(Loc::current(), "Propagate iSTA timing: timing_order=", database.get_timing_order_list().size(), " loop_vertices=",
              loop_vertex_num);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

TimingPropagator* TimingPropagator::_tp_instance = nullptr;

void TimingPropagator::propagateArrival(Database& database)
{
  initTimingPointList(database);
  seedStartPointList(database);

  for (std::string& pin_name : database.get_timing_order_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      propagateArrivalArc(database, arc_idx);
    }
  }
}

void TimingPropagator::initTimingPointList(Database& database)
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

void TimingPropagator::seedStartPointList(Database& database)
{
  for (std::string& start_point : database.get_start_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[start_point];
    timing_point.set_arrival(getStartPointArrival(database, start_point));
    timing_point.set_launch_time(0.0);
    timing_point.set_clock_name(getClockName(database, start_point));
  }
}

double TimingPropagator::getStartPointArrival(Database& database, std::string& start_point)
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

std::string TimingPropagator::getClockName(Database& database, std::string& pin_name)
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

void TimingPropagator::propagateArrivalArc(Database& database, std::size_t arc_idx)
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

bool TimingPropagator::isFinite(double value)
{
  return std::isfinite(value);
}

void TimingPropagator::propagateRequired(Database& database)
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

double TimingPropagator::resolveRequiredTime(Database& database)
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

void TimingPropagator::seedEndPointRequired(Database& database, double required_time)
{
  for (std::string& end_point : database.get_end_point_list()) {
    database.get_timing_point_map()[end_point].set_required(getEndPointRequired(database, end_point, required_time));
  }
}

double TimingPropagator::getEndPointRequired(Database& database, std::string& end_point, double default_required_time)
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

double TimingPropagator::getClockPeriod(Database& database, std::string& clock_name)
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

void TimingPropagator::propagateRequiredArc(Database& database, Arc& arc)
{
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (isFinite(sink_point.get_required())) {
    source_point.set_required(std::min(source_point.get_required(), sink_point.get_required() - arc.get_delay()));
  }
}

void TimingPropagator::updateSlack(Database& database)
{
  for (auto& timing_pair : database.get_timing_point_map()) {
    TimingPoint& timing_point = timing_pair.second;
    if (isFinite(timing_point.get_arrival()) && isFinite(timing_point.get_required())) {
      timing_point.set_slack(timing_point.get_required() - timing_point.get_arrival());
    }
  }
}

void TimingPropagator::analyzeEndPointList(Database& database)
{
  double worst_slack = std::numeric_limits<double>::infinity();
  std::string worst_end_point;
  std::size_t checked_end_point_num = 0;
  std::size_t unconstrained_end_point_num = 0;
  std::size_t violation_num = 0;
  double total_negative_slack = 0.0;
  TimingPathGroup timing_path_group = initTimingPathGroup(database);

  database.get_timing_path_group_list().clear();
  for (std::string& end_point : database.get_end_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[end_point];
    if (!hasValidTiming(timing_point)) {
      ++unconstrained_end_point_num;
      continue;
    }
    ++checked_end_point_num;
    TimingPath timing_path = buildTimingPath(database, end_point);
    insertTimingPath(timing_path_group, timing_path);
    updateWorstSlack(end_point, timing_point, worst_slack, worst_end_point);
    updateViolation(timing_point, violation_num, total_negative_slack);
  }

  if (!std::isfinite(worst_slack)) {
    worst_slack = 0.0;
  }
  updateSummary(database, timing_path_group, checked_end_point_num, unconstrained_end_point_num, violation_num, worst_slack,
                total_negative_slack, worst_end_point);
  database.get_timing_path_group_list().push_back(timing_path_group);

  STALOG.info(Loc::current(), "Analyze iSTA timing: timing_paths=", getTimingPathNum(timing_path_group),
              " checked_end_points=", checked_end_point_num, " unconstrained_end_points=", unconstrained_end_point_num,
              " violating_end_points=", violation_num, " worst_slack=", worst_slack,
              " total_negative_slack=", total_negative_slack, " end_point=", worst_end_point);
}

TimingPathGroup TimingPropagator::initTimingPathGroup(Database& database)
{
  TimingPathGroup timing_path_group;
  if (!database.get_timing_constraint().get_clock_map().empty()) {
    timing_path_group.set_group_name(database.get_timing_constraint().get_clock_map().begin()->first);
  } else {
    timing_path_group.set_group_name("default");
  }
  return timing_path_group;
}

bool TimingPropagator::hasValidTiming(TimingPoint& timing_point)
{
  return std::isfinite(timing_point.get_arrival()) && std::isfinite(timing_point.get_required());
}

TimingPath TimingPropagator::buildTimingPath(Database& database, std::string& end_point)
{
  std::vector<std::string> path_pin_name_list = getPathPinNameList(database, end_point);
  std::vector<std::size_t> path_arc_idx_list = getPathArcIdxList(database, path_pin_name_list);
  TimingPath timing_path;
  timing_path.set_start_point(path_pin_name_list.front());
  timing_path.set_end_point(end_point);
  timing_path.set_path_delay(database.get_timing_point_map()[end_point].get_arrival());
  timing_path.set_required_time(database.get_timing_point_map()[end_point].get_required());
  timing_path.set_slack(database.get_timing_point_map()[end_point].get_slack());
  timing_path.set_level(database.get_timing_point_map()[end_point].get_level());
  updateClockInfo(database, timing_path);

  Arc* arc = nullptr;
  for (size_t i = 0; i < path_pin_name_list.size(); i++) {
    if (i > 0) {
      arc = &database.get_arc_list()[path_arc_idx_list[i - 1]];
      updatePathDelay(timing_path, arc);
    }
    timing_path.get_point_list().push_back(makeTimingPathPoint(database, path_pin_name_list[i], arc));
  }
  return timing_path;
}

std::vector<std::string> TimingPropagator::getPathPinNameList(Database& database, std::string& end_point)
{
  std::vector<std::string> path_pin_name_list;
  std::string pin_name = end_point;
  while (!pin_name.empty()) {
    path_pin_name_list.push_back(pin_name);
    pin_name = database.get_timing_point_map()[pin_name].get_predecessor();
  }
  std::reverse(path_pin_name_list.begin(), path_pin_name_list.end());
  return path_pin_name_list;
}

std::vector<std::size_t> TimingPropagator::getPathArcIdxList(Database& database, std::vector<std::string>& path_pin_name_list)
{
  std::vector<std::size_t> path_arc_idx_list;
  for (size_t i = 1; i < path_pin_name_list.size(); i++) {
    TimingPoint& timing_point = database.get_timing_point_map()[path_pin_name_list[i]];
    path_arc_idx_list.push_back(timing_point.get_predecessor_arc_idx());
  }
  return path_arc_idx_list;
}

void TimingPropagator::updatePathDelay(TimingPath& timing_path, Arc* arc)
{
  if (arc == nullptr) {
    return;
  }
  if (arc->get_type() == ArcType::kCell) {
    timing_path.set_cell_delay(timing_path.get_cell_delay() + arc->get_delay());
  } else if (arc->get_type() == ArcType::kNet) {
    timing_path.set_net_delay(timing_path.get_net_delay() + arc->get_delay());
  }
}

void TimingPropagator::updateClockInfo(Database& database, TimingPath& timing_path)
{
  TimingPoint& end_timing_point = database.get_timing_point_map()[timing_path.get_end_point()];
  timing_path.set_launch_time(end_timing_point.get_launch_time());
  timing_path.set_clock_name(end_timing_point.get_clock_name());
  timing_path.set_capture_time(getClockPeriod(database, timing_path.get_clock_name()));

  Pin& end_pin = database.get_pin_map()[timing_path.get_end_point()];
  if (end_pin.get_is_port() || database.get_instance_map().count(end_pin.get_instance_name()) == 0) {
    return;
  }
  Instance& instance = database.get_instance_map()[end_pin.get_instance_name()];
  if (instance.get_is_sequential() && timing_path.get_end_point() == instance.get_data_pin_name()) {
    timing_path.set_capture_clock_pin(instance.get_clock_pin_name());
    timing_path.set_setup_time(instance.get_setup_time());
  }
}

TimingPathPoint TimingPropagator::makeTimingPathPoint(Database& database, std::string& pin_name, Arc* arc)
{
  Pin& pin = database.get_pin_map()[pin_name];
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  TimingPathPoint path_point;
  path_point.set_pin_name(pin_name);
  path_point.set_instance_name(pin.get_instance_name());
  if (!pin.get_instance_name().empty()) {
    path_point.set_cell_name(database.get_instance_map()[pin.get_instance_name()].get_cell_name());
  }
  path_point.set_net_name(pin.get_net_name());
  path_point.set_arrival(timing_point.get_arrival());
  path_point.set_required(timing_point.get_required());
  path_point.set_slack(timing_point.get_slack());
  if (arc != nullptr) {
    path_point.set_arc_name(arc->get_arc_name());
    path_point.set_source_pin(arc->get_source_pin());
    path_point.set_sink_pin(arc->get_sink_pin());
    path_point.set_arc_type(arc->get_type());
    path_point.set_arc_delay(arc->get_delay());
  }
  return path_point;
}

void TimingPropagator::insertTimingPath(TimingPathGroup& timing_path_group, TimingPath& timing_path)
{
  std::string& end_point = timing_path.get_end_point();
  if (timing_path_group.get_timing_path_end_map().count(end_point) == 0) {
    timing_path_group.get_timing_path_end_map()[end_point] = initTimingPathEnd(end_point);
  }
  timing_path_group.get_timing_path_end_map()[end_point].get_timing_path_list().push_back(timing_path);
}

TimingPathEnd TimingPropagator::initTimingPathEnd(std::string& end_point)
{
  TimingPathEnd timing_path_end;
  timing_path_end.set_end_point(end_point);
  return timing_path_end;
}

void TimingPropagator::updateWorstSlack(std::string& end_point, TimingPoint& timing_point, double& worst_slack,
                                        std::string& worst_end_point)
{
  if (timing_point.get_slack() < worst_slack) {
    worst_slack = timing_point.get_slack();
    worst_end_point = end_point;
  }
}

void TimingPropagator::updateViolation(TimingPoint& timing_point, std::size_t& violation_num, double& total_negative_slack)
{
  if (timing_point.get_slack() < 0.0) {
    ++violation_num;
    total_negative_slack += timing_point.get_slack();
  }
}

std::size_t TimingPropagator::getTimingPathNum(TimingPathGroup& timing_path_group)
{
  std::size_t timing_path_num = 0;
  for (auto& [end_point, timing_path_end] : timing_path_group.get_timing_path_end_map()) {
    timing_path_num += timing_path_end.get_timing_path_list().size();
  }
  return timing_path_num;
}

void TimingPropagator::updateSummary(Database& database, TimingPathGroup& timing_path_group, std::size_t checked_end_point_num,
                                     std::size_t unconstrained_end_point_num, std::size_t violation_num, double worst_slack,
                                     double total_negative_slack, std::string& worst_end_point)
{
  TPSummary& tp_summary = database.get_summary().tp_summary;
  tp_summary.timing_path_num = getTimingPathNum(timing_path_group);
  tp_summary.checked_end_point_num = checked_end_point_num;
  tp_summary.unconstrained_end_point_num = unconstrained_end_point_num;
  tp_summary.violating_end_point_num = violation_num;
  tp_summary.worst_slack = worst_slack;
  tp_summary.total_negative_slack = total_negative_slack;
  tp_summary.worst_end_point = worst_end_point;
}

}  // namespace ista
