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
  void propagate();

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
  void buildArcDelayList();
  void buildArcDelay(Arc& arc);
  void buildAnalysisArcDelay(Arc& arc, AnalysisType analysis_type);
  void buildTransArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  bool isClockArcTriggerTrans(TimingCellArc& timing_cell_arc, TransType input_trans_type);
  double calcArcDelay(Arc& arc);
  double calcCellArcDelay(Arc& arc, AnalysisType analysis_type);
  double calcCellArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  TimingCellArc* getTimingCellArc(Arc& arc);
  double calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type);
  double calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type);
  double calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                TransType output_trans_type);
  double calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                TransType output_trans_type, double input_slew);
  double calcTimingCellArcSlew(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                               TransType output_trans_type, double input_slew);
  double calcTimingCellArcDelay(TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                TransType output_trans_type, double input_slew, double output_load);
  double calcTimingCellArcSlew(TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                               TransType output_trans_type, double input_slew, double output_load);
  TransType getOutputTransType(TimingCellArc& timing_cell_arc, TransType input_trans_type);
  std::vector<TransType> getOutputTransTypeList(TimingCellArc& timing_cell_arc, TransType input_trans_type);
  std::vector<TimingArc*> getCandidateTimingArcList(TimingCellArc& timing_cell_arc, TransType input_trans_type, TransType output_trans_type);
  std::vector<TimingArc*> getCandidateTimingCheckArcList(TimingCheckArc& timing_check_arc, TransType clock_trans_type, TransType data_trans_type);
  bool isMatchTimingType(TimingArc& timing_arc, TransType trans_type);
  bool isPositiveArc(TimingArc& timing_arc);
  bool isNegativeArc(TimingArc& timing_arc);
  bool isUnateArc(TimingCellArc& timing_cell_arc);
  bool isNegativeArc(TimingCellArc& timing_cell_arc);
  bool isTwoTypeSenseArcSet(TimingCellArc& timing_cell_arc);
  bool isMatchTimingType(TimingCellArc& timing_cell_arc, TransType trans_type);
  double convertOutputLoad(TimingArc& timing_arc, double output_load);
  double getArcOutputLoad(Arc& arc, AnalysisType analysis_type, TransType output_trans_type);
  double getOutputPinLoad(std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type);
  double getNetOutputLoad(Net& net, AnalysisType analysis_type, TransType output_trans_type);
  double getPinCapacitance(std::string& pin_name, AnalysisType analysis_type, TransType trans_type);
  double calcNetArcDelay(Arc& arc);
  double calcParasiticDelay(Arc& arc);
  double getParasiticNodeCapacitance(ParasiticNet& parasitic_net, std::string& pin_name);
  double getParasiticTotalResistance(ParasiticNet& parasitic_net);
  void propagateArrival();
  bool isDisableArc(Arc& arc);
  bool shouldStopDataPropagation(Arc& arc);
  bool isSequentialClockPin(std::string& pin_name);
  void initTimingPointList();
  void markClockPointList();
  void markClockPoint(std::string& clock_source);
  void propagateClockArrival();
  void seedClockArrival(std::string& clock_source);
  void propagateClockArrivalArc(std::size_t arc_idx, AnalysisType analysis_type);
  void propagateClockArrivalArc(std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type);
  void updateClockPathState(Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                            TransType input_trans_type, TransType output_trans_type);
  bool hasClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  double getClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  void updateClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, double clock_arrival);
  void updateClockPredecessor(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, TransType predecessor_trans_type, Arc& arc,
                              double arc_delay);
  bool shouldStopClockPropagation(std::string& pin_name);
  double getClockArrival(std::string& pin_name, AnalysisType analysis_type);
  double getClockArrival(std::string& pin_name, AnalysisType analysis_type, TransType trans_type);
  double getClockSlew(std::string& pin_name, AnalysisType analysis_type, TransType trans_type);
  void seedStartPointList();
  void propagateDataSlewDelay();
  void seedDataSlewList();
  void seedDataSlew(std::string& start_point, AnalysisType analysis_type);
  void seedDataSlew(std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  void propagateDataSlewDelayArc(std::size_t arc_idx);
  void propagateDataSlewDelayArc(std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type);
  bool isClockArcTriggerTrans(Arc& arc, TransType input_trans_type);
  void updateDataSlewDelay(Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                           TransType input_trans_type, TransType output_trans_type);
  void updateGraphArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double arc_delay);
  void updateDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, double data_slew);
  bool hasDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  double getDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type);
  bool isBetterDelay(double candidate_delay, double current_delay, AnalysisType analysis_type);
  bool isBetterSlew(double candidate_slew, double current_slew, AnalysisType analysis_type);
  double roundTime(double time);
  double getStartPointArrival(std::string& start_point, AnalysisType analysis_type);
  double getStartPointArrival(std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  bool isClockSourceStartPoint(std::string& start_point);
  TimingClock* getStartPointClock(std::string& start_point);
  double getStartPointClockEdge(std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  double getStartPointSlew(std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  double calcTimingCellArcDelay(std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcTimingCellArcSlew(std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                               TransType input_trans_type, TransType output_trans_type, double input_slew);
  double getStartPointLaunchTime(std::string& start_point, AnalysisType analysis_type);
  double getStartPointLaunchTime(std::string& start_point, AnalysisType analysis_type, TransType trans_type);
  std::string getStartPointCrprClockPin(std::string& start_point);
  TransType getStartPointCrprClockTransType(std::string& start_point);
  TransType getClockTransType(TimingCellArc& timing_cell_arc);
  std::string getClockName(std::string& pin_name);
  std::string getPathStateStartPoint(std::string& start_point);
  void seedPathState(std::string& start_point, AnalysisType analysis_type);
  PathSourceType getStartPointSourceType(std::string& start_point, AnalysisType analysis_type);
  bool hasInputDelay(std::string& start_point, AnalysisType analysis_type);
  bool isInputStartPoint(std::string& start_point);
  bool isRegisterStartPoint(std::string& start_point);
  bool hasClockPoint(std::string& pin_name);
  void propagateArrivalArc(std::size_t arc_idx);
  void propagatePathStateArc(std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type);
  void propagatePathStateArc(std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type, TransType input_trans_type);
  void propagatePathStateArc(std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type, TransType input_trans_type,
                             TransType output_trans_type);
  std::vector<TransType> getOutputTransTypeList(Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  TransType getOutputTransType(Arc& arc, TransType input_trans_type);
  double getArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  double getArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type);
  double calcArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcArcSlew(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double input_slew);
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
  void propagateRequired();
  double resolveRequiredTime();
  void seedEndPointRequired(double required_time);
  double getEndPointRequired(std::string& end_point, double default_required_time, AnalysisType analysis_type);
  double getEndPointRequired(std::string& end_point, double default_required_time, AnalysisType analysis_type,
                             TransType data_trans_type, double data_slew);
  double getEndPointRequired(std::string& start_point, std::string& end_point, double default_required_time,
                             AnalysisType analysis_type, TransType data_trans_type, double data_slew);
  double getEndPointRequired(TimingPathState& end_path_state, std::string& end_point, double default_required_time,
                             AnalysisType analysis_type);
  bool isMatchCheckTransType(TimingCheckArc& timing_check_arc, TransType data_trans_type);
  double getEndPointCheckTime(std::string& end_point, TimingCheckArc& timing_check_arc, AnalysisType analysis_type,
                              TransType data_trans_type, double data_slew);
  double calcTimingCheckArcTime(TimingCheckArc& timing_check_arc, AnalysisType analysis_type, TransType clock_trans_type, TransType data_trans_type,
                                double clock_slew, double data_slew);
  AnalysisType getCaptureAnalysisType(AnalysisType analysis_type);
  TransType getClockTransType(TimingCheckArc& timing_check_arc);
  double getEndPointCaptureTime(std::string& end_point, AnalysisType analysis_type);
  double getEndPointClockArrival(std::string& end_point, AnalysisType analysis_type);
  double getEndPointClockArrival(std::string& end_point, AnalysisType analysis_type, TransType trans_type);
  double getClockReconvergencePessimism(TimingPathState& end_path_state, std::string& end_point, AnalysisType analysis_type,
                                        std::string& common_pin_name);
  double getClockReconvergencePessimism(std::string& start_point, std::string& end_point, AnalysisType analysis_type,
                                        std::string& common_pin_name);
  double getClockReconvergencePessimism(std::pair<std::string, TransType>& launch_crpr_pin, std::string& end_point,
                                        AnalysisType analysis_type, std::string& common_pin_name);
  double getClockCommonPathDelayDelta(std::pair<std::string, TransType>& common_pin, AnalysisType analysis_type);
  void shrinkClockPathToCrprPath(std::vector<std::pair<std::string, TransType>>& clock_path);
  bool isLeafClockDriverPin(std::string& pin_name);
  bool isLeafClockBufferDriverPin(std::vector<std::pair<std::string, TransType>>& clock_path);
  bool hasSingleLeafClockBufferLoad(std::string& pin_name);
  bool isClockRootBufferDriverPin(std::string& pin_name);
  bool isLeafClockBufferDriverPin(std::string& pin_name);
  bool isLeafClockBufferLoadPin(std::string& pin_name);
  bool shouldShrinkLeafClockBufferLoad(std::string& pin_name);
  double getBufferDriveResistance(std::string& pin_name);
  std::vector<std::pair<std::string, TransType>> getClockPathPinList(std::string& clock_pin_name, AnalysisType analysis_type,
                                                                     TransType trans_type);
  double getClockCommonPathArrival(std::pair<std::string, TransType>& common_pin, AnalysisType analysis_type);
  TimingCheckArc* getEndPointCheckArc(std::string& end_point, AnalysisType analysis_type);
  bool isMatchCheckType(TimingCheckArc& timing_check_arc, AnalysisType analysis_type);
  double getClockPeriod(std::string& clock_name);
  void propagateRequiredArc(Arc& arc);
  void updateSlack();
  void analyzeEndPointList();
  TimingPathGroup initTimingPathGroup();
  std::vector<TimingPath> buildTimingPathList(std::string& end_point);
  void buildPathDiversionList(std::string& end_point);
  void buildPathDiversionList(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type);
  void buildPathDiversionList(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                              std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                              std::vector<std::size_t>& path_arc_idx_list);
  void buildPathDiversionState(AnalysisType analysis_type, PathSourceType source_type,
                               std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                               std::vector<std::size_t>& path_arc_idx_list, std::size_t sink_idx, std::size_t diversion_arc_idx,
                               TransType input_trans_type, TimingPathState& source_path_state);
  bool isOutputTransType(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type);
  bool updateDiversionPathState(std::string& pin_name, AnalysisType analysis_type, PathSourceType source_type,
                                TransType trans_type, TimingPathState& source_path_state, std::string& predecessor,
                                std::size_t predecessor_arc_idx, double predecessor_arc_delay, TransType predecessor_trans_type, double arrival);
  TimingPathState* getWorstSlackPathState(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type);
  double calcPathRequiredTime(std::string& end_point, TimingPathState& end_path_state, AnalysisType analysis_type);
  double calcPathSlack(TimingPathState& end_path_state, double required_time, AnalysisType analysis_type);
  bool isConstrainedEndPoint(std::string& end_point);
  bool isOutputEndPoint(std::string& end_point);
  bool hasOutputDelay(std::string& end_point);
  bool isRegisterEndPoint(std::string& end_point);
  bool isTimingCheckEndPoint(std::string& end_point);
  TimingPath buildTimingPath(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                             std::string& start_point);
  void buildPathTrace(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                      std::string& start_point, std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list);
  std::vector<std::size_t> getPathArcIdxList(std::vector<std::string>& path_pin_name_list,
                                             std::vector<TransType>& path_trans_type_list, AnalysisType analysis_type, PathSourceType source_type,
                                             std::string& start_point);
  void updatePathDelay(TimingPath& timing_path, Arc* arc, double arc_delay);
  void updateClockInfo(TimingPath& timing_path, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                       std::string& start_point);
  TimingPathPoint makeTimingPathPoint(std::string& pin_name, Arc* arc, AnalysisType analysis_type, PathSourceType source_type,
                                      TransType input_trans_type, TransType trans_type, std::string& start_point);
  void insertTimingPath(TimingPathGroup& timing_path_group, TimingPath& timing_path);
  TimingPathEnd initTimingPathEnd(std::string& end_point);
  void updateWorstSlack(TimingPath& timing_path, double& worst_slack, std::string& worst_end_point);
  void updateViolation(TimingPath& timing_path, std::size_t& violation_num, double& total_negative_slack);
  std::size_t getTimingPathNum(TimingPathGroup& timing_path_group);
  void updateSummary(TimingPathGroup& timing_path_group, std::size_t checked_end_point_num, std::size_t unconstrained_end_point_num,
                     std::size_t violation_num, double worst_slack, double total_negative_slack, std::string& worst_end_point);
};

}  // namespace ista
