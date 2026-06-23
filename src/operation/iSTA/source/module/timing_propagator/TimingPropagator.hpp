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
#pragma once

#include "Database.hpp"

namespace ista {

#define STATP (ista::TimingPropagator::getInst())

class TimingPropagator
{
 public:
  static void initInst();
  static TimingPropagator& getInst();
  static void destroyInst();
  // function
  bool build();

 private:
  // self
  static TimingPropagator* _tp_instance;

  TimingPropagator() = default;
  TimingPropagator(const TimingPropagator& other) = delete;
  TimingPropagator(TimingPropagator&& other) = delete;
  ~TimingPropagator() = default;
  TimingPropagator& operator=(const TimingPropagator& other) = delete;
  TimingPropagator& operator=(TimingPropagator&& other) = delete;
  // function
  void propagateArrival(Database& database);
  void initTimingPointList(Database& database);
  void seedStartPointList(Database& database);
  double getStartPointArrival(Database& database, std::string& start_point);
  std::string getClockName(Database& database, std::string& pin_name);
  void propagateArrivalArc(Database& database, std::size_t arc_idx);
  bool isFinite(double value);
  void propagateRequired(Database& database);
  double resolveRequiredTime(Database& database);
  void seedEndPointRequired(Database& database, double required_time);
  double getEndPointRequired(Database& database, std::string& end_point, double default_required_time);
  double getClockPeriod(Database& database, std::string& clock_name);
  void propagateRequiredArc(Database& database, Arc& arc);
  void updateSlack(Database& database);
  void analyzeEndPointList(Database& database);
  TimingPathGroup initTimingPathGroup(Database& database);
  bool hasValidTiming(TimingPoint& timing_point);
  TimingPath buildTimingPath(Database& database, std::string& end_point);
  std::vector<std::string> getPathPinNameList(Database& database, std::string& end_point);
  std::vector<std::size_t> getPathArcIdxList(Database& database, std::vector<std::string>& path_pin_name_list);
  void updatePathDelay(TimingPath& timing_path, Arc* arc);
  void updateClockInfo(Database& database, TimingPath& timing_path);
  TimingPathPoint makeTimingPathPoint(Database& database, std::string& pin_name, Arc* arc);
  void insertTimingPath(TimingPathGroup& timing_path_group, TimingPath& timing_path);
  TimingPathEnd initTimingPathEnd(std::string& end_point);
  void updateWorstSlack(std::string& end_point, TimingPoint& timing_point, double& worst_slack, std::string& worst_end_point);
  void updateViolation(TimingPoint& timing_point, std::size_t& violation_num, double& total_negative_slack);
  std::size_t getTimingPathNum(TimingPathGroup& timing_path_group);
  void updateSummary(Database& database, TimingPathGroup& timing_path_group, std::size_t checked_end_point_num,
                     std::size_t unconstrained_end_point_num, std::size_t violation_num, double worst_slack,
                     double total_negative_slack, std::string& worst_end_point);
};

}  // namespace ista
