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
  bool isDisableArc(Arc& arc);
  void initTimingPointList(Database& database);
  void markClockPointList(Database& database);
  void markClockPoint(Database& database, std::string& clock_source);
  void propagateClockArrival(Database& database);
  void seedClockArrival(Database& database, std::string& clock_source);
  void propagateClockArrivalArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type);
  void propagateClockArrivalArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type);
  void updateClockPathState(Database& database, Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                            TransType input_trans_type, TransType output_trans_type);
  bool hasClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  double getClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  void updateClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, double clock_arrival);
  void updateClockPredecessor(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, TransType predecessor_trans_type, Arc& arc,
                              double arc_delay);
  bool shouldStopClockPropagation(Database& database, std::string& pin_name);
  double getClockArrival(Database& database, std::string& pin_name, AnalysisType analysis_type);
  double getClockArrival(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type);
  double getClockSlew(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type);
  void seedStartPointList(Database& database);
  void propagateDataSlewDelay(Database& database);
  void seedDataSlewList(Database& database);
  void seedDataSlew(Database& database, std::string& start_point, AnalysisType analysis_type);
  void seedDataSlew(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  void propagateDataSlewDelayArc(Database& database, std::size_t arc_idx);
  void propagateDataSlewDelayArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type);
  bool isClockArcTriggerTrans(Arc& arc, TransType input_trans_type);
  void updateDataSlewDelay(Database& database, Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                           TransType input_trans_type, TransType output_trans_type);
  void updateGraphArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double arc_delay);
  void updateDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, double data_slew);
  bool hasDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  double getDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  bool isBetterDelay(double candidate_delay, double current_delay, AnalysisType analysis_type);
  bool isBetterSlew(double candidate_slew, double current_slew, AnalysisType analysis_type);
  double roundTime(double time);
  double getStartPointArrival(Database& database, std::string& start_point, AnalysisType analysis_type);
  double getStartPointArrival(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  double getStartPointSlew(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  double getStartPointLaunchTime(Database& database, std::string& start_point, AnalysisType analysis_type);
  TransType getClockTransType(TimingCellArc& timing_cell_arc);
  std::string getClockName(Database& database, std::string& pin_name);
  std::string getPathStateStartPoint(Database& database, std::string& start_point);
  void seedPathState(Database& database, std::string& start_point, AnalysisType analysis_type);
  PathSourceType getStartPointSourceType(Database& database, std::string& start_point, AnalysisType analysis_type);
  bool hasInputDelay(Database& database, std::string& start_point, AnalysisType analysis_type);
  bool isInputStartPoint(Database& database, std::string& start_point);
  bool isRegisterStartPoint(Database& database, std::string& start_point);
  bool hasClockPoint(Database& database, std::string& pin_name);
  void propagateArrivalArc(Database& database, std::size_t arc_idx);
  void propagatePathStateArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type);
  void propagatePathStateArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type, TransType input_trans_type);
  void propagatePathStateArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type, TransType input_trans_type,
                             TransType output_trans_type);
  std::vector<TransType> getOutputTransTypeList(Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  TransType getOutputTransType(Arc& arc, TransType input_trans_type);
  double getArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  double getArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type);
  double calcArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcArcSlew(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double input_slew);
  bool hasPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type);
  bool hasPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type);
  std::map<std::string, TimingPathState>& getPathStateMap(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type,
                                                          TransType trans_type);
  TimingPathState& getPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                                std::string& start_point);
  TimingPathState* getWorstPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type);
  TimingPathState* getWorstPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type);
  TransType getEndPointTransType(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type);
  bool isBetterArrival(double candidate_arrival, double current_arrival, AnalysisType analysis_type);
  bool isFinite(double value);
  void propagateRequired(Database& database);
  double resolveRequiredTime(Database& database);
  void seedEndPointRequired(Database& database, double required_time);
  double getEndPointRequired(Database& database, std::string& end_point, double default_required_time, AnalysisType analysis_type);
  double getEndPointRequired(Database& database, std::string& end_point, double default_required_time, AnalysisType analysis_type,
                             TransType data_trans_type, double data_slew);
  double getEndPointRequired(Database& database, std::string& start_point, std::string& end_point, double default_required_time,
                             AnalysisType analysis_type, TransType data_trans_type, double data_slew);
  double getEndPointCheckTime(Database& database, std::string& end_point, TimingCheckArc& timing_check_arc, AnalysisType analysis_type,
                              TransType data_trans_type, double data_slew);
  AnalysisType getCaptureAnalysisType(AnalysisType analysis_type);
  TransType getClockTransType(TimingCheckArc& timing_check_arc);
  double getEndPointCaptureTime(Database& database, std::string& end_point, AnalysisType analysis_type);
  double getEndPointClockArrival(Database& database, std::string& end_point, AnalysisType analysis_type);
  double getEndPointClockArrival(Database& database, std::string& end_point, AnalysisType analysis_type, TransType trans_type);
  double getClockReconvergencePessimism(Database& database, std::string& start_point, std::string& end_point, AnalysisType analysis_type,
                                        std::string& common_pin_name);
  void shrinkClockPathToCrprPath(Database& database, std::vector<std::pair<std::string, TransType>>& clock_path);
  bool isLeafClockDriverPin(Database& database, std::string& pin_name);
  std::vector<std::pair<std::string, TransType>> getClockPathPinList(Database& database, std::string& clock_pin_name, AnalysisType analysis_type,
                                                                     TransType trans_type);
  double getClockCommonPathArrival(Database& database, std::pair<std::string, TransType>& common_pin, AnalysisType analysis_type);
  TimingCheckArc* getEndPointCheckArc(Database& database, std::string& end_point, AnalysisType analysis_type);
  bool isMatchCheckType(TimingCheckArc& timing_check_arc, AnalysisType analysis_type);
  double getClockPeriod(Database& database, std::string& clock_name);
  void propagateRequiredArc(Database& database, Arc& arc);
  void updateSlack(Database& database);
  void analyzeEndPointList(Database& database);
  TimingPathGroup initTimingPathGroup(Database& database);
  std::vector<TimingPath> buildTimingPathList(Database& database, std::string& end_point);
  void buildPathDiversionList(Database& database, std::string& end_point);
  void buildPathDiversionList(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type);
  void buildPathDiversionList(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                              std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                              std::vector<std::size_t>& path_arc_idx_list);
  void buildPathDiversionState(Database& database, AnalysisType analysis_type, PathSourceType source_type,
                               std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                               std::vector<std::size_t>& path_arc_idx_list, std::size_t sink_idx, std::size_t diversion_arc_idx,
                               TransType input_trans_type, TimingPathState& source_path_state);
  bool isOutputTransType(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type);
  bool updateDiversionPathState(Database& database, std::string& pin_name, AnalysisType analysis_type, PathSourceType source_type,
                                TransType trans_type, TimingPathState& source_path_state, std::string& predecessor,
                                std::size_t predecessor_arc_idx, double predecessor_arc_delay, TransType predecessor_trans_type, double arrival);
  TimingPathState* getWorstSlackPathState(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type);
  double calcPathRequiredTime(Database& database, std::string& end_point, TimingPathState& end_path_state, AnalysisType analysis_type);
  double calcPathSlack(TimingPathState& end_path_state, double required_time, AnalysisType analysis_type);
  bool isConstrainedEndPoint(Database& database, std::string& end_point);
  bool isOutputEndPoint(Database& database, std::string& end_point);
  bool hasOutputDelay(Database& database, std::string& end_point);
  bool isRegisterEndPoint(Database& database, std::string& end_point);
  bool isTimingCheckEndPoint(Database& database, std::string& end_point);
  TimingPath buildTimingPath(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                             std::string& start_point);
  void buildPathTrace(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                      std::string& start_point, std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list);
  std::vector<std::size_t> getPathArcIdxList(Database& database, std::vector<std::string>& path_pin_name_list,
                                             std::vector<TransType>& path_trans_type_list, AnalysisType analysis_type, PathSourceType source_type,
                                             std::string& start_point);
  void updatePathDelay(TimingPath& timing_path, Arc* arc, double arc_delay);
  void updateClockInfo(Database& database, TimingPath& timing_path, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                       std::string& start_point);
  TimingPathPoint makeTimingPathPoint(Database& database, std::string& pin_name, Arc* arc, AnalysisType analysis_type, PathSourceType source_type,
                                      TransType input_trans_type, TransType trans_type, std::string& start_point);
  void insertTimingPath(TimingPathGroup& timing_path_group, TimingPath& timing_path);
  TimingPathEnd initTimingPathEnd(std::string& end_point);
  void updateWorstSlack(TimingPath& timing_path, double& worst_slack, std::string& worst_end_point);
  void updateViolation(TimingPath& timing_path, std::size_t& violation_num, double& total_negative_slack);
  std::size_t getTimingPathNum(TimingPathGroup& timing_path_group);
  void updateSummary(Database& database, TimingPathGroup& timing_path_group, std::size_t checked_end_point_num, std::size_t unconstrained_end_point_num,
                     std::size_t violation_num, double worst_slack, double total_negative_slack, std::string& worst_end_point);
};

}  // namespace ista
