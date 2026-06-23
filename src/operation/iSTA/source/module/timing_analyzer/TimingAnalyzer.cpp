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
#include "TimingAnalyzer.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void TimingAnalyzer::initInst()
{
  if (_ta_instance == nullptr) {
    _ta_instance = new TimingAnalyzer();
  }
}

TimingAnalyzer& TimingAnalyzer::getInst()
{
  if (_ta_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_ta_instance;
}

void TimingAnalyzer::destroyInst()
{
  if (_ta_instance != nullptr) {
    delete _ta_instance;
    _ta_instance = nullptr;
  }
}

// function

bool TimingAnalyzer::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();
  analyzeEndPointList(database);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

TimingAnalyzer* TimingAnalyzer::_ta_instance = nullptr;

void TimingAnalyzer::analyzeEndPointList(Database& database)
{
  double worst_slack = std::numeric_limits<double>::infinity();
  std::string worst_end_point;
  std::size_t checked_end_point_num = 0;
  std::size_t unconstrained_end_point_num = 0;
  std::size_t violation_num = 0;
  double total_negative_slack = 0.0;
  TimingPathGroup timing_path_group = initTimingPathGroup();

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

TimingPathGroup TimingAnalyzer::initTimingPathGroup()
{
  TimingPathGroup timing_path_group;
  timing_path_group.set_group_name("default");
  return timing_path_group;
}

bool TimingAnalyzer::hasValidTiming(TimingPoint& timing_point)
{
  return std::isfinite(timing_point.get_arrival()) && std::isfinite(timing_point.get_required());
}

TimingPath TimingAnalyzer::buildTimingPath(Database& database, std::string& end_point)
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

std::vector<std::string> TimingAnalyzer::getPathPinNameList(Database& database, std::string& end_point)
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

std::vector<std::size_t> TimingAnalyzer::getPathArcIdxList(Database& database, std::vector<std::string>& path_pin_name_list)
{
  std::vector<std::size_t> path_arc_idx_list;
  for (size_t i = 1; i < path_pin_name_list.size(); i++) {
    TimingPoint& timing_point = database.get_timing_point_map()[path_pin_name_list[i]];
    path_arc_idx_list.push_back(timing_point.get_predecessor_arc_idx());
  }
  return path_arc_idx_list;
}

void TimingAnalyzer::updatePathDelay(TimingPath& timing_path, Arc* arc)
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

TimingPathPoint TimingAnalyzer::makeTimingPathPoint(Database& database, std::string& pin_name, Arc* arc)
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

void TimingAnalyzer::insertTimingPath(TimingPathGroup& timing_path_group, TimingPath& timing_path)
{
  std::string& end_point = timing_path.get_end_point();
  if (timing_path_group.get_timing_path_end_map().count(end_point) == 0) {
    timing_path_group.get_timing_path_end_map()[end_point] = initTimingPathEnd(end_point);
  }
  timing_path_group.get_timing_path_end_map()[end_point].get_timing_path_list().push_back(timing_path);
}

TimingPathEnd TimingAnalyzer::initTimingPathEnd(std::string& end_point)
{
  TimingPathEnd timing_path_end;
  timing_path_end.set_end_point(end_point);
  return timing_path_end;
}

void TimingAnalyzer::updateWorstSlack(std::string& end_point, TimingPoint& timing_point, double& worst_slack,
                                      std::string& worst_end_point)
{
  if (timing_point.get_slack() < worst_slack) {
    worst_slack = timing_point.get_slack();
    worst_end_point = end_point;
  }
}

void TimingAnalyzer::updateViolation(TimingPoint& timing_point, std::size_t& violation_num, double& total_negative_slack)
{
  if (timing_point.get_slack() < 0.0) {
    ++violation_num;
    total_negative_slack += timing_point.get_slack();
  }
}

std::size_t TimingAnalyzer::getTimingPathNum(TimingPathGroup& timing_path_group)
{
  std::size_t timing_path_num = 0;
  for (auto& [end_point, timing_path_end] : timing_path_group.get_timing_path_end_map()) {
    timing_path_num += timing_path_end.get_timing_path_list().size();
  }
  return timing_path_num;
}

void TimingAnalyzer::updateSummary(Database& database, TimingPathGroup& timing_path_group, std::size_t checked_end_point_num,
                                   std::size_t unconstrained_end_point_num, std::size_t violation_num, double worst_slack,
                                   double total_negative_slack, std::string& worst_end_point)
{
  TASummary& ta_summary = database.get_summary().ta_summary;
  ta_summary.timing_path_num = getTimingPathNum(timing_path_group);
  ta_summary.checked_end_point_num = checked_end_point_num;
  ta_summary.unconstrained_end_point_num = unconstrained_end_point_num;
  ta_summary.violating_end_point_num = violation_num;
  ta_summary.worst_slack = worst_slack;
  ta_summary.total_negative_slack = total_negative_slack;
  ta_summary.worst_end_point = worst_end_point;
}

}  // namespace ista
