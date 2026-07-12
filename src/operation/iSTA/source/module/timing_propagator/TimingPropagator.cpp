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

#include <Eigen/IterativeLinearSolvers>
#include <Eigen/SparseCore>

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

void TimingPropagator::propagate()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");
  clearParasiticCache();
  buildArcDelayList();
  propagateArrival();
  propagateRequired();
  analyzeEndPointList();
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TimingPropagator* TimingPropagator::_tp_instance = nullptr;

void TimingPropagator::clearParasiticCache()
{
  _parasitic_resistor_map_cache.clear();
  _parasitic_load_map_cache.clear();
  _parasitic_delay_map_cache.clear();
  _parasitic_impulse_map_cache.clear();
  _parasitic_dmp_model_cache.clear();
  _parasitic_dmp_timing_result_cache.clear();
  _parasitic_dmp_driver_result_cache.clear();
  _parasitic_arnoldi_model_cache.clear();
  _parasitic_arnoldi_timing_result_cache.clear();
  _parasitic_arnoldi_driver_result_cache.clear();
}

void TimingPropagator::buildArcDelayList()
{
  Database& database = STADM.getDatabase();
  for (Arc& arc : database.get_arc_list()) {
    buildArcDelay(arc);
  }
}

void TimingPropagator::buildArcDelay(Arc& arc)
{
  buildAnalysisArcDelay(arc, AnalysisType::kMax);
  buildAnalysisArcDelay(arc, AnalysisType::kMin);
  arc.set_delay_max(std::max(arc.get_trans_delay_map()[AnalysisType::kMax][TransType::kRise], arc.get_trans_delay_map()[AnalysisType::kMax][TransType::kFall]));
  arc.set_delay_min(std::min(arc.get_trans_delay_map()[AnalysisType::kMin][TransType::kRise], arc.get_trans_delay_map()[AnalysisType::kMin][TransType::kFall]));
  arc.set_delay(arc.get_delay_max());
}

void TimingPropagator::buildAnalysisArcDelay(Arc& arc, AnalysisType analysis_type)
{
  buildTransArcDelay(arc, analysis_type, TransType::kRise);
  buildTransArcDelay(arc, analysis_type, TransType::kFall);
}

void TimingPropagator::buildTransArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  if (arc.get_type() == ArcType::kNet) {
    double delay = calcNetArcDelay(arc, analysis_type, input_trans_type);
    arc.get_input_output_delay_map()[analysis_type][input_trans_type][input_trans_type] = delay;
    arc.get_trans_delay_map()[analysis_type][input_trans_type] = delay;
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    return;
  }

  TimingCellArc* timing_cell_arc = getTimingCellArc(arc);
  if (timing_cell_arc == nullptr || timing_cell_arc->get_timing_arc_list().empty()) {
    double delay = calcCellArcDelay(arc, analysis_type, input_trans_type);
    arc.get_input_output_delay_map()[analysis_type][input_trans_type][input_trans_type] = delay;
    arc.get_trans_delay_map()[analysis_type][input_trans_type] = delay;
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    return;
  }
  if (!isClockArcTriggerTrans(*timing_cell_arc, input_trans_type)) {
    return;
  }

  for (TransType output_trans_type : getOutputTransTypeList(*timing_cell_arc, input_trans_type)) {
    double output_load = getArcOutputLoad(arc, analysis_type, output_trans_type);
    double delay = calcTimingCellArcDelay(*timing_cell_arc, analysis_type, input_trans_type, output_trans_type, 0.0, output_load);
    arc.get_input_output_delay_map()[analysis_type][input_trans_type][output_trans_type] = delay;
    if (arc.get_trans_delay_map()[analysis_type].count(input_trans_type) == 0
        || (analysis_type == AnalysisType::kMin && delay < arc.get_trans_delay_map()[analysis_type][input_trans_type])
        || (analysis_type == AnalysisType::kMax && delay > arc.get_trans_delay_map()[analysis_type][input_trans_type])) {
      arc.get_trans_delay_map()[analysis_type][input_trans_type] = delay;
      arc.get_trans_type_map()[input_trans_type] = output_trans_type;
    }
  }
}

bool TimingPropagator::isClockArcTriggerTrans(TimingCellArc& timing_cell_arc, TransType input_trans_type)
{
  if (!timing_cell_arc.get_is_clock_arc()) {
    return true;
  }
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    return true;
  }
  TimingArc& timing_arc = timing_cell_arc.get_timing_arc_list().front();
  if (timing_arc.get_trigger_trans_type() == TransType::kFall) {
    return input_trans_type == TransType::kFall;
  }
  if (timing_arc.get_trigger_trans_type() == TransType::kRise) {
    return input_trans_type == TransType::kRise;
  }
  return input_trans_type == TransType::kRise;
}

double TimingPropagator::calcArcDelay(Arc& arc)
{
  if (arc.get_type() == ArcType::kCell) {
    return calcCellArcDelay(arc, AnalysisType::kMax);
  }
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(arc);
  }
  return 0.0;
}

double TimingPropagator::calcCellArcDelay(Arc& arc, AnalysisType analysis_type)
{
  return calcCellArcDelay(arc, analysis_type, TransType::kRise);
}

double TimingPropagator::calcCellArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  if (arc.get_type() == ArcType::kNet) {
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    return calcNetArcDelay(arc, analysis_type, input_trans_type);
  }
  TimingCellArc* timing_cell_arc = getTimingCellArc(arc);
  if (timing_cell_arc != nullptr) {
    return calcTimingCellArcDelay(arc, *timing_cell_arc, analysis_type, input_trans_type);
  }
  arc.get_trans_type_map()[input_trans_type] = input_trans_type;
  return 1.0;
}

TimingCellArc* TimingPropagator::getTimingCellArc(Arc& arc)
{
  Database& database = STADM.getDatabase();
  if (arc.get_timing_cell_arc() != nullptr) {
    return arc.get_timing_cell_arc();
  }
  if (database.get_instance_map().count(arc.get_owner_name()) == 0) {
    return nullptr;
  }
  Instance& instance = database.get_instance_map()[arc.get_owner_name()];
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return nullptr;
  }
  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  for (TimingCellArc& timing_cell_arc : timing_cell.get_cell_arc_list()) {
    if (timing_cell_arc.get_source_port() == arc.get_library_source_port() && timing_cell_arc.get_sink_port() == arc.get_library_sink_port()) {
      return &timing_cell_arc;
    }
  }
  return nullptr;
}

double TimingPropagator::calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type)
{
  return calcTimingCellArcDelay(arc, timing_cell_arc, analysis_type, TransType::kRise);
}

double TimingPropagator::calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  TransType output_trans_type = getOutputTransType(timing_cell_arc, input_trans_type);
  arc.get_trans_type_map()[input_trans_type] = output_trans_type;
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  double input_slew = 0.0;
  double raw_output_load = getArcOutputLoad(arc, analysis_type, output_trans_type);
  std::vector<double> delay_list;
  for (TimingArc* timing_arc : getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type)) {
    if (timing_arc->get_delay_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double delay = calcTimingArcDelay(arc.get_sink_pin(), *timing_arc, analysis_type, output_trans_type, input_slew, raw_output_load);
    delay_list.push_back(delay);
  }
  if (delay_list.empty()) {
    return timing_cell_arc.get_delay();
  }
  std::ranges::sort(delay_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type, TransType output_trans_type)
{
  return calcTimingCellArcDelay(arc, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, 0.0);
}

double TimingPropagator::calcTimingCellArcDelay(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  double raw_output_load = getArcOutputLoad(arc, analysis_type, output_trans_type);
  std::vector<double> delay_list;
  for (TimingArc* timing_arc : getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type)) {
    if (timing_arc->get_delay_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double delay = calcTimingArcDelay(arc.get_sink_pin(), *timing_arc, analysis_type, output_trans_type, input_slew, raw_output_load);
    delay_list.push_back(delay);
  }
  if (delay_list.empty()) {
    return timing_cell_arc.get_delay();
  }
  std::ranges::sort(delay_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcSlew(Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                               TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    return input_slew;
  }
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return input_slew;
  }
  double output_load = getArcOutputLoad(arc, analysis_type, output_trans_type);
  std::vector<TimingArc*> candidate_arc_list = getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type);
  std::vector<double> slew_list;
  for (TimingArc* timing_arc : candidate_arc_list) {
    if (timing_arc->get_slew_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double slew = calcTimingArcSlew(arc.get_sink_pin(), *timing_arc, analysis_type, output_trans_type, input_slew, output_load);
    slew_list.push_back(slew);
  }
  if (slew_list.empty()) {
    return input_slew;
  }
  std::ranges::sort(slew_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return slew_list.back();
  }
  return slew_list.front();
}

double TimingPropagator::calcTimingCellArcDelay(TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                                TransType output_trans_type, double input_slew, double output_load)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  std::vector<double> delay_list;
  for (TimingArc* timing_arc : getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type)) {
    if (timing_arc->get_delay_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double delay = calcTimingArcDelayByLoad(*timing_arc, output_trans_type, input_slew, output_load);
    delay_list.push_back(delay);
  }
  if (delay_list.empty()) {
    return timing_cell_arc.get_delay();
  }
  std::ranges::sort(delay_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcSlew(TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                               TransType output_trans_type, double input_slew, double output_load)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    return input_slew;
  }
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return input_slew;
  }
  std::vector<TimingArc*> candidate_arc_list = getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type);
  std::vector<double> slew_list;
  for (TimingArc* timing_arc : candidate_arc_list) {
    if (timing_arc->get_slew_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double slew = calcTimingArcSlewByLoad(*timing_arc, output_trans_type, input_slew, output_load);
    slew_list.push_back(slew);
  }
  if (slew_list.empty()) {
    return input_slew;
  }
  std::ranges::sort(slew_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return slew_list.back();
  }
  return slew_list.front();
}

TransType TimingPropagator::getOutputTransType(TimingCellArc& timing_cell_arc, TransType input_trans_type)
{
  if (isNegativeArc(timing_cell_arc)) {
    return input_trans_type == TransType::kRise ? TransType::kFall : TransType::kRise;
  }
  return input_trans_type;
}

std::vector<TransType> TimingPropagator::getOutputTransTypeList(TimingCellArc& timing_cell_arc, TransType input_trans_type)
{
  std::vector<TransType> output_trans_type_list;
  if (!isUnateArc(timing_cell_arc) || isTwoTypeSenseArcSet(timing_cell_arc) || timing_cell_arc.get_is_clock_arc()) {
    for (TransType output_trans_type : {TransType::kRise, TransType::kFall}) {
      if (isMatchTimingType(timing_cell_arc, output_trans_type)) {
        output_trans_type_list.push_back(output_trans_type);
      }
    }
    return output_trans_type_list;
  }

  TransType output_trans_type = getOutputTransType(timing_cell_arc, input_trans_type);
  if (isMatchTimingType(timing_cell_arc, output_trans_type)) {
    output_trans_type_list.push_back(output_trans_type);
  }
  return output_trans_type_list;
}

std::vector<TimingArc*> TimingPropagator::getCandidateTimingArcList(TimingCellArc& timing_cell_arc, TransType input_trans_type, TransType output_trans_type)
{
  bool is_flip = input_trans_type != output_trans_type;
  std::vector<TimingArc*> candidate_arc_list;
  for (TimingArc& timing_arc : timing_cell_arc.get_timing_arc_list()) {
    if (is_flip && isPositiveArc(timing_arc)) {
      continue;
    }
    if (!is_flip && isNegativeArc(timing_arc)) {
      continue;
    }
    if (!isMatchTimingType(timing_arc, output_trans_type)) {
      continue;
    }
    candidate_arc_list.push_back(&timing_arc);
  }
  return candidate_arc_list;
}

std::vector<TimingArc*> TimingPropagator::getCandidateTimingCheckArcList(TimingCheckArc& timing_check_arc, TransType clock_trans_type,
                                                                         TransType data_trans_type)
{
  std::vector<TimingArc*> candidate_arc_list;
  for (TimingArc& timing_arc : timing_check_arc.get_timing_arc_list()) {
    if (!isMatchTimingType(timing_arc, data_trans_type)) {
      continue;
    }
    candidate_arc_list.push_back(&timing_arc);
  }
  return candidate_arc_list;
}

bool TimingPropagator::isMatchTimingType(TimingArc& timing_arc, TransType trans_type)
{
  return timing_arc.get_delay_table_map().count(trans_type) > 0 || timing_arc.get_slew_table_map().count(trans_type) > 0
         || timing_arc.get_check_table_map().count(trans_type) > 0;
}

bool TimingPropagator::isPositiveArc(TimingArc& timing_arc)
{
  return timing_arc.get_sense() == TimingArcSense::kPositive || timing_arc.get_sense() == TimingArcSense::kNone;
}

bool TimingPropagator::isNegativeArc(TimingArc& timing_arc)
{
  return timing_arc.get_sense() == TimingArcSense::kNegative;
}

bool TimingPropagator::isUnateArc(TimingCellArc& timing_cell_arc)
{
  for (TimingArc& timing_arc : timing_cell_arc.get_timing_arc_list()) {
    if (timing_arc.get_sense() == TimingArcSense::kNonUnate) {
      return false;
    }
  }
  return true;
}

bool TimingPropagator::isNegativeArc(TimingCellArc& timing_cell_arc)
{
  for (TimingArc& timing_arc : timing_cell_arc.get_timing_arc_list()) {
    if (!isNegativeArc(timing_arc)) {
      return false;
    }
  }
  return true;
}

bool TimingPropagator::isTwoTypeSenseArcSet(TimingCellArc& timing_cell_arc)
{
  bool has_positive = false;
  bool has_negative = false;
  for (TimingArc& timing_arc : timing_cell_arc.get_timing_arc_list()) {
    if (isPositiveArc(timing_arc)) {
      has_positive = true;
    } else if (isNegativeArc(timing_arc)) {
      has_negative = true;
    }
  }
  return has_positive && has_negative;
}

bool TimingPropagator::isMatchTimingType(TimingCellArc& timing_cell_arc, TransType trans_type)
{
  for (TimingArc& timing_arc : timing_cell_arc.get_timing_arc_list()) {
    if (isMatchTimingType(timing_arc, trans_type)) {
      return true;
    }
  }
  return false;
}

double TimingPropagator::convertOutputLoad(TimingArc& timing_arc, double output_load)
{
  if (std::abs(timing_arc.get_cap_unit_scale() - 1.0) < STA_ERROR) {
    return output_load;
  }
  return static_cast<int>(std::ceil(output_load * timing_arc.get_cap_unit_scale()));
}

double TimingPropagator::calcTimingArcDelay(std::string& output_pin, TimingArc& timing_arc, AnalysisType analysis_type, TransType output_trans_type,
                                            double input_slew, double output_load)
{
  ParasiticArnoldiTimingResult& timing_result
      = getParasiticArnoldiTimingResult(output_pin, timing_arc, analysis_type, output_trans_type, input_slew, output_load);
  if (timing_result.get_is_valid()) {
    return timing_result.get_gate_delay();
  }
  ParasiticDmpTimingResult& dmp_timing_result
      = getParasiticDmpTimingResult(output_pin, timing_arc, analysis_type, output_trans_type, input_slew, output_load);
  if (dmp_timing_result.get_is_valid()) {
    return dmp_timing_result.get_gate_delay();
  }
  return calcTimingArcDelayByLoad(timing_arc, output_trans_type, input_slew, output_load);
}

double TimingPropagator::calcTimingArcSlew(std::string& output_pin, TimingArc& timing_arc, AnalysisType analysis_type, TransType output_trans_type,
                                           double input_slew, double output_load)
{
  ParasiticArnoldiTimingResult& timing_result
      = getParasiticArnoldiTimingResult(output_pin, timing_arc, analysis_type, output_trans_type, input_slew, output_load);
  if (timing_result.get_is_valid()) {
    return timing_result.get_driver_slew();
  }
  ParasiticDmpTimingResult& dmp_timing_result
      = getParasiticDmpTimingResult(output_pin, timing_arc, analysis_type, output_trans_type, input_slew, output_load);
  if (dmp_timing_result.get_is_valid()) {
    return dmp_timing_result.get_driver_slew();
  }
  return calcTimingArcSlewByLoad(timing_arc, output_trans_type, input_slew, output_load);
}

double TimingPropagator::calcTimingArcDelayByLoad(TimingArc& timing_arc, TransType output_trans_type, double input_slew, double output_load)
{
  if (timing_arc.get_delay_table_map().count(output_trans_type) == 0) {
    return 0.0;
  }
  double converted_output_load = convertOutputLoad(timing_arc, output_load);
  double delay = timing_arc.get_delay_table_map()[output_trans_type].findValue(input_slew * timing_arc.get_time_unit_scale(), converted_output_load);
  return delay / timing_arc.get_time_unit_scale();
}

double TimingPropagator::calcTimingArcSlewByLoad(TimingArc& timing_arc, TransType output_trans_type, double input_slew, double output_load)
{
  if (timing_arc.get_slew_table_map().count(output_trans_type) == 0) {
    return input_slew;
  }
  double converted_output_load = convertOutputLoad(timing_arc, output_load);
  double slew = timing_arc.get_slew_table_map()[output_trans_type].findValue(input_slew * timing_arc.get_time_unit_scale(), converted_output_load);
  return slew / timing_arc.get_time_unit_scale();
}

double TimingPropagator::calcTimingArcDelayByRawLoad(TimingArc& timing_arc, TransType output_trans_type, double input_slew, double output_load)
{
  if (timing_arc.get_delay_table_map().count(output_trans_type) == 0) {
    return 0.0;
  }
  double converted_output_load = output_load * timing_arc.get_cap_unit_scale();
  double delay = timing_arc.get_delay_table_map()[output_trans_type].findValue(input_slew * timing_arc.get_time_unit_scale(), converted_output_load);
  return delay / timing_arc.get_time_unit_scale();
}

double TimingPropagator::calcTimingArcSlewByRawLoad(TimingArc& timing_arc, TransType output_trans_type, double input_slew, double output_load)
{
  if (timing_arc.get_slew_table_map().count(output_trans_type) == 0) {
    return input_slew;
  }
  double converted_output_load = output_load * timing_arc.get_cap_unit_scale();
  double slew = timing_arc.get_slew_table_map()[output_trans_type].findValue(input_slew * timing_arc.get_time_unit_scale(), converted_output_load);
  return slew / timing_arc.get_time_unit_scale();
}

double TimingPropagator::getArcOutputLoad(Arc& arc, AnalysisType analysis_type, TransType output_trans_type)
{
  Database& database = STADM.getDatabase();
  std::string& sink_pin_name = arc.get_sink_pin();
  if (database.get_pin_map().count(sink_pin_name) == 0) {
    return 0.0;
  }
  Pin& sink_pin = database.get_pin_map()[sink_pin_name];
  if (sink_pin.get_net_name().empty() || database.get_net_map().count(sink_pin.get_net_name()) == 0) {
    return 0.0;
  }
  Net& net = database.get_net_map()[sink_pin.get_net_name()];
  return getNetOutputLoad(net, analysis_type, output_trans_type);
}

double TimingPropagator::getOutputPinLoad(std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(output_pin) == 0) {
    return 0.0;
  }
  Pin& pin = database.get_pin_map()[output_pin];
  if (pin.get_net_name().empty() || database.get_net_map().count(pin.get_net_name()) == 0) {
    return 0.0;
  }
  Net& net = database.get_net_map()[pin.get_net_name()];
  return getNetOutputLoad(net, analysis_type, output_trans_type);
}

double TimingPropagator::getNetOutputLoad(Net& net, AnalysisType analysis_type, TransType output_trans_type)
{
  Database& database = STADM.getDatabase();
  if (database.get_parasitic_library().get_net_map().count(net.get_net_name()) > 0) {
    ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[net.get_net_name()];
    return getParasiticNetOutputLoad(net, parasitic_net, analysis_type, output_trans_type);
  }

  double output_load = 0.0;
  for (std::string& load_pin_name : net.get_load_pin_list()) {
    output_load += getPinCapacitance(load_pin_name, analysis_type, output_trans_type);
  }
  return output_load;
}

double TimingPropagator::getParasiticNetOutputLoad(Net& net, ParasiticNet& parasitic_net, AnalysisType analysis_type, TransType trans_type)
{
  std::string source_node_name = getParasiticNodeName(parasitic_net, net.get_driver_pin());
  if (source_node_name.empty()) {
    return getParasiticTotalLoad(parasitic_net, analysis_type, trans_type);
  }

  buildParasiticDelayMap(parasitic_net, source_node_name, analysis_type, trans_type);
  return _parasitic_load_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type][source_node_name];
}

double TimingPropagator::getPinCapacitance(std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port()) {
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (port_constraint_map.count(pin_name) > 0 && port_constraint_map[pin_name].get_has_load()) {
      return port_constraint_map[pin_name].get_load();
    }
    return 0.0;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return 0.0;
  }
  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  if (timing_cell.get_port_map().count(pin.get_pin_name()) == 0) {
    return 0.0;
  }
  TimingCellPort& timing_cell_port = timing_cell.get_port_map()[pin.get_pin_name()];
  if (timing_cell_port.get_trans_capacitance_map().count(analysis_type) > 0
      && timing_cell_port.get_trans_capacitance_map()[analysis_type].count(trans_type) > 0) {
    return timing_cell_port.get_trans_capacitance_map()[analysis_type][trans_type];
  }
  return timing_cell_port.get_capacitance();
}

double TimingPropagator::calcNetArcDelay(Arc& arc)
{
  return calcNetArcDelay(arc, AnalysisType::kMax, TransType::kRise);
}

double TimingPropagator::calcNetArcDelay(Arc& arc, AnalysisType analysis_type, TransType trans_type)
{
  return calcNetArcDelay(arc, analysis_type, trans_type, std::numeric_limits<double>::quiet_NaN());
}

double TimingPropagator::calcNetArcDelay(Arc& arc, AnalysisType analysis_type, TransType trans_type, double input_slew)
{
  Database& database = STADM.getDatabase();
  if (database.get_parasitic_library().get_net_map().count(arc.get_owner_name()) > 0) {
    return calcParasiticDelay(arc, analysis_type, trans_type, input_slew);
  }
  return 0.0;
}

double TimingPropagator::calcParasiticDelay(Arc& arc)
{
  return calcParasiticDelay(arc, AnalysisType::kMax, TransType::kRise);
}

double TimingPropagator::calcParasiticDelay(Arc& arc, AnalysisType analysis_type, TransType trans_type)
{
  return calcParasiticDelay(arc, analysis_type, trans_type, std::numeric_limits<double>::quiet_NaN());
}

double TimingPropagator::calcParasiticDelay(Arc& arc, AnalysisType analysis_type, TransType trans_type, double input_slew)
{
  Database& database = STADM.getDatabase();
  ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[arc.get_owner_name()];
  std::string source_node_name = getParasiticNodeName(parasitic_net, arc.get_source_pin());
  std::string sink_node_name = getParasiticNodeName(parasitic_net, arc.get_sink_pin());
  if (source_node_name.empty() || sink_node_name.empty()) {
    double source_capacitance = getParasiticNodeCapacitance(parasitic_net, arc.get_source_pin());
    double sink_capacitance = getParasiticNodeCapacitance(parasitic_net, arc.get_sink_pin());
    double resistance = getParasiticTotalResistance(parasitic_net);
    return resistance * (source_capacitance + sink_capacitance) * 0.5 * 1E-3;
  }

  std::optional<double> cached_wire_delay = getParasiticArnoldiCachedWireDelay(arc, analysis_type, trans_type, input_slew);
  if (cached_wire_delay) {
    return *cached_wire_delay;
  }
  cached_wire_delay = getParasiticDmpCachedWireDelay(arc, analysis_type, trans_type, input_slew);
  if (cached_wire_delay) {
    return *cached_wire_delay;
  }
  std::optional<double> input_port_delay = calcParasiticArnoldiInputPortDelay(parasitic_net, source_node_name, sink_node_name, analysis_type, trans_type);
  if (input_port_delay) {
    return *input_port_delay;
  }

  buildParasiticDelayMap(parasitic_net, source_node_name, analysis_type, trans_type);
  if (_parasitic_delay_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type].count(sink_node_name) == 0) {
    return 0.0;
  }
  return _parasitic_delay_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type][sink_node_name];
}

double TimingPropagator::getParasiticNodeCapacitance(ParasiticNet& parasitic_net, std::string& pin_name)
{
  std::string spef_pin_name = pin_name;
  std::replace(spef_pin_name.begin(), spef_pin_name.end(), ':', '/');
  if (parasitic_net.get_node_map().count(spef_pin_name) > 0) {
    return parasitic_net.get_node_map()[spef_pin_name].get_capacitance();
  }
  if (parasitic_net.get_node_map().count(pin_name) > 0) {
    return parasitic_net.get_node_map()[pin_name].get_capacitance();
  }
  return parasitic_net.get_lumped_capacitance();
}

double TimingPropagator::getParasiticNodeLoad(ParasiticNet& parasitic_net, std::string& node_name, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  double node_load = 0.0;
  if (parasitic_net.get_node_map().count(node_name) > 0) {
    node_load += parasitic_net.get_node_map()[node_name].get_capacitance();
  }

  std::string pin_name = getPinNameByParasiticNodeName(node_name);
  if (database.get_pin_map().count(pin_name) > 0) {
    node_load += getPinCapacitance(pin_name, analysis_type, trans_type);
  }
  return node_load;
}

void TimingPropagator::buildParasiticDelayMap(ParasiticNet& parasitic_net, std::string& source_node_name, AnalysisType analysis_type,
                                              TransType trans_type)
{
  std::string& net_name = parasitic_net.get_net_name();
  if (_parasitic_delay_map_cache[net_name][analysis_type][trans_type].count(source_node_name) > 0
      && _parasitic_impulse_map_cache[net_name][analysis_type][trans_type].count(source_node_name) > 0) {
    return;
  }

  if (_parasitic_resistor_map_cache.count(net_name) == 0) {
    buildParasiticResistorMap(parasitic_net, _parasitic_resistor_map_cache[net_name]);
  }
  std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map = _parasitic_resistor_map_cache[net_name];

  std::string parent_node_name;
  std::set<std::string> load_visited_node_set;
  updateParasiticLoadMap(parasitic_net, source_node_name, parent_node_name, resistor_map, load_visited_node_set, analysis_type, trans_type);

  _parasitic_delay_map_cache[net_name][analysis_type][trans_type][source_node_name] = 0.0;
  std::set<std::string> delay_visited_node_set;
  updateParasiticDelayMap(parasitic_net, source_node_name, parent_node_name, resistor_map, delay_visited_node_set, analysis_type, trans_type);

  std::map<std::string, double> load_delay_map;
  std::set<std::string> load_delay_visited_node_set;
  updateParasiticLoadDelayMap(parasitic_net, source_node_name, parent_node_name, resistor_map, load_delay_visited_node_set, analysis_type, trans_type,
                              load_delay_map);

  std::map<std::string, double> beta_map;
  beta_map[source_node_name] = 0.0;
  std::set<std::string> impulse_visited_node_set;
  updateParasiticImpulseMap(parasitic_net, source_node_name, parent_node_name, resistor_map, impulse_visited_node_set, analysis_type, trans_type,
                            load_delay_map, beta_map);
}

double TimingPropagator::updateParasiticLoadMap(ParasiticNet& parasitic_net, std::string& node_name, std::string& parent_node_name,
                                                std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map,
                                                std::set<std::string>& visited_node_set, AnalysisType analysis_type, TransType trans_type)
{
  if (visited_node_set.count(node_name) > 0) {
    return 0.0;
  }
  visited_node_set.insert(node_name);

  double subtree_load = getParasiticNodeLoad(parasitic_net, node_name, analysis_type, trans_type);
  if (resistor_map.count(node_name) == 0) {
    _parasitic_load_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type][node_name] = subtree_load;
    return subtree_load;
  }

  for (std::pair<std::string, double>& next_node_pair : resistor_map[node_name]) {
    if (next_node_pair.first == parent_node_name) {
      continue;
    }
    subtree_load += updateParasiticLoadMap(parasitic_net, next_node_pair.first, node_name, resistor_map, visited_node_set, analysis_type, trans_type);
  }
  _parasitic_load_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type][node_name] = subtree_load;
  return subtree_load;
}

void TimingPropagator::updateParasiticDelayMap(ParasiticNet& parasitic_net, std::string& node_name, std::string& parent_node_name,
                                               std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map,
                                               std::set<std::string>& visited_node_set, AnalysisType analysis_type, TransType trans_type)
{
  if (visited_node_set.count(node_name) > 0) {
    return;
  }
  visited_node_set.insert(node_name);
  if (resistor_map.count(node_name) == 0) {
    return;
  }

  std::string& net_name = parasitic_net.get_net_name();
  for (std::pair<std::string, double>& next_node_pair : resistor_map[node_name]) {
    if (next_node_pair.first == parent_node_name || visited_node_set.count(next_node_pair.first) > 0) {
      continue;
    }
    double node_delay = _parasitic_delay_map_cache[net_name][analysis_type][trans_type][node_name];
    double next_node_load = _parasitic_load_map_cache[net_name][analysis_type][trans_type][next_node_pair.first];
    _parasitic_delay_map_cache[net_name][analysis_type][trans_type][next_node_pair.first] = node_delay + next_node_pair.second * next_node_load * 1E-3;
    updateParasiticDelayMap(parasitic_net, next_node_pair.first, node_name, resistor_map, visited_node_set, analysis_type, trans_type);
  }
}

double TimingPropagator::updateParasiticLoadDelayMap(ParasiticNet& parasitic_net, std::string& node_name, std::string& parent_node_name,
                                                     std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map,
                                                     std::set<std::string>& visited_node_set, AnalysisType analysis_type, TransType trans_type,
                                                     std::map<std::string, double>& load_delay_map)
{
  if (visited_node_set.count(node_name) > 0) {
    return 0.0;
  }
  visited_node_set.insert(node_name);

  double load_delay = getParasiticNodeLoad(parasitic_net, node_name, analysis_type, trans_type)
                      * _parasitic_delay_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type][node_name];
  if (resistor_map.count(node_name) == 0) {
    load_delay_map[node_name] = load_delay;
    return load_delay;
  }

  for (std::pair<std::string, double>& next_node_pair : resistor_map[node_name]) {
    if (next_node_pair.first == parent_node_name) {
      continue;
    }
    load_delay += updateParasiticLoadDelayMap(parasitic_net, next_node_pair.first, node_name, resistor_map, visited_node_set, analysis_type, trans_type,
                                              load_delay_map);
  }
  load_delay_map[node_name] = load_delay;
  return load_delay;
}

void TimingPropagator::updateParasiticImpulseMap(ParasiticNet& parasitic_net, std::string& node_name, std::string& parent_node_name,
                                                 std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map,
                                                 std::set<std::string>& visited_node_set, AnalysisType analysis_type, TransType trans_type,
                                                 std::map<std::string, double>& load_delay_map, std::map<std::string, double>& beta_map)
{
  if (visited_node_set.count(node_name) > 0) {
    return;
  }
  visited_node_set.insert(node_name);

  std::string& net_name = parasitic_net.get_net_name();
  if (resistor_map.count(node_name) > 0) {
    for (std::pair<std::string, double>& next_node_pair : resistor_map[node_name]) {
      if (next_node_pair.first == parent_node_name || visited_node_set.count(next_node_pair.first) > 0) {
        continue;
      }
      beta_map[next_node_pair.first] = beta_map[node_name] + next_node_pair.second * load_delay_map[next_node_pair.first] * 1E-3;
      updateParasiticImpulseMap(parasitic_net, next_node_pair.first, node_name, resistor_map, visited_node_set, analysis_type, trans_type, load_delay_map,
                                beta_map);
    }
  }

  double node_delay = _parasitic_delay_map_cache[net_name][analysis_type][trans_type][node_name];
  double impulse = 2.0 * beta_map[node_name] - std::pow(node_delay, 2);
  _parasitic_impulse_map_cache[net_name][analysis_type][trans_type][node_name] = std::max(0.0, impulse);
}

double TimingPropagator::getParasiticTotalLoad(ParasiticNet& parasitic_net, AnalysisType analysis_type, TransType trans_type)
{
  double total_load = 0.0;
  for (std::pair<const std::string, ParasiticNode>& node_pair : parasitic_net.get_node_map()) {
    std::string node_name = node_pair.first;
    total_load += getParasiticNodeLoad(parasitic_net, node_name, analysis_type, trans_type);
  }
  return total_load;
}

void TimingPropagator::buildParasiticResistorMap(ParasiticNet& parasitic_net,
                                                 std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map)
{
  resistor_map.clear();
  for (ParasiticResistor& parasitic_resistor : parasitic_net.get_resistor_list()) {
    resistor_map[parasitic_resistor.get_source_node()].push_back(
        std::make_pair(parasitic_resistor.get_sink_node(), parasitic_resistor.get_resistance()));
    resistor_map[parasitic_resistor.get_sink_node()].push_back(
        std::make_pair(parasitic_resistor.get_source_node(), parasitic_resistor.get_resistance()));
  }
}

std::string TimingPropagator::getParasiticNodeName(ParasiticNet& parasitic_net, std::string& pin_name)
{
  if (parasitic_net.get_node_map().count(pin_name) > 0) {
    return pin_name;
  }

  std::string spef_pin_name = pin_name;
  std::replace(spef_pin_name.begin(), spef_pin_name.end(), ':', '/');
  if (parasitic_net.get_node_map().count(spef_pin_name) > 0) {
    return spef_pin_name;
  }
  return "";
}

std::string TimingPropagator::getPinNameByParasiticNodeName(std::string& node_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(node_name) > 0) {
    return node_name;
  }

  std::string pin_name = node_name;
  std::replace(pin_name.begin(), pin_name.end(), '/', ':');
  if (database.get_pin_map().count(pin_name) > 0) {
    return pin_name;
  }
  return node_name;
}

ParasiticDmpTimingResult& TimingPropagator::getParasiticDmpTimingResult(std::string& output_pin, TimingArc& timing_arc,
                                                                       AnalysisType analysis_type, TransType output_trans_type,
                                                                       double input_slew, double output_load)
{
  std::string timing_result_key = getParasiticDmpTimingResultKey(output_pin, timing_arc, analysis_type, output_trans_type, input_slew);
  if (_parasitic_dmp_timing_result_cache.count(timing_result_key) == 0) {
    _parasitic_dmp_timing_result_cache[timing_result_key]
        = calcParasiticDmpTimingResult(output_pin, timing_arc, analysis_type, output_trans_type, input_slew, output_load);
  }
  return _parasitic_dmp_timing_result_cache[timing_result_key];
}

std::string TimingPropagator::getParasiticDmpTimingResultKey(std::string& output_pin, TimingArc& timing_arc, AnalysisType analysis_type,
                                                             TransType output_trans_type, double input_slew)
{
  std::stringstream key_stream;
  key_stream << output_pin << "|" << reinterpret_cast<std::uintptr_t>(&timing_arc) << "|" << static_cast<int32_t>(analysis_type) << "|"
             << static_cast<int32_t>(output_trans_type) << "|" << std::setprecision(17) << input_slew;
  return key_stream.str();
}

ParasiticDmpTimingResult TimingPropagator::calcParasiticDmpTimingResult(std::string& output_pin, TimingArc& timing_arc,
                                                                       AnalysisType analysis_type, TransType output_trans_type,
                                                                       double input_slew, double output_load)
{
  ParasiticDmpTimingResult timing_result;
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(output_pin) == 0) {
    return timing_result;
  }
  Pin& output_pin_data = database.get_pin_map()[output_pin];
  if (output_pin_data.get_net_name().empty() || database.get_parasitic_library().get_net_map().count(output_pin_data.get_net_name()) == 0) {
    return timing_result;
  }

  ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[output_pin_data.get_net_name()];
  std::string source_node_name = getParasiticNodeName(parasitic_net, output_pin);
  if (source_node_name.empty()) {
    return timing_result;
  }

  ParasiticDmpModel& dmp_model = getParasiticDmpModel(parasitic_net, source_node_name, analysis_type, output_trans_type);
  if (!dmp_model.get_is_valid()) {
    return timing_result;
  }

  double gate_delay = 0.0;
  double driver_slew = 0.0;
  double effective_capacitance = output_load;
  if (!calcParasiticDmpCeff(timing_arc, output_trans_type, input_slew, dmp_model, gate_delay, driver_slew, effective_capacitance)) {
    return timing_result;
  }

  timing_result.set_is_valid(true);
  timing_result.set_effective_capacitance(effective_capacitance);
  timing_result.set_gate_delay(gate_delay);
  timing_result.set_driver_slew(driver_slew);
  timing_result.get_wire_delay_map()[source_node_name] = 0.0;
  timing_result.get_wire_delay_map()[output_pin] = 0.0;
  timing_result.get_load_slew_map()[source_node_name] = driver_slew;
  timing_result.get_load_slew_map()[output_pin] = driver_slew;

  for (std::pair<const std::string, ParasiticDmpLoadModel>& load_model_pair : dmp_model.get_load_model_map()) {
    std::string load_node_name = load_model_pair.first;
    std::string load_pin_name = getPinNameByParasiticNodeName(load_node_name);
    double wire_delay = 0.0;
    double load_slew = driver_slew;
    if (!calcParasiticDmpLoadDelay(load_model_pair.second, timing_arc, output_trans_type, driver_slew, wire_delay, load_slew)) {
      continue;
    }
    adjustParasiticLoadThreshold(timing_arc, load_pin_name, output_trans_type, wire_delay, load_slew);
    timing_result.get_wire_delay_map()[load_node_name] = wire_delay;
    timing_result.get_wire_delay_map()[load_pin_name] = wire_delay;
    timing_result.get_load_slew_map()[load_node_name] = load_slew;
    timing_result.get_load_slew_map()[load_pin_name] = load_slew;
  }
  cacheParasiticDmpDriverResult(output_pin, analysis_type, output_trans_type, driver_slew, timing_result);
  return timing_result;
}

bool TimingPropagator::calcParasiticDmpCeff(TimingArc& timing_arc, TransType trans_type, double input_slew, ParasiticDmpModel& dmp_model,
                                  double& gate_delay, double& driver_slew, double& effective_capacitance)
{
  if (!initParasiticDmpCeff(timing_arc, trans_type, input_slew, dmp_model)) {
    return false;
  }

  _driver_resistance = calcGateResistance();
  if (!std::isfinite(_driver_resistance) || _driver_resistance < 0.0) {
    return false;
  }
  if (_driver_resistance < kCapacitiveDriverResistance || _pi_resistance < _driver_resistance * 1E-3 || _load_capacitance == 0.0
      || _load_capacitance < _driver_capacitance * 1E-3 || _pi_resistance == 0.0) {
    return calcCap(gate_delay, driver_slew, effective_capacitance);
  }
  if (_driver_capacitance < _load_capacitance * 1E-3) {
    return calcZeroC2(gate_delay, driver_slew, effective_capacitance);
  }
  return calcPi(gate_delay, driver_slew, effective_capacitance);
}

bool TimingPropagator::initParasiticDmpCeff(TimingArc& timing_arc, TransType trans_type, double input_slew, ParasiticDmpModel& dmp_model)
{
  if (!dmp_model.get_is_valid() || timing_arc.get_delay_table_map().count(trans_type) == 0
      || timing_arc.get_slew_table_map().count(trans_type) == 0) {
    return false;
  }

  _timing_arc = &timing_arc;
  _trans_type = trans_type;
  _input_slew = input_slew;
  _driver_capacitance = dmp_model.get_driver_capacitance();
  _pi_resistance = dmp_model.get_pi_resistance();
  _load_capacitance = dmp_model.get_load_capacitance();
  _threshold = getNormalizedThreshold(trans_type == TransType::kFall ? timing_arc.get_output_threshold_pct_fall()
                                                                     : timing_arc.get_output_threshold_pct_rise());
  _lower_threshold = getNormalizedThreshold(trans_type == TransType::kFall ? timing_arc.get_slew_lower_threshold_pct_fall()
                                                                           : timing_arc.get_slew_lower_threshold_pct_rise());
  _upper_threshold = getNormalizedThreshold(trans_type == TransType::kFall ? timing_arc.get_slew_upper_threshold_pct_fall()
                                                                           : timing_arc.get_slew_upper_threshold_pct_rise());
  _slew_derate = timing_arc.get_slew_derate();
  _is_pi = false;
  _is_zero_c2 = false;
  _newton_order = 0;
  _parameter_list.fill(0.0);
  _function_list.fill(0.0);
  _scale_list.fill(0.0);
  _delta_list.fill(0.0);
  _index_list.fill(0);
  for (std::array<double, 3>& row : _jacobian) {
    row.fill(0.0);
  }
  return std::isfinite(_input_slew) && _driver_capacitance >= 0.0 && _pi_resistance >= 0.0 && _load_capacitance >= 0.0
         && _threshold > 0.0 && _threshold < 1.0 && _lower_threshold >= 0.0 && _upper_threshold <= 1.0
         && _upper_threshold > _lower_threshold && _slew_derate > 0.0;
}

double TimingPropagator::getNormalizedThreshold(double threshold)
{
  if (threshold > 1.0) {
    return threshold * 0.01;
  }
  return threshold;
}

double TimingPropagator::calcGateResistance()
{
  double capacitance1 = _driver_capacitance + _load_capacitance;
  double capacitance2 = capacitance1 + kGateResistanceCapacitanceStep;
  double delay1 = 0.0;
  double slew1 = 0.0;
  double delay2 = 0.0;
  double slew2 = 0.0;
  if (!getGateDelaySlew(capacitance1, delay1, slew1) || !getGateDelaySlew(capacitance2, delay2, slew2)) {
    return 0.0;
  }
  return -std::log(_threshold) * std::abs(delay1 - delay2) / (capacitance2 - capacitance1);
}

bool TimingPropagator::getGateDelaySlew(double capacitance, double& gate_delay, double& gate_slew)
{
  if (_timing_arc == nullptr || _timing_arc->get_delay_table_map().count(_trans_type) == 0
      || _timing_arc->get_slew_table_map().count(_trans_type) == 0) {
    return false;
  }
  double converted_slew = _input_slew * _timing_arc->get_time_unit_scale();
  double converted_capacitance = capacitance * _timing_arc->get_cap_unit_scale();
  gate_delay = _timing_arc->get_delay_table_map()[_trans_type].findValue(converted_slew, converted_capacitance)
               / _timing_arc->get_time_unit_scale();
  gate_slew = _timing_arc->get_slew_table_map()[_trans_type].findValue(converted_slew, converted_capacitance)
              / _timing_arc->get_time_unit_scale();
  return std::isfinite(gate_delay) && std::isfinite(gate_slew) && gate_slew >= 0.0;
}

bool TimingPropagator::calcCap(double& gate_delay, double& driver_slew, double& effective_capacitance)
{
  effective_capacitance = _driver_capacitance + _load_capacitance;
  return getGateDelaySlew(effective_capacitance, gate_delay, driver_slew);
}

bool TimingPropagator::calcPi(double& gate_delay, double& driver_slew, double& effective_capacitance)
{
  if (!initPi()) {
    return calcCap(gate_delay, driver_slew, effective_capacitance);
  }

  if (!findDriverParams(_driver_capacitance + _load_capacitance) && !findDriverParams(_driver_capacitance)) {
    return calcCap(gate_delay, driver_slew, effective_capacitance);
  }

  effective_capacitance = _parameter_list[kEffectiveCapacitanceIndex];
  double table_slew = 0.0;
  if (!getGateDelaySlew(effective_capacitance, gate_delay, table_slew)) {
    return false;
  }
  double waveform_delay = 0.0;
  if (!findDriverDelaySlew(waveform_delay, driver_slew)) {
    driver_slew = table_slew;
  }
  return std::isfinite(gate_delay) && std::isfinite(driver_slew);
}

bool TimingPropagator::initPi()
{
  _is_pi = true;
  _is_zero_c2 = false;
  _newton_order = 3;
  double denominator = _pi_resistance * _driver_resistance * _load_capacitance * _driver_capacitance;
  double coefficient = _driver_resistance * (_load_capacitance + _driver_capacitance) + _pi_resistance * _load_capacitance;
  double discriminant = coefficient * coefficient - 4.0 * denominator;
  if (!(denominator > 0.0) || discriminant < 0.0) {
    return false;
  }

  _pi_zero = 1.0 / (_pi_resistance * _load_capacitance);
  _pi_scale = 1.0 / (_driver_resistance * _driver_capacitance);
  double root = std::sqrt(discriminant);
  _pi_pole1 = (coefficient + root) / (2.0 * denominator);
  _pi_pole2 = (coefficient - root) / (2.0 * denominator);
  double pole_product = _pi_pole1 * _pi_pole2;
  if (!(_pi_pole1 > 0.0) || !(_pi_pole2 > 0.0) || !(pole_product > 0.0)
      || std::abs(_pi_pole2 - _pi_pole1) < kTinyNumber) {
    return false;
  }

  _pi_constant2 = _pi_zero / pole_product;
  _pi_constant1 = (1.0 - _pi_constant2 * (_pi_pole1 + _pi_pole2)) / pole_product;
  _pi_residue2 = (_pi_constant1 * _pi_pole1 + _pi_constant2) / (_pi_pole2 - _pi_pole1);
  _pi_residue1 = -_pi_constant1 - _pi_residue2;
  double current_zero = (_load_capacitance + _driver_capacitance)
                        / (_pi_resistance * _load_capacitance * _driver_capacitance);
  _pi_current_constant = current_zero / pole_product;
  _pi_current_residue1 = (current_zero - _pi_pole1) / (_pi_pole1 * (_pi_pole1 - _pi_pole2));
  _pi_current_residue2 = (current_zero - _pi_pole2) / (_pi_pole2 * (_pi_pole2 - _pi_pole1));
  return std::isfinite(_pi_residue1) && std::isfinite(_pi_residue2) && std::isfinite(_pi_current_constant)
         && std::isfinite(_pi_current_residue1) && std::isfinite(_pi_current_residue2);
}

bool TimingPropagator::calcZeroC2(double& gate_delay, double& driver_slew, double& effective_capacitance)
{
  if (!initZeroC2()) {
    return calcCap(gate_delay, driver_slew, effective_capacitance);
  }

  effective_capacitance = _load_capacitance;
  if (findDriverParams(effective_capacitance) && findDriverDelaySlew(gate_delay, driver_slew)) {
    return true;
  }
  return getGateDelaySlew(effective_capacitance, gate_delay, driver_slew);
}

bool TimingPropagator::initZeroC2()
{
  _is_pi = false;
  _is_zero_c2 = true;
  _newton_order = 2;
  if (!(_pi_resistance > 0.0) || !(_load_capacitance > 0.0) || !(_driver_resistance > 0.0)) {
    return false;
  }

  _zero_zero = 1.0 / (_pi_resistance * _load_capacitance);
  _zero_pole = 1.0 / (_load_capacitance * (_driver_resistance + _pi_resistance));
  _zero_scale = _zero_pole / _zero_zero;
  if (!(_zero_scale > 0.0) || !(_zero_pole > 0.0)) {
    return false;
  }
  _zero_constant2 = 1.0 / _zero_scale;
  _zero_constant1 = (_zero_pole - _zero_zero) / (_zero_pole * _zero_pole);
  _zero_residue = -_zero_constant1;
  return std::isfinite(_zero_constant1) && std::isfinite(_zero_constant2) && std::isfinite(_zero_residue);
}

bool TimingPropagator::findDriverParams(double effective_capacitance)
{
  if (_newton_order == 3) {
    _parameter_list[kEffectiveCapacitanceIndex] = effective_capacitance;
  }
  double threshold_delay = 0.0;
  double lower_delay = 0.0;
  double measured_slew = 0.0;
  if (!getGateDelays(effective_capacitance, threshold_delay, lower_delay, measured_slew)) {
    return false;
  }

  double threshold_span = _upper_threshold - _lower_threshold;
  double transition_time = measured_slew / threshold_span;
  double start_time = threshold_delay + std::log(1.0 - _threshold) * _driver_resistance * effective_capacitance
                      - _threshold * transition_time;
  _parameter_list[kTransitionTimeIndex] = transition_time;
  _parameter_list[kStartTimeIndex] = start_time;
  if (!newtonRaphson()) {
    return false;
  }
  _start_time = _parameter_list[kStartTimeIndex];
  _transition_time = _parameter_list[kTransitionTimeIndex];
  return std::isfinite(_start_time) && std::isfinite(_transition_time) && _transition_time > 0.0;
}

bool TimingPropagator::getGateDelays(double effective_capacitance, double& threshold_delay, double& lower_delay, double& measured_slew)
{
  double table_slew = 0.0;
  if (!getGateDelaySlew(effective_capacitance, threshold_delay, table_slew)) {
    return false;
  }
  measured_slew = table_slew * _slew_derate;
  double threshold_span = _upper_threshold - _lower_threshold;
  if (!(threshold_span > 0.0)) {
    return false;
  }
  lower_delay = threshold_delay - measured_slew * (_threshold - _lower_threshold) / threshold_span;
  return measured_slew > 0.0;
}

bool TimingPropagator::newtonRaphson()
{
  for (int32_t iteration = 0; iteration < kMaxNewtonIteration; iteration++) {
    if (!evalDmpEqns()) {
      return false;
    }
    for (int32_t index = 0; index < _newton_order; index++) {
      _delta_list[index] = -_function_list[index];
    }
    if (!decomposeJacobian()) {
      return false;
    }
    solveJacobian();

    bool is_converged = true;
    for (int32_t index = 0; index < _newton_order; index++) {
      if (!std::isfinite(_delta_list[index])
          || std::abs(_delta_list[index]) > std::abs(_parameter_list[index]) * kDriverParameterTolerance) {
        is_converged = false;
      }
      _parameter_list[index] += _delta_list[index];
    }
    if (is_converged) {
      return evalDmpEqns();
    }
  }
  return false;
}

bool TimingPropagator::evalDmpEqns()
{
  if (_is_pi) {
    return evalPiEqns();
  }
  if (_is_zero_c2) {
    return evalOnePoleEqns();
  }
  return false;
}

bool TimingPropagator::evalPiEqns()
{
  double start_time = _parameter_list[kStartTimeIndex];
  double transition_time = _parameter_list[kTransitionTimeIndex];
  double effective_capacitance = _parameter_list[kEffectiveCapacitanceIndex];
  double total_capacitance = _load_capacitance + _driver_capacitance;
  if (!(effective_capacitance > 0.0) || effective_capacitance > total_capacitance || !(transition_time > 0.0)) {
    return false;
  }

  double threshold_delay = 0.0;
  double lower_delay = 0.0;
  double measured_slew = 0.0;
  if (!getGateDelays(effective_capacitance, threshold_delay, lower_delay, measured_slew)) {
    return false;
  }
  double effective_capacitance_time = measured_slew / (_upper_threshold - _lower_threshold);
  effective_capacitance_time = std::min(effective_capacitance_time, 1.4 * transition_time);

  double threshold_voltage = 0.0;
  double lower_voltage = 0.0;
  calcCapacitiveWaveform(threshold_delay, start_time, transition_time, effective_capacitance, threshold_voltage);
  calcCapacitiveWaveform(lower_delay, start_time, transition_time, effective_capacitance, lower_voltage);
  _function_list[kCurrentIndex] = calcPiCurrentDifference(transition_time, effective_capacitance_time, effective_capacitance);
  _function_list[kThresholdVoltageIndex] = threshold_voltage - _threshold;
  _function_list[kLowerVoltageIndex] = lower_voltage - _lower_threshold;

  double exp_pole1 = calcDmpExp(-_pi_pole1 * transition_time);
  double exp_pole2 = calcDmpExp(-_pi_pole2 * transition_time);
  double exp_effective = calcDmpExp(-transition_time / (_driver_resistance * effective_capacitance));
  _jacobian[kCurrentIndex][kStartTimeIndex] = 0.0;
  _jacobian[kCurrentIndex][kTransitionTimeIndex]
      = (-_pi_current_constant * transition_time + _pi_current_residue1 * transition_time * exp_pole1
         - (2.0 * _pi_current_residue1 / _pi_pole1) * (1.0 - exp_pole1)
         + _pi_current_residue2 * transition_time * exp_pole2
         - (2.0 * _pi_current_residue2 / _pi_pole2) * (1.0 - exp_pole2)
         + _driver_resistance * effective_capacitance
               * (transition_time + transition_time * exp_effective
                  - 2.0 * _driver_resistance * effective_capacitance * (1.0 - exp_effective)))
        / (_driver_resistance * transition_time * transition_time * transition_time);
  _jacobian[kCurrentIndex][kEffectiveCapacitanceIndex]
      = (2.0 * _driver_resistance * effective_capacitance - transition_time
         - (2.0 * _driver_resistance * effective_capacitance + transition_time) * exp_effective)
        / (transition_time * transition_time);

  calcCapacitiveWaveformDerivative(lower_delay, start_time, transition_time, effective_capacitance,
                                   _jacobian[kLowerVoltageIndex][kStartTimeIndex],
                                   _jacobian[kLowerVoltageIndex][kTransitionTimeIndex],
                                   _jacobian[kLowerVoltageIndex][kEffectiveCapacitanceIndex]);
  calcCapacitiveWaveformDerivative(threshold_delay, start_time, transition_time, effective_capacitance,
                                   _jacobian[kThresholdVoltageIndex][kStartTimeIndex],
                                   _jacobian[kThresholdVoltageIndex][kTransitionTimeIndex],
                                   _jacobian[kThresholdVoltageIndex][kEffectiveCapacitanceIndex]);
  return std::isfinite(_function_list[kCurrentIndex]) && std::isfinite(_function_list[kThresholdVoltageIndex])
         && std::isfinite(_function_list[kLowerVoltageIndex]);
}

double TimingPropagator::calcPiCurrentDifference(double transition_time, double effective_capacitance_time, double effective_capacitance)
{
  double exp_pole1 = calcDmpExp(-_pi_pole1 * effective_capacitance_time);
  double exp_pole2 = calcDmpExp(-_pi_pole2 * effective_capacitance_time);
  double exp_effective = calcDmpExp(-effective_capacitance_time / (_driver_resistance * effective_capacitance));
  double pi_current = (_pi_current_constant * effective_capacitance_time
                       + (_pi_current_residue1 / _pi_pole1) * (1.0 - exp_pole1)
                       + (_pi_current_residue2 / _pi_pole2) * (1.0 - exp_pole2))
                      / (_driver_resistance * effective_capacitance_time * transition_time);
  double effective_current
      = (_driver_resistance * effective_capacitance * effective_capacitance_time
         - std::pow(_driver_resistance * effective_capacitance, 2) * (1.0 - exp_effective))
        / (_driver_resistance * effective_capacitance_time * transition_time);
  return pi_current - effective_current;
}

bool TimingPropagator::evalOnePoleEqns()
{
  double start_time = _parameter_list[kStartTimeIndex];
  double transition_time = _parameter_list[kTransitionTimeIndex];
  double threshold_delay = 0.0;
  double lower_delay = 0.0;
  double measured_slew = 0.0;
  if (!getGateDelays(_load_capacitance, threshold_delay, lower_delay, measured_slew) || !(transition_time > 0.0)) {
    return false;
  }

  double threshold_voltage = 0.0;
  double lower_voltage = 0.0;
  calcCapacitiveWaveform(threshold_delay, start_time, transition_time, _load_capacitance, threshold_voltage);
  calcCapacitiveWaveform(lower_delay, start_time, transition_time, _load_capacitance, lower_voltage);
  _function_list[kThresholdVoltageIndex] = threshold_voltage - _threshold;
  _function_list[kLowerVoltageIndex] = lower_voltage - _lower_threshold;

  double ignored_capacitance_derivative = 0.0;
  calcCapacitiveWaveformDerivative(lower_delay, start_time, transition_time, _load_capacitance,
                                   _jacobian[kLowerVoltageIndex][kStartTimeIndex],
                                   _jacobian[kLowerVoltageIndex][kTransitionTimeIndex], ignored_capacitance_derivative);
  calcCapacitiveWaveformDerivative(threshold_delay, start_time, transition_time, _load_capacitance,
                                   _jacobian[kThresholdVoltageIndex][kStartTimeIndex],
                                   _jacobian[kThresholdVoltageIndex][kTransitionTimeIndex], ignored_capacitance_derivative);
  return std::isfinite(_function_list[kThresholdVoltageIndex]) && std::isfinite(_function_list[kLowerVoltageIndex]);
}

void TimingPropagator::calcCapacitiveWaveform(double time, double start_time, double transition_time, double capacitance, double& voltage)
{
  double shifted_time = time - start_time;
  if (shifted_time <= 0.0) {
    voltage = 0.0;
  } else if (shifted_time <= transition_time) {
    voltage = calcCapacitiveUnitRamp(shifted_time, capacitance) / transition_time;
  } else {
    voltage = (calcCapacitiveUnitRamp(shifted_time, capacitance)
               - calcCapacitiveUnitRamp(shifted_time - transition_time, capacitance))
              / transition_time;
  }
}

double TimingPropagator::calcCapacitiveUnitRamp(double time, double capacitance)
{
  double time_constant = _driver_resistance * capacitance;
  if (!(time_constant > 0.0)) {
    return time;
  }
  return time - time_constant * (1.0 - calcDmpExp(-time / time_constant));
}

void TimingPropagator::calcCapacitiveWaveformDerivative(double time, double start_time, double transition_time, double capacitance,
                                                         double& start_derivative, double& transition_derivative,
                                                         double& capacitance_derivative)
{
  double shifted_time = time - start_time;
  if (shifted_time <= 0.0) {
    start_derivative = 0.0;
    transition_derivative = 0.0;
    capacitance_derivative = 0.0;
  } else if (shifted_time <= transition_time) {
    start_derivative = -calcCapacitiveUnitRampTimeDerivative(shifted_time, capacitance) / transition_time;
    transition_derivative = -calcCapacitiveUnitRamp(shifted_time, capacitance) / (transition_time * transition_time);
    capacitance_derivative = calcCapacitiveUnitRampCapDerivative(shifted_time, capacitance) / transition_time;
  } else {
    start_derivative = -(calcCapacitiveUnitRampTimeDerivative(shifted_time, capacitance)
                         - calcCapacitiveUnitRampTimeDerivative(shifted_time - transition_time, capacitance))
                       / transition_time;
    transition_derivative
        = -(calcCapacitiveUnitRamp(shifted_time, capacitance) + calcCapacitiveUnitRamp(shifted_time - transition_time, capacitance))
              / (transition_time * transition_time)
          + calcCapacitiveUnitRampTimeDerivative(shifted_time - transition_time, capacitance) / transition_time;
    capacitance_derivative = (calcCapacitiveUnitRampCapDerivative(shifted_time, capacitance)
                              - calcCapacitiveUnitRampCapDerivative(shifted_time - transition_time, capacitance))
                             / transition_time;
  }
}

double TimingPropagator::calcCapacitiveUnitRampTimeDerivative(double time, double capacitance)
{
  double time_constant = _driver_resistance * capacitance;
  if (!(time_constant > 0.0)) {
    return 1.0;
  }
  return 1.0 - calcDmpExp(-time / time_constant);
}

double TimingPropagator::calcCapacitiveUnitRampCapDerivative(double time, double capacitance)
{
  double time_constant = _driver_resistance * capacitance;
  if (!(time_constant > 0.0)) {
    return 0.0;
  }
  return _driver_resistance * ((1.0 + time / time_constant) * calcDmpExp(-time / time_constant) - 1.0);
}

bool TimingPropagator::decomposeJacobian()
{
  for (int32_t row = 0; row < _newton_order; row++) {
    double largest_value = 0.0;
    for (int32_t column = 0; column < _newton_order; column++) {
      largest_value = std::max(largest_value, std::abs(_jacobian[row][column]));
    }
    if (largest_value == 0.0 || !std::isfinite(largest_value)) {
      return false;
    }
    _scale_list[row] = 1.0 / largest_value;
  }

  int32_t last_index = _newton_order - 1;
  for (int32_t column = 0; column < _newton_order; column++) {
    for (int32_t row = 0; row < column; row++) {
      double value = _jacobian[row][column];
      for (int32_t index = 0; index < row; index++) {
        value -= _jacobian[row][index] * _jacobian[index][column];
      }
      _jacobian[row][column] = value;
    }

    double largest_value = 0.0;
    int32_t pivot_row = column;
    for (int32_t row = column; row < _newton_order; row++) {
      double value = _jacobian[row][column];
      for (int32_t index = 0; index < column; index++) {
        value -= _jacobian[row][index] * _jacobian[index][column];
      }
      _jacobian[row][column] = value;
      double scaled_value = _scale_list[row] * std::abs(value);
      if (scaled_value >= largest_value) {
        largest_value = scaled_value;
        pivot_row = row;
      }
    }
    if (column != pivot_row) {
      std::swap(_jacobian[pivot_row], _jacobian[column]);
      _scale_list[pivot_row] = _scale_list[column];
    }
    _index_list[column] = pivot_row;
    if (_jacobian[column][column] == 0.0) {
      _jacobian[column][column] = kTinyNumber;
    }
    if (column != last_index) {
      double inverse_pivot = 1.0 / _jacobian[column][column];
      for (int32_t row = column + 1; row < _newton_order; row++) {
        _jacobian[row][column] *= inverse_pivot;
      }
    }
  }
  return true;
}

void TimingPropagator::solveJacobian()
{
  int32_t first_nonzero = -1;
  for (int32_t row = 0; row < _newton_order; row++) {
    int32_t pivot_row = _index_list[row];
    double value = _delta_list[pivot_row];
    _delta_list[pivot_row] = _delta_list[row];
    if (first_nonzero != -1) {
      for (int32_t column = first_nonzero; column < row; column++) {
        value -= _jacobian[row][column] * _delta_list[column];
      }
    } else if (value != 0.0) {
      first_nonzero = row;
    }
    _delta_list[row] = value;
  }

  for (int32_t row = _newton_order - 1; row >= 0; row--) {
    double value = _delta_list[row];
    for (int32_t column = row + 1; column < _newton_order; column++) {
      value -= _jacobian[row][column] * _delta_list[column];
    }
    _delta_list[row] = value / _jacobian[row][row];
  }
}

bool TimingPropagator::findDriverDelaySlew(double& driver_delay, double& driver_slew)
{
  double upper_bound = getOutputCrossingUpperBound();
  double lower_crossing = 0.0;
  double upper_crossing = 0.0;
  if (!(upper_bound > _start_time) || !findOutputCrossing(_threshold, _start_time, upper_bound, driver_delay)
      || !findOutputCrossing(_lower_threshold, _start_time, driver_delay, lower_crossing)
      || !findOutputCrossing(_upper_threshold, driver_delay, upper_bound, upper_crossing)) {
    return false;
  }
  driver_slew = (upper_crossing - lower_crossing) / _slew_derate;
  return std::isfinite(driver_delay) && std::isfinite(driver_slew) && driver_slew >= 0.0;
}

bool TimingPropagator::findOutputCrossing(double threshold, double lower_time, double upper_time, double& crossing_time)
{
  std::function<void(double, double&, double&)> waveform_function = [this, threshold](double time, double& value, double& derivative) {
    calcOutputWaveform(time, value, derivative);
    value -= threshold;
  };
  return findRoot(waveform_function, lower_time, upper_time, crossing_time);
}

bool TimingPropagator::findRoot(std::function<void(double, double&, double&)>& function, double lower_time, double upper_time, double& root)
{
  double lower_value = 0.0;
  double upper_value = 0.0;
  double ignored_derivative = 0.0;
  function(lower_time, lower_value, ignored_derivative);
  function(upper_time, upper_value, ignored_derivative);
  if (!std::isfinite(lower_value) || !std::isfinite(upper_value) || lower_value * upper_value > 0.0) {
    return false;
  }
  if (lower_value == 0.0) {
    root = lower_time;
    return true;
  }
  if (upper_value == 0.0) {
    root = upper_time;
    return true;
  }
  if (lower_value > 0.0) {
    std::swap(lower_time, upper_time);
  }

  root = 0.5 * (lower_time + upper_time);
  double previous_delta = std::abs(upper_time - lower_time);
  double delta = previous_delta;
  double value = 0.0;
  double derivative = 0.0;
  function(root, value, derivative);
  for (int32_t iteration = 0; iteration < kMaxRootIteration; iteration++) {
    bool use_bisection = derivative == 0.0
                          || (((root - upper_time) * derivative - value) * ((root - lower_time) * derivative - value) > 0.0)
                          || std::abs(2.0 * value) > std::abs(previous_delta * derivative);
    if (use_bisection) {
      previous_delta = delta;
      delta = 0.5 * (upper_time - lower_time);
      root = lower_time + delta;
    } else {
      previous_delta = delta;
      delta = value / derivative;
      root -= delta;
    }
    if (std::abs(delta) <= kThresholdTimeTolerance * std::abs(root)) {
      return true;
    }

    function(root, value, derivative);
    if (!std::isfinite(value) || !std::isfinite(derivative)) {
      return false;
    }
    if (value < 0.0) {
      lower_time = root;
    } else {
      upper_time = root;
    }
  }
  return false;
}

void TimingPropagator::calcOutputWaveform(double time, double& voltage, double& derivative)
{
  double shifted_time = time - _start_time;
  if (shifted_time <= 0.0) {
    voltage = 0.0;
    derivative = 0.0;
    return;
  }

  double unit_voltage = 0.0;
  double unit_derivative = 0.0;
  if (shifted_time <= _transition_time) {
    if (_is_pi) {
      calcPiUnitRamp(shifted_time, unit_voltage, unit_derivative);
    } else {
      calcZeroC2UnitRamp(shifted_time, unit_voltage, unit_derivative);
    }
    voltage = unit_voltage / _transition_time;
    derivative = unit_derivative / _transition_time;
    return;
  }

  double delayed_voltage = 0.0;
  double delayed_derivative = 0.0;
  if (_is_pi) {
    calcPiUnitRamp(shifted_time, unit_voltage, unit_derivative);
    calcPiUnitRamp(shifted_time - _transition_time, delayed_voltage, delayed_derivative);
  } else {
    calcZeroC2UnitRamp(shifted_time, unit_voltage, unit_derivative);
    calcZeroC2UnitRamp(shifted_time - _transition_time, delayed_voltage, delayed_derivative);
  }
  voltage = (unit_voltage - delayed_voltage) / _transition_time;
  derivative = (unit_derivative - delayed_derivative) / _transition_time;
}

void TimingPropagator::calcPiUnitRamp(double time, double& voltage, double& derivative)
{
  double exp_pole1 = calcDmpExp(-_pi_pole1 * time);
  double exp_pole2 = calcDmpExp(-_pi_pole2 * time);
  voltage = _pi_scale * (_pi_constant1 + _pi_constant2 * time + _pi_residue1 * exp_pole1 + _pi_residue2 * exp_pole2);
  derivative = _pi_scale
               * (_pi_constant2 - _pi_residue1 * _pi_pole1 * exp_pole1 - _pi_residue2 * _pi_pole2 * exp_pole2);
}

void TimingPropagator::calcZeroC2UnitRamp(double time, double& voltage, double& derivative)
{
  double exp_pole = calcDmpExp(-_zero_pole * time);
  voltage = _zero_scale * (_zero_constant1 + _zero_constant2 * time + _zero_residue * exp_pole);
  derivative = _zero_scale * (_zero_constant2 - _zero_residue * _zero_pole * exp_pole);
}

double TimingPropagator::getOutputCrossingUpperBound()
{
  if (_is_pi) {
    return _start_time + _transition_time
           + (_load_capacitance + _driver_capacitance) * (_driver_resistance + _pi_resistance) * 2.0;
  }
  if (_is_zero_c2) {
    return _start_time + _transition_time + _load_capacitance * (_driver_resistance + _pi_resistance) * 2.0;
  }
  return _start_time + _transition_time;
}

double TimingPropagator::calcDmpExp(double value)
{
  if (value < -12.0) {
    return 0.0;
  }
  double result = 1.0 + value / 4096.0;
  for (int32_t iteration = 0; iteration < 12; iteration++) {
    result *= result;
  }
  return result;
}


void TimingPropagator::cacheParasiticDmpDriverResult(std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type,
                                                     double driver_slew, ParasiticDmpTimingResult& timing_result)
{
  std::string driver_result_key = getParasiticDmpDriverResultKey(output_pin, analysis_type, output_trans_type, driver_slew);
  _parasitic_dmp_driver_result_cache[driver_result_key] = timing_result;
}

std::string TimingPropagator::getParasiticDmpDriverResultKey(std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type,
                                                             double driver_slew)
{
  std::stringstream key_stream;
  key_stream << output_pin << "|" << static_cast<int32_t>(analysis_type) << "|" << static_cast<int32_t>(output_trans_type) << "|"
             << std::setprecision(17) << driver_slew;
  return key_stream.str();
}

std::optional<double> TimingPropagator::getParasiticDmpCachedWireDelay(Arc& arc, AnalysisType analysis_type, TransType trans_type,
                                                                      double input_slew)
{
  if (!std::isfinite(input_slew)) {
    return std::nullopt;
  }
  std::string& source_pin = arc.get_source_pin();
  std::string driver_result_key = getParasiticDmpDriverResultKey(source_pin, analysis_type, trans_type, input_slew);
  if (_parasitic_dmp_driver_result_cache.count(driver_result_key) == 0) {
    return std::nullopt;
  }
  ParasiticDmpTimingResult& timing_result = _parasitic_dmp_driver_result_cache[driver_result_key];
  std::string& sink_pin = arc.get_sink_pin();
  if (timing_result.get_wire_delay_map().count(sink_pin) == 0) {
    return std::nullopt;
  }
  return timing_result.get_wire_delay_map()[sink_pin];
}

std::optional<double> TimingPropagator::getParasiticDmpCachedLoadSlew(Arc& arc, AnalysisType analysis_type, TransType trans_type,
                                                                     double input_slew)
{
  if (!std::isfinite(input_slew)) {
    return std::nullopt;
  }
  std::string& source_pin = arc.get_source_pin();
  std::string driver_result_key = getParasiticDmpDriverResultKey(source_pin, analysis_type, trans_type, input_slew);
  if (_parasitic_dmp_driver_result_cache.count(driver_result_key) == 0) {
    return std::nullopt;
  }
  ParasiticDmpTimingResult& timing_result = _parasitic_dmp_driver_result_cache[driver_result_key];
  std::string& sink_pin = arc.get_sink_pin();
  if (timing_result.get_load_slew_map().count(sink_pin) == 0) {
    return std::nullopt;
  }
  return timing_result.get_load_slew_map()[sink_pin];
}

ParasiticDmpModel& TimingPropagator::getParasiticDmpModel(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                          AnalysisType analysis_type, TransType trans_type)
{
  std::string dmp_model_key = getParasiticDmpModelKey(parasitic_net, source_node_name, analysis_type, trans_type);
  if (_parasitic_dmp_model_cache.count(dmp_model_key) == 0) {
    _parasitic_dmp_model_cache[dmp_model_key] = buildParasiticDmpModel(parasitic_net, source_node_name, analysis_type, trans_type);
  }
  return _parasitic_dmp_model_cache[dmp_model_key];
}

std::string TimingPropagator::getParasiticDmpModelKey(ParasiticNet& parasitic_net, std::string& source_node_name, AnalysisType analysis_type,
                                                      TransType trans_type)
{
  std::stringstream key_stream;
  key_stream << parasitic_net.get_net_name() << "|" << source_node_name << "|" << static_cast<int32_t>(analysis_type) << "|"
             << static_cast<int32_t>(trans_type);
  return key_stream.str();
}

ParasiticDmpModel TimingPropagator::buildParasiticDmpModel(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                           AnalysisType analysis_type, TransType trans_type)
{
  ParasiticDmpModel dmp_model;
  std::vector<std::string> node_name_list;
  std::vector<int32_t> parent_idx_list;
  std::vector<double> resistance_list;
  std::vector<double> capacitance_list;
  initParasiticArnoldiTree(parasitic_net, source_node_name, analysis_type, trans_type, node_name_list, parent_idx_list, resistance_list,
                           capacitance_list);
  if (node_name_list.empty()) {
    return dmp_model;
  }

  buildParasiticDmpPiModel(dmp_model, parent_idx_list, resistance_list, capacitance_list);
  if (!dmp_model.get_is_valid()) {
    return dmp_model;
  }
  buildParasiticDmpLoadModelList(dmp_model, node_name_list, parent_idx_list, resistance_list, capacitance_list, source_node_name);
  return dmp_model;
}

void TimingPropagator::buildParasiticDmpPiModel(ParasiticDmpModel& dmp_model, std::vector<int32_t>& parent_idx_list,
                                                std::vector<double>& resistance_list, std::vector<double>& capacitance_list)
{
  if (parent_idx_list.size() != resistance_list.size() || resistance_list.size() != capacitance_list.size() || capacitance_list.empty()) {
    return;
  }

  std::vector<double> admittance1_list = capacitance_list;
  std::vector<double> admittance2_list(capacitance_list.size(), 0.0);
  std::vector<double> admittance3_list(capacitance_list.size(), 0.0);
  for (std::size_t node_idx = capacitance_list.size() - 1; node_idx > 0; node_idx--) {
    int32_t parent_idx = parent_idx_list[node_idx];
    if (parent_idx < 0) {
      return;
    }
    double resistance = resistance_list[node_idx];
    double admittance1 = admittance1_list[node_idx];
    double admittance2 = admittance2_list[node_idx];
    double admittance3 = admittance3_list[node_idx];
    admittance1_list[parent_idx] += admittance1;
    admittance2_list[parent_idx] += admittance2 - resistance * admittance1 * admittance1;
    admittance3_list[parent_idx]
        += admittance3 - 2.0 * resistance * admittance1 * admittance2 + resistance * resistance * admittance1 * admittance1 * admittance1;
  }

  double admittance1 = admittance1_list.front();
  double admittance2 = admittance2_list.front();
  double admittance3 = admittance3_list.front();
  double driver_capacitance = 0.0;
  double pi_resistance = 0.0;
  double load_capacitance = 0.0;
  if (admittance2 == 0.0 && admittance3 == 0.0) {
    load_capacitance = admittance1;
  } else {
    if (admittance3 == 0.0 || admittance2 == 0.0) {
      return;
    }
    load_capacitance = admittance2 * admittance2 / admittance3;
    driver_capacitance = admittance1 - load_capacitance;
    pi_resistance = -admittance3 * admittance3 / (admittance2 * admittance2 * admittance2);
  }
  if (!std::isfinite(driver_capacitance) || !std::isfinite(pi_resistance) || !std::isfinite(load_capacitance)
      || driver_capacitance < 0.0 || pi_resistance < 0.0 || load_capacitance < 0.0
      || driver_capacitance + load_capacitance <= 0.0) {
    return;
  }

  dmp_model.set_is_valid(true);
  dmp_model.set_driver_capacitance(driver_capacitance);
  dmp_model.set_pi_resistance(pi_resistance);
  dmp_model.set_load_capacitance(load_capacitance);
}

void TimingPropagator::buildParasiticDmpLoadModelList(ParasiticDmpModel& dmp_model, std::vector<std::string>& node_name_list,
                                                      std::vector<int32_t>& parent_idx_list, std::vector<double>& resistance_list,
                                                      std::vector<double>& capacitance_list, std::string& source_node_name)
{
  Database& database = STADM.getDatabase();
  std::vector<std::vector<double>> moment_list(4, std::vector<double>(node_name_list.size(), 0.0));
  for (int32_t moment_idx = 1; moment_idx < 4; moment_idx++) {
    std::vector<double> branch_current_list(node_name_list.size(), 0.0);
    for (std::size_t node_idx = 0; node_idx < node_name_list.size(); node_idx++) {
      double previous_moment = moment_idx == 1 ? 1.0 : moment_list[moment_idx - 1][node_idx];
      branch_current_list[node_idx] = capacitance_list[node_idx] * previous_moment;
    }
    for (std::size_t node_idx = node_name_list.size() - 1; node_idx > 0; node_idx--) {
      int32_t parent_idx = parent_idx_list[node_idx];
      if (parent_idx < 0) {
        return;
      }
      branch_current_list[parent_idx] += branch_current_list[node_idx];
    }
    moment_list[moment_idx].front() = 0.0;
    for (std::size_t node_idx = 1; node_idx < node_name_list.size(); node_idx++) {
      int32_t parent_idx = parent_idx_list[node_idx];
      moment_list[moment_idx][node_idx] = moment_list[moment_idx][parent_idx] - resistance_list[node_idx] * branch_current_list[node_idx];
    }
  }

  for (std::size_t node_idx = 0; node_idx < node_name_list.size(); node_idx++) {
    std::string& node_name = node_name_list[node_idx];
    if (node_name == source_node_name) {
      continue;
    }
    std::string pin_name = getPinNameByParasiticNodeName(node_name);
    if (database.get_pin_map().count(pin_name) == 0) {
      continue;
    }
    ParasiticDmpLoadModel load_model
        = buildParasiticDmpLoadModel(moment_list[1][node_idx], moment_list[2][node_idx], moment_list[3][node_idx]);
    if (load_model.get_is_valid()) {
      dmp_model.get_load_model_map()[node_name] = load_model;
    }
  }
}

ParasiticDmpLoadModel TimingPropagator::buildParasiticDmpLoadModel(double moment1, double moment2, double moment3)
{
  ParasiticDmpLoadModel load_model;
  if (!std::isfinite(moment1) || !std::isfinite(moment2) || !std::isfinite(moment3) || moment1 == 0.0) {
    return load_model;
  }

  double pole1 = moment3 == 0.0 ? 0.0 : -moment2 / moment3;
  double ratio_denominator = moment2 == 0.0 || moment3 == 0.0 ? 0.0 : moment1 / moment2 - moment2 / moment3;
  double pole2 = 0.0;
  if (pole1 > 0.0 && ratio_denominator != 0.0 && moment2 != 0.0) {
    pole2 = pole1 * (1.0 / moment1 - moment1 / moment2) / ratio_denominator;
  }
  if (!(pole1 > 0.0) || !(pole2 > 0.0) || pole1 == pole2 || ratio_denominator == 0.0 || !std::isfinite(pole1)
      || !std::isfinite(pole2)) {
    pole1 = -1.0 / moment1;
    if (!(pole1 > 0.0) || !std::isfinite(pole1)) {
      return load_model;
    }
    load_model.set_is_valid(true);
    load_model.get_pole_list().push_back(pole1);
    load_model.get_residue_list().push_back(1.0);
    return load_model;
  }

  double residue1 = pole1 * pole1 * (1.0 + moment1 * pole2) / (pole1 - pole2);
  double residue2 = -pole2 * pole2 * (1.0 + moment1 * pole1) / (pole1 - pole2);
  if (!std::isfinite(residue1) || !std::isfinite(residue2)) {
    return load_model;
  }
  if (residue1 < 0.0 && residue2 > 0.0) {
    std::swap(pole1, pole2);
    std::swap(residue1, residue2);
  }
  load_model.set_is_valid(true);
  load_model.get_pole_list().push_back(pole1);
  load_model.get_pole_list().push_back(pole2);
  load_model.get_residue_list().push_back(residue1);
  load_model.get_residue_list().push_back(residue2);
  return load_model;
}

bool TimingPropagator::calcParasiticDmpLoadDelay(ParasiticDmpLoadModel& load_model, TimingArc& timing_arc, TransType trans_type,
                                                 double driver_slew, double& wire_delay, double& load_slew)
{
  if (!load_model.get_is_valid() || load_model.get_pole_list().empty() || load_model.get_residue_list().empty()) {
    return false;
  }
  double pole1 = load_model.get_pole_list().front();
  if (!(pole1 > 0.0)) {
    return false;
  }
  wire_delay = 1.0 / pole1;
  load_slew = driver_slew;
  if (load_model.get_pole_list().size() < 2 || load_model.get_residue_list().size() < 2 || std::abs(driver_slew) <= STA_ERROR) {
    return true;
  }

  double pole2 = load_model.get_pole_list()[1];
  double residue1 = load_model.get_residue_list()[0];
  double residue2 = load_model.get_residue_list()[1];
  if (!(pole2 > 0.0)) {
    return true;
  }
  double threshold = getNormalizedThreshold(timing_arc.get_output_threshold_pct_rise());
  double lower_threshold = getNormalizedThreshold(timing_arc.get_slew_lower_threshold_pct_rise());
  double upper_threshold = getNormalizedThreshold(timing_arc.get_slew_upper_threshold_pct_rise());
  if (trans_type == TransType::kFall) {
    threshold = getNormalizedThreshold(timing_arc.get_output_threshold_pct_fall());
    lower_threshold = getNormalizedThreshold(timing_arc.get_slew_lower_threshold_pct_fall());
    upper_threshold = getNormalizedThreshold(timing_arc.get_slew_upper_threshold_pct_fall());
  }
  double slew_derate = timing_arc.get_slew_derate();
  if (!(upper_threshold > lower_threshold) || !(slew_derate > 0.0)) {
    return true;
  }

  double residue_pole1 = residue1 / (pole1 * pole1);
  double residue_pole2 = residue2 / (pole2 * pole2);
  double constant = residue_pole1 + residue_pole2;
  double transition_time = driver_slew * slew_derate / (upper_threshold - lower_threshold);
  if (!(transition_time > 0.0)) {
    return true;
  }
  double transition_voltage = (transition_time - constant + residue_pole1 * std::exp(-pole1 * transition_time)
                               + residue_pole2 * std::exp(-pole2 * transition_time))
                              / transition_time;
  double threshold_time = calcParasiticDmpLoadTime(threshold, pole1, pole2, residue1, residue2, constant, residue_pole1,
                                                   residue_pole2, transition_time, transition_voltage);
  double lower_time = calcParasiticDmpLoadTime(lower_threshold, pole1, pole2, residue1, residue2, constant, residue_pole1,
                                               residue_pole2, transition_time, transition_voltage);
  double upper_time = calcParasiticDmpLoadTime(upper_threshold, pole1, pole2, residue1, residue2, constant, residue_pole1,
                                               residue_pole2, transition_time, transition_voltage);
  double dmp_wire_delay = threshold_time - transition_time * threshold;
  double dmp_load_slew = (upper_time - lower_time) / slew_derate;
  if (std::isfinite(dmp_wire_delay) && std::isfinite(dmp_load_slew) && dmp_load_slew >= 0.0) {
    wire_delay = dmp_wire_delay;
    load_slew = dmp_load_slew;
  }
  return true;
}

double TimingPropagator::calcParasiticDmpLoadTime(double threshold, double pole1, double pole2, double residue1, double residue2,
                                                  double constant, double residue_pole1, double residue_pole2, double transition_time,
                                                  double transition_voltage)
{
  if (transition_voltage < threshold) {
    double logarithm_argument
        = residue1 * (std::exp(pole1 * transition_time) - 1.0) / ((1.0 - threshold) * pole1 * pole1 * transition_time);
    if (!(logarithm_argument > 0.0)) {
      return transition_time * threshold;
    }
    double time = std::log(logarithm_argument) / pole1;
    double exp_pole1_time = std::exp(-pole1 * time);
    double exp_pole2_time = std::exp(-pole2 * time);
    double exp_pole1_delta = std::exp(-pole1 * (time - transition_time));
    double exp_pole2_delta = std::exp(-pole2 * (time - transition_time));
    double voltage = (transition_time - residue_pole1 * (exp_pole1_delta - exp_pole1_time)
                      - residue_pole2 * (exp_pole2_delta - exp_pole2_time))
                     / transition_time;
    double derivative = (residue1 / pole1 * (exp_pole1_delta - exp_pole1_time)
                         - residue2 / pole2 * (exp_pole2_delta - exp_pole2_time))
                        / transition_time;
    if (std::abs(derivative) <= STA_ERROR) {
      return time;
    }
    return time - (voltage - threshold) / derivative;
  }

  double time = threshold * transition_time / transition_voltage;
  double exp_pole1_time = std::exp(-pole1 * time);
  double exp_pole2_time = std::exp(-pole2 * time);
  double voltage = (time - constant + residue_pole1 * exp_pole1_time + residue_pole2 * exp_pole1_time) / transition_time;
  double derivative = (1.0 - residue1 / pole1 * exp_pole1_time - residue2 / pole2 * exp_pole2_time) / transition_time;
  if (std::abs(derivative) <= STA_ERROR) {
    return time;
  }
  return time - (voltage - threshold) / derivative;
}

ParasiticArnoldiTimingResult& TimingPropagator::getParasiticArnoldiTimingResult(std::string& output_pin, TimingArc& timing_arc,
                                                                                AnalysisType analysis_type, TransType output_trans_type,
                                                                                double input_slew, double output_load)
{
  ParasiticArnoldiTimingResultKey timing_result_key
      = getParasiticArnoldiTimingResultKey(output_pin, timing_arc, analysis_type, output_trans_type, input_slew);
  if (_parasitic_arnoldi_timing_result_cache.count(timing_result_key) == 0) {
    _parasitic_arnoldi_timing_result_cache[timing_result_key]
        = calcParasiticArnoldiTimingResult(output_pin, timing_arc, analysis_type, output_trans_type, input_slew, output_load);
  }
  return _parasitic_arnoldi_timing_result_cache[timing_result_key];
}

ParasiticArnoldiTimingResultKey TimingPropagator::getParasiticArnoldiTimingResultKey(std::string& output_pin, TimingArc& timing_arc,
                                                                                     AnalysisType analysis_type, TransType output_trans_type,
                                                                                     double input_slew)
{
  return std::make_tuple(output_pin, reinterpret_cast<std::uintptr_t>(&timing_arc), analysis_type, output_trans_type, input_slew);
}

ParasiticArnoldiTimingResult TimingPropagator::calcParasiticArnoldiTimingResult(std::string& output_pin, TimingArc& timing_arc,
                                                                                AnalysisType analysis_type, TransType output_trans_type,
                                                                                double input_slew, double output_load)
{
  ParasiticArnoldiTimingResult timing_result;
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(output_pin) == 0) {
    return timing_result;
  }
  Pin& output_pin_data = database.get_pin_map()[output_pin];
  if (output_pin_data.get_net_name().empty() || database.get_parasitic_library().get_net_map().count(output_pin_data.get_net_name()) == 0) {
    return timing_result;
  }

  ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[output_pin_data.get_net_name()];
  std::string source_node_name = getParasiticNodeName(parasitic_net, output_pin);
  if (source_node_name.empty()) {
    return timing_result;
  }

  ParasiticArnoldiModel& arnoldi_model = getParasiticArnoldiModel(parasitic_net, source_node_name, analysis_type, output_trans_type);
  if (!arnoldi_model.get_is_valid() || arnoldi_model.get_order() <= 0 || arnoldi_model.get_total_capacitance() <= 0.0) {
    return timing_result;
  }
  double total_capacitance = arnoldi_model.get_total_capacitance();
  double delay_at_total_capacitance = calcTimingArcDelayByRawLoad(timing_arc, output_trans_type, input_slew, total_capacitance);
  double delay_at_half_capacitance = calcTimingArcDelayByRawLoad(timing_arc, output_trans_type, input_slew, 0.5 * total_capacitance);
  double delay_resistance = 0.0;
  if (total_capacitance > STA_ERROR) {
    delay_resistance = (delay_at_total_capacitance - delay_at_half_capacitance) / (0.5 * total_capacitance);
  }
  if (!(delay_resistance > 0.0)) {
    delay_resistance = 1.0;
  }

  double slew_derate = 1.0;
  double lower_threshold = 0.1;
  double upper_threshold = 0.9;
  double voltage_log = 0.0;
  double min_slew_factor = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
  calcParasiticArnoldiThreshold(timing_arc, output_trans_type, slew_derate, lower_threshold, upper_threshold, voltage_log, min_slew_factor, x1,
                                y1);

  double drive_resistance
      = calcParasiticArnoldiTableResistance(timing_arc, output_trans_type, input_slew, total_capacitance, voltage_log, slew_derate, delay_resistance);
  if (!(drive_resistance > 0.0 && drive_resistance < 100.0)) {
    drive_resistance = 1.0;
  }
  bool bad_drive_resistance = drive_resistance < delay_resistance;
  double driver_ramp = calcParasiticArnoldiTableRamp(timing_arc, output_trans_type, input_slew, drive_resistance, total_capacitance,
                                                     lower_threshold, upper_threshold, voltage_log, slew_derate, min_slew_factor);
  if (!(driver_ramp > 0.0 && driver_ramp < 100.0)) {
    driver_ramp = 0.5;
  }

  ParasiticArnoldiPoleResidue pole_residue = calcParasiticArnoldiPoleResidue(arnoldi_model, drive_resistance);
  if (!pole_residue.get_is_valid()) {
    return timing_result;
  }

  double effective_capacitance = total_capacitance;
  if (!bad_drive_resistance) {
    for (int32_t iter = 0; iter < 3; iter++) {
      effective_capacitance = calcParasiticArnoldiEffectiveCapacitance(driver_ramp, drive_resistance, pole_residue, driver_ramp);
      if (!std::isfinite(effective_capacitance) || effective_capacitance < 1E-8) {
        effective_capacitance = total_capacitance;
      }
      driver_ramp = calcParasiticArnoldiTableRamp(timing_arc, output_trans_type, input_slew, drive_resistance, effective_capacitance,
                                                  lower_threshold, upper_threshold, voltage_log, slew_derate, min_slew_factor);
      if (!(driver_ramp > 0.0 && driver_ramp < 100.0)) {
        driver_ramp = 0.5;
      }
    }
  }

  double table_delay = calcTimingArcDelayByRawLoad(timing_arc, output_trans_type, input_slew, effective_capacitance);
  double reference_time = solveParasiticArnoldiRampTime(1.0 / (drive_resistance * effective_capacitance), driver_ramp, 0.5);
  std::vector<double> delay_list;
  std::vector<double> slew_list;
  for (std::size_t term_idx = 0; term_idx < arnoldi_model.get_term_node_list().size(); term_idx++) {
    double upper_time = 0.0;
    double middle_time = 0.0;
    double lower_time = 0.0;
    solveParasiticArnoldiWaveformTime(driver_ramp, pole_residue, term_idx, upper_threshold, upper_time, 0.5, middle_time, lower_threshold, lower_time);
    delay_list.push_back(middle_time + table_delay - reference_time);
    slew_list.push_back((lower_time - upper_time) / slew_derate);
  }
  if (delay_list.empty() || slew_list.empty()) {
    return timing_result;
  }

  timing_result.set_is_valid(true);
  timing_result.set_effective_capacitance(effective_capacitance);
  timing_result.set_gate_delay(delay_list.front());
  timing_result.set_driver_slew(slew_list.front());
  for (std::size_t term_idx = 0; term_idx < arnoldi_model.get_term_node_list().size(); term_idx++) {
    std::string& term_node_name = arnoldi_model.get_term_node_list()[term_idx];
    double wire_delay = delay_list[term_idx] - delay_list.front();
    double load_slew = slew_list[term_idx];
    std::string pin_name = getPinNameByParasiticNodeName(term_node_name);
    if (term_idx > 0) {
      adjustParasiticLoadThreshold(timing_arc, pin_name, output_trans_type, wire_delay, load_slew);
    }
    timing_result.get_wire_delay_map()[term_node_name] = wire_delay;
    timing_result.get_load_slew_map()[term_node_name] = load_slew;
    timing_result.get_wire_delay_map()[pin_name] = wire_delay;
    timing_result.get_load_slew_map()[pin_name] = load_slew;
  }
  cacheParasiticArnoldiDriverResult(output_pin, analysis_type, output_trans_type, timing_result.get_driver_slew(), timing_result);
  return timing_result;
}

void TimingPropagator::adjustParasiticLoadThreshold(TimingArc& timing_arc, std::string& load_pin, TransType trans_type, double& wire_delay,
                                                    double& load_slew)
{
  TimingCell* load_timing_cell = getThresholdTimingCell(load_pin);
  if (load_timing_cell == nullptr || load_timing_cell->get_library_name() == timing_arc.get_library_name()) {
    return;
  }

  double driver_output_threshold = trans_type == TransType::kFall ? timing_arc.get_output_threshold_pct_fall()
                                                                  : timing_arc.get_output_threshold_pct_rise();
  double driver_slew_lower_threshold = trans_type == TransType::kFall ? timing_arc.get_slew_lower_threshold_pct_fall()
                                                                      : timing_arc.get_slew_lower_threshold_pct_rise();
  double driver_slew_upper_threshold = trans_type == TransType::kFall ? timing_arc.get_slew_upper_threshold_pct_fall()
                                                                      : timing_arc.get_slew_upper_threshold_pct_rise();
  double driver_slew_derate = timing_arc.get_slew_derate();
  double load_input_threshold = getTimingCellInputThreshold(*load_timing_cell, trans_type);
  double load_slew_lower_threshold = getTimingCellSlewLowerThreshold(*load_timing_cell, trans_type);
  double load_slew_upper_threshold = getTimingCellSlewUpperThreshold(*load_timing_cell, trans_type);
  double load_slew_derate = load_timing_cell->get_slew_derate_from_library();

  driver_output_threshold = getNormalizedThreshold(driver_output_threshold);
  driver_slew_lower_threshold = getNormalizedThreshold(driver_slew_lower_threshold);
  driver_slew_upper_threshold = getNormalizedThreshold(driver_slew_upper_threshold);
  load_input_threshold = getNormalizedThreshold(load_input_threshold);
  load_slew_lower_threshold = getNormalizedThreshold(load_slew_lower_threshold);
  load_slew_upper_threshold = getNormalizedThreshold(load_slew_upper_threshold);

  double driver_slew_delta = driver_slew_upper_threshold - driver_slew_lower_threshold;
  double load_slew_delta = load_slew_upper_threshold - load_slew_lower_threshold;
  if (!(driver_slew_delta > STA_ERROR && load_slew_delta > STA_ERROR && driver_slew_derate > STA_ERROR && load_slew_derate > STA_ERROR)) {
    return;
  }

  double wire_delay_delta = load_slew * ((load_input_threshold - driver_output_threshold) / driver_slew_delta);
  if (trans_type == TransType::kFall) {
    wire_delay -= wire_delay_delta;
  } else {
    wire_delay += wire_delay_delta;
  }
  load_slew = load_slew * ((load_slew_delta / load_slew_derate) / (driver_slew_delta / driver_slew_derate));
}

TimingCell* TimingPropagator::getThresholdTimingCell(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(pin_name) == 0) {
    return nullptr;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return nullptr;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (database.get_timing_library().get_cell_map().count(instance.get_cell_name()) == 0) {
    return nullptr;
  }
  return &database.get_timing_library().get_cell_map()[instance.get_cell_name()];
}

double TimingPropagator::getTimingCellSlewLowerThreshold(TimingCell& timing_cell, TransType trans_type)
{
  if (trans_type == TransType::kFall) {
    return timing_cell.get_slew_lower_threshold_pct_fall();
  }
  return timing_cell.get_slew_lower_threshold_pct_rise();
}

double TimingPropagator::getTimingCellSlewUpperThreshold(TimingCell& timing_cell, TransType trans_type)
{
  if (trans_type == TransType::kFall) {
    return timing_cell.get_slew_upper_threshold_pct_fall();
  }
  return timing_cell.get_slew_upper_threshold_pct_rise();
}

double TimingPropagator::getTimingCellInputThreshold(TimingCell& timing_cell, TransType trans_type)
{
  if (trans_type == TransType::kFall) {
    return timing_cell.get_input_threshold_pct_fall();
  }
  return timing_cell.get_input_threshold_pct_rise();
}

void TimingPropagator::cacheParasiticArnoldiDriverResult(std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type,
                                                        double driver_slew, ParasiticArnoldiTimingResult& timing_result)
{
  ParasiticArnoldiDriverResultKey driver_result_key
      = getParasiticArnoldiDriverResultKey(output_pin, analysis_type, output_trans_type, driver_slew);
  _parasitic_arnoldi_driver_result_cache[driver_result_key] = timing_result;
}

ParasiticArnoldiDriverResultKey TimingPropagator::getParasiticArnoldiDriverResultKey(std::string& output_pin, AnalysisType analysis_type,
                                                                                     TransType output_trans_type, double driver_slew)
{
  return std::make_tuple(output_pin, analysis_type, output_trans_type, driver_slew);
}

std::optional<double> TimingPropagator::getParasiticArnoldiCachedWireDelay(Arc& arc, AnalysisType analysis_type, TransType trans_type, double input_slew)
{
  if (!std::isfinite(input_slew)) {
    return std::nullopt;
  }
  std::string& source_pin = arc.get_source_pin();
  ParasiticArnoldiDriverResultKey driver_result_key = getParasiticArnoldiDriverResultKey(source_pin, analysis_type, trans_type, input_slew);
  if (_parasitic_arnoldi_driver_result_cache.count(driver_result_key) == 0) {
    return std::nullopt;
  }
  ParasiticArnoldiTimingResult& timing_result = _parasitic_arnoldi_driver_result_cache[driver_result_key];
  std::string& sink_pin = arc.get_sink_pin();
  if (timing_result.get_wire_delay_map().count(sink_pin) == 0) {
    return std::nullopt;
  }
  return timing_result.get_wire_delay_map()[sink_pin];
}

std::optional<double> TimingPropagator::getParasiticArnoldiCachedLoadSlew(Arc& arc, AnalysisType analysis_type, TransType trans_type, double input_slew)
{
  if (!std::isfinite(input_slew)) {
    return std::nullopt;
  }
  std::string& source_pin = arc.get_source_pin();
  ParasiticArnoldiDriverResultKey driver_result_key = getParasiticArnoldiDriverResultKey(source_pin, analysis_type, trans_type, input_slew);
  if (_parasitic_arnoldi_driver_result_cache.count(driver_result_key) == 0) {
    return std::nullopt;
  }
  ParasiticArnoldiTimingResult& timing_result = _parasitic_arnoldi_driver_result_cache[driver_result_key];
  std::string& sink_pin = arc.get_sink_pin();
  if (timing_result.get_load_slew_map().count(sink_pin) == 0) {
    return std::nullopt;
  }
  return timing_result.get_load_slew_map()[sink_pin];
}

ParasiticArnoldiModel& TimingPropagator::getParasiticArnoldiModel(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                                  AnalysisType analysis_type, TransType trans_type)
{
  ParasiticArnoldiModelKey arnoldi_model_key = getParasiticArnoldiModelKey(parasitic_net, source_node_name, analysis_type, trans_type);
  if (_parasitic_arnoldi_model_cache.count(arnoldi_model_key) == 0) {
    _parasitic_arnoldi_model_cache[arnoldi_model_key] = buildParasiticArnoldiModel(parasitic_net, source_node_name, analysis_type, trans_type);
  }
  return _parasitic_arnoldi_model_cache[arnoldi_model_key];
}

ParasiticArnoldiModelKey TimingPropagator::getParasiticArnoldiModelKey(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                                       AnalysisType analysis_type, TransType trans_type)
{
  return std::make_tuple(parasitic_net.get_net_name(), source_node_name, analysis_type, trans_type);
}

ParasiticArnoldiModel TimingPropagator::buildParasiticArnoldiModel(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                                   AnalysisType analysis_type, TransType trans_type)
{
  ParasiticArnoldiModel arnoldi_model;
  std::vector<std::string> node_name_list;
  std::vector<int32_t> parent_idx_list;
  std::vector<double> resistance_list;
  std::vector<double> capacitance_list;
  initParasiticArnoldiTree(parasitic_net, source_node_name, analysis_type, trans_type, node_name_list, parent_idx_list, resistance_list, capacitance_list);
  if (node_name_list.empty()) {
    return arnoldi_model;
  }

  initParasiticArnoldiTerm(arnoldi_model, node_name_list, source_node_name);
  if (arnoldi_model.get_term_node_list().empty()) {
    return arnoldi_model;
  }

  std::vector<std::size_t> term_point_idx_list;
  for (std::string& term_node_name : arnoldi_model.get_term_node_list()) {
    for (std::size_t node_idx = 0; node_idx < node_name_list.size(); node_idx++) {
      if (node_name_list[node_idx] == term_node_name) {
        term_point_idx_list.push_back(node_idx);
        break;
      }
    }
  }
  if (term_point_idx_list.size() != arnoldi_model.get_term_node_list().size()) {
    return arnoldi_model;
  }

  updateParasiticArnoldiModel(arnoldi_model, parasitic_net, node_name_list, parent_idx_list, resistance_list, capacitance_list,
                              term_point_idx_list);
  return arnoldi_model;
}

void TimingPropagator::initParasiticArnoldiTree(ParasiticNet& parasitic_net, std::string& source_node_name, AnalysisType analysis_type,
                                                TransType trans_type, std::vector<std::string>& node_name_list,
                                                std::vector<int32_t>& parent_idx_list, std::vector<double>& resistance_list,
                                                std::vector<double>& capacitance_list)
{
  std::string& net_name = parasitic_net.get_net_name();
  if (_parasitic_resistor_map_cache.count(net_name) == 0) {
    buildParasiticResistorMap(parasitic_net, _parasitic_resistor_map_cache[net_name]);
  }
  std::map<std::string, std::vector<std::pair<std::string, double>>>& resistor_map = _parasitic_resistor_map_cache[net_name];

  std::set<std::string> visited_node_set;
  node_name_list.push_back(source_node_name);
  parent_idx_list.push_back(-1);
  resistance_list.push_back(0.0);
  capacitance_list.push_back(getParasiticNodeLoad(parasitic_net, source_node_name, analysis_type, trans_type));
  visited_node_set.insert(source_node_name);

  std::vector<std::size_t> stack_node_idx_list;
  std::vector<std::size_t> stack_next_edge_idx_list;
  stack_node_idx_list.push_back(0);
  stack_next_edge_idx_list.push_back(0);
  while (!stack_node_idx_list.empty()) {
    std::size_t node_idx = stack_node_idx_list.back();
    std::string& node_name = node_name_list[node_idx];
    if (resistor_map.count(node_name) == 0) {
      stack_node_idx_list.pop_back();
      stack_next_edge_idx_list.pop_back();
      continue;
    }

    std::vector<std::pair<std::string, double>>& edge_list = resistor_map[node_name];
    std::size_t& edge_idx = stack_next_edge_idx_list.back();
    if (edge_idx >= edge_list.size()) {
      stack_node_idx_list.pop_back();
      stack_next_edge_idx_list.pop_back();
      continue;
    }

    std::pair<std::string, double>& next_node_pair = edge_list[edge_idx];
    edge_idx++;
    std::string& next_node_name = next_node_pair.first;
    if (visited_node_set.count(next_node_name) > 0) {
      continue;
    }

    visited_node_set.insert(next_node_name);
    node_name_list.push_back(next_node_name);
    parent_idx_list.push_back(static_cast<int32_t>(node_idx));
    resistance_list.push_back(next_node_pair.second * 1E-3);
    capacitance_list.push_back(getParasiticNodeLoad(parasitic_net, next_node_name, analysis_type, trans_type));
    stack_node_idx_list.push_back(node_name_list.size() - 1);
    stack_next_edge_idx_list.push_back(0);
  }
}

void TimingPropagator::initParasiticArnoldiTerm(ParasiticArnoldiModel& arnoldi_model, std::vector<std::string>& node_name_list,
                                                std::string& source_node_name)
{
  Database& database = STADM.getDatabase();
  arnoldi_model.get_term_node_list().push_back(source_node_name);
  arnoldi_model.get_term_pin_list().push_back(getPinNameByParasiticNodeName(source_node_name));
  arnoldi_model.get_term_index_map()[source_node_name] = 0;
  arnoldi_model.get_term_index_map()[arnoldi_model.get_term_pin_list().front()] = 0;
  for (std::string& node_name : node_name_list) {
    if (node_name == source_node_name) {
      continue;
    }
    std::string pin_name = getPinNameByParasiticNodeName(node_name);
    if (database.get_pin_map().count(pin_name) == 0) {
      continue;
    }
    std::size_t term_idx = arnoldi_model.get_term_node_list().size();
    arnoldi_model.get_term_node_list().push_back(node_name);
    arnoldi_model.get_term_pin_list().push_back(pin_name);
    arnoldi_model.get_term_index_map()[node_name] = term_idx;
    arnoldi_model.get_term_index_map()[pin_name] = term_idx;
  }
}

void TimingPropagator::updateParasiticArnoldiModel(ParasiticArnoldiModel& arnoldi_model, ParasiticNet& parasitic_net,
                                                   std::vector<std::string>& node_name_list, std::vector<int32_t>& parent_idx_list,
                                                   std::vector<double>& resistance_list, std::vector<double>& capacitance_list,
                                                   std::vector<std::size_t>& term_point_idx_list)
{
  double total_capacitance = 0.0;
  for (double capacitance : capacitance_list) {
    total_capacitance += capacitance;
  }
  if (total_capacitance <= 0.0) {
    return;
  }

  std::size_t node_num = capacitance_list.size();
  int32_t max_order = 5;
  int32_t order = std::min(static_cast<int32_t>(node_num), max_order);
  double sqrt_total_capacitance = std::sqrt(total_capacitance);
  std::vector<double> current_basis_list(node_num, 1.0 / sqrt_total_capacitance);
  std::vector<double> previous_basis_list(node_num, 0.0);
  std::vector<double> diagonal_list(order, 0.0);
  std::vector<double> off_diagonal_list(order, 0.0);
  std::vector<std::vector<double>> projection_list(order, std::vector<double>(term_point_idx_list.size(), 0.0));
  int32_t final_order = order;

  std::map<std::string, std::size_t> node_idx_map;
  for (std::size_t node_idx = 0; node_idx < node_name_list.size(); node_idx++) {
    node_idx_map[node_name_list[node_idx]] = node_idx;
  }
  std::size_t network_resistance_num = 0;
  for (ParasiticResistor& parasitic_resistor : parasitic_net.get_resistor_list()) {
    if (node_idx_map.count(parasitic_resistor.get_source_node()) == 0 || node_idx_map.count(parasitic_resistor.get_sink_node()) == 0
        || parasitic_resistor.get_source_node() == parasitic_resistor.get_sink_node()) {
      continue;
    }
    network_resistance_num++;
  }

  bool has_resistance_loop = network_resistance_num + 1 > node_num;
  Eigen::SparseMatrix<double> conductance_matrix;
  Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper, Eigen::IncompleteCholesky<double>> conductance_solver;
  if (has_resistance_loop) {
    std::vector<Eigen::Triplet<double>> conductance_triplet_list;
    for (ParasiticResistor& parasitic_resistor : parasitic_net.get_resistor_list()) {
      if (node_idx_map.count(parasitic_resistor.get_source_node()) == 0 || node_idx_map.count(parasitic_resistor.get_sink_node()) == 0) {
        continue;
      }
      std::size_t source_idx = node_idx_map[parasitic_resistor.get_source_node()];
      std::size_t sink_idx = node_idx_map[parasitic_resistor.get_sink_node()];
      if (source_idx == sink_idx) {
        continue;
      }
      double conductance = 1.0 / (parasitic_resistor.get_resistance() * 1E-3);
      if (source_idx > 0) {
        conductance_triplet_list.emplace_back(source_idx - 1, source_idx - 1, conductance);
      }
      if (sink_idx > 0) {
        conductance_triplet_list.emplace_back(sink_idx - 1, sink_idx - 1, conductance);
      }
      if (source_idx > 0 && sink_idx > 0) {
        conductance_triplet_list.emplace_back(source_idx - 1, sink_idx - 1, -conductance);
        conductance_triplet_list.emplace_back(sink_idx - 1, source_idx - 1, -conductance);
      }
    }
    conductance_matrix.resize(node_num - 1, node_num - 1);
    conductance_matrix.setFromTriplets(conductance_triplet_list.begin(), conductance_triplet_list.end());
    conductance_solver.compute(conductance_matrix);
    has_resistance_loop = conductance_solver.info() == Eigen::Success;
  }

  for (int32_t order_idx = 0; order_idx < order; order_idx++) {
    updateParasiticArnoldiProjection(arnoldi_model, current_basis_list, term_point_idx_list, order_idx);
    if (arnoldi_model.get_projection_list().empty()) {
      arnoldi_model.set_projection_list(projection_list);
    } else {
      projection_list = arnoldi_model.get_projection_list();
    }

    std::vector<double> response_list(node_num, 0.0);
    if (has_resistance_loop) {
      Eigen::VectorXd current_load_vector(node_num - 1);
      for (std::size_t node_idx = 1; node_idx < node_num; node_idx++) {
        current_load_vector[node_idx - 1] = capacitance_list[node_idx] * current_basis_list[node_idx];
      }
      Eigen::VectorXd response_vector = conductance_solver.solve(current_load_vector);
      for (std::size_t node_idx = 1; node_idx < node_num; node_idx++) {
        response_list[node_idx] = response_vector[node_idx - 1];
      }
    } else {
      std::vector<double> current_load_list(node_num, 0.0);
      for (int32_t node_idx = static_cast<int32_t>(node_num) - 1; node_idx > 0; node_idx--) {
        current_load_list[node_idx] += capacitance_list[node_idx] * current_basis_list[node_idx];
        current_load_list[parent_idx_list[node_idx]] += current_load_list[node_idx];
      }
      current_load_list[0] += capacitance_list[0] * current_basis_list[0];
      for (std::size_t node_idx = 1; node_idx < node_num; node_idx++) {
        response_list[node_idx] = response_list[parent_idx_list[node_idx]] + resistance_list[node_idx] * current_load_list[node_idx];
      }
    }

    double diagonal = 0.0;
    for (std::size_t node_idx = 1; node_idx < node_num; node_idx++) {
      diagonal += current_basis_list[node_idx] * capacitance_list[node_idx] * response_list[node_idx];
    }
    diagonal_list[order_idx] = diagonal;
    if (order_idx == order - 1) {
      break;
    }
    if (diagonal < STA_ERROR) {
      final_order = order_idx + 1;
      break;
    }

    for (std::size_t node_idx = 0; node_idx < node_num; node_idx++) {
      response_list[node_idx] -= diagonal * current_basis_list[node_idx];
      if (order_idx > 0) {
        response_list[node_idx] -= off_diagonal_list[order_idx - 1] * previous_basis_list[node_idx];
      }
    }

    double off_diagonal = 0.0;
    for (std::size_t node_idx = 0; node_idx < node_num; node_idx++) {
      off_diagonal += capacitance_list[node_idx] * response_list[node_idx] * response_list[node_idx];
    }
    if (off_diagonal < STA_ERROR * STA_ERROR) {
      final_order = order_idx + 1;
      break;
    }
    off_diagonal = std::sqrt(off_diagonal);
    off_diagonal_list[order_idx] = off_diagonal;

    previous_basis_list = current_basis_list;
    for (std::size_t node_idx = 0; node_idx < node_num; node_idx++) {
      current_basis_list[node_idx] = response_list[node_idx] / off_diagonal;
    }
  }

  diagonal_list.resize(final_order);
  if (final_order > 1) {
    off_diagonal_list.resize(final_order - 1);
  } else {
    off_diagonal_list.clear();
  }
  projection_list.resize(final_order);
  arnoldi_model.set_is_valid(final_order > 0);
  arnoldi_model.set_order(final_order);
  arnoldi_model.set_total_capacitance(total_capacitance);
  arnoldi_model.set_sqrt_total_capacitance(sqrt_total_capacitance);
  arnoldi_model.set_diagonal_list(diagonal_list);
  arnoldi_model.set_off_diagonal_list(off_diagonal_list);
  arnoldi_model.set_projection_list(projection_list);
}

void TimingPropagator::updateParasiticArnoldiProjection(ParasiticArnoldiModel& arnoldi_model, std::vector<double>& basis_list,
                                                        std::vector<std::size_t>& term_point_idx_list, std::size_t order_idx)
{
  if (arnoldi_model.get_projection_list().size() <= order_idx) {
    arnoldi_model.get_projection_list().resize(order_idx + 1);
  }
  arnoldi_model.get_projection_list()[order_idx].clear();
  for (std::size_t term_point_idx : term_point_idx_list) {
    arnoldi_model.get_projection_list()[order_idx].push_back(basis_list[term_point_idx]);
  }
}

double TimingPropagator::calcParasiticArnoldiElmore(ParasiticArnoldiModel& arnoldi_model, std::string& sink_node_name)
{
  if (arnoldi_model.get_term_index_map().count(sink_node_name) == 0) {
    return 0.0;
  }
  return calcParasiticArnoldiElmore(arnoldi_model, arnoldi_model.get_term_index_map()[sink_node_name]);
}

double TimingPropagator::calcParasiticArnoldiElmore(ParasiticArnoldiModel& arnoldi_model, std::size_t term_idx)
{
  if (!arnoldi_model.get_is_valid() || arnoldi_model.get_order() <= 0 || arnoldi_model.get_diagonal_list().empty()) {
    return 0.0;
  }
  if (arnoldi_model.get_order() == 1 || arnoldi_model.get_projection_list().size() < 2 || arnoldi_model.get_off_diagonal_list().empty()) {
    return arnoldi_model.get_diagonal_list().front();
  }
  if (term_idx >= arnoldi_model.get_projection_list().front().size() || term_idx >= arnoldi_model.get_projection_list()[1].size()
      || std::abs(arnoldi_model.get_projection_list().front().front()) < STA_ERROR) {
    return arnoldi_model.get_diagonal_list().front();
  }
  return arnoldi_model.get_diagonal_list().front()
         + arnoldi_model.get_off_diagonal_list().front() * arnoldi_model.get_projection_list()[1][term_idx]
               / arnoldi_model.get_projection_list().front().front();
}

std::optional<double> TimingPropagator::calcParasiticArnoldiInputPortDelay(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                                          std::string& sink_node_name, AnalysisType analysis_type,
                                                                          TransType trans_type)
{
  Database& database = STADM.getDatabase();
  std::string source_pin_name = getPinNameByParasiticNodeName(source_node_name);
  if (database.get_pin_map().count(source_pin_name) == 0 || !database.get_pin_map()[source_pin_name].get_is_port()) {
    return std::nullopt;
  }
  ParasiticArnoldiModel& arnoldi_model = getParasiticArnoldiModel(parasitic_net, source_node_name, analysis_type, trans_type);
  if (!arnoldi_model.get_is_valid() || arnoldi_model.get_term_index_map().count(sink_node_name) == 0) {
    return std::nullopt;
  }
  double elmore = calcParasiticArnoldiElmore(arnoldi_model, sink_node_name);
  return std::log(2.0) * elmore;
}

std::optional<double> TimingPropagator::calcParasiticArnoldiInputPortSlew(ParasiticNet& parasitic_net, std::string& source_node_name,
                                                                         std::string& sink_node_name, AnalysisType analysis_type,
                                                                         TransType trans_type, double input_slew)
{
  Database& database = STADM.getDatabase();
  std::string source_pin_name = getPinNameByParasiticNodeName(source_node_name);
  if (database.get_pin_map().count(source_pin_name) == 0 || !database.get_pin_map()[source_pin_name].get_is_port()) {
    return std::nullopt;
  }
  ParasiticArnoldiModel& arnoldi_model = getParasiticArnoldiModel(parasitic_net, source_node_name, analysis_type, trans_type);
  if (!arnoldi_model.get_is_valid() || arnoldi_model.get_term_index_map().count(sink_node_name) == 0) {
    return std::nullopt;
  }
  double elmore = calcParasiticArnoldiElmore(arnoldi_model, sink_node_name);
  double abs_input_slew = std::abs(input_slew);
  double output_slew = abs_input_slew + getParasiticArnoldiSlewScale(trans_type) * elmore;
  if (input_slew < 0.0) {
    return -output_slew;
  }
  return output_slew;
}

double TimingPropagator::getParasiticArnoldiSlewScale(TransType trans_type)
{
  double slew_derate = 1.0;
  double lower_threshold = 0.1;
  double upper_threshold = 0.9;
  double voltage_log = 0.0;
  double min_slew_factor = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
  calcParasiticArnoldiThreshold(trans_type, slew_derate, lower_threshold, upper_threshold, voltage_log, min_slew_factor, x1, y1);
  return voltage_log / slew_derate;
}

ParasiticArnoldiPoleResidue TimingPropagator::calcParasiticArnoldiPoleResidue(ParasiticArnoldiModel& arnoldi_model, double drive_resistance)
{
  ParasiticArnoldiPoleResidue pole_residue;
  if (!arnoldi_model.get_is_valid() || arnoldi_model.get_order() <= 0) {
    return pole_residue;
  }

  std::vector<double> diagonal_list = arnoldi_model.get_diagonal_list();
  std::vector<double> off_diagonal_list = arnoldi_model.get_off_diagonal_list();
  diagonal_list.front() += drive_resistance * arnoldi_model.get_total_capacitance();

  std::vector<double> eigenvalue_list;
  std::vector<std::vector<double>> eigenvector_list;
  if (!solveParasiticArnoldiTridiagonalEigen(diagonal_list, off_diagonal_list, eigenvalue_list, eigenvector_list)) {
    return pole_residue;
  }

  int32_t order = arnoldi_model.get_order();
  std::size_t term_num = arnoldi_model.get_term_node_list().size();
  std::vector<double> pole_list(order, 0.0);
  for (int32_t order_idx = 0; order_idx < order; order_idx++) {
    pole_list[order_idx] = 1.0 / std::max(eigenvalue_list[order_idx], 1E-5);
  }

  std::vector<std::vector<double>> residue_list(term_num, std::vector<double>(order, 0.0));
  for (int32_t order_idx = 0; order_idx < order; order_idx++) {
    double source_projection = arnoldi_model.get_sqrt_total_capacitance() * eigenvector_list[order_idx][0];
    for (std::size_t term_idx = 0; term_idx < term_num; term_idx++) {
      double term_projection = 0.0;
      for (int32_t basis_idx = 0; basis_idx < order; basis_idx++) {
        term_projection += eigenvector_list[order_idx][basis_idx] * arnoldi_model.get_projection_list()[basis_idx][term_idx];
      }
      residue_list[term_idx][order_idx] = source_projection * term_projection;
    }
  }

  pole_residue.set_is_valid(true);
  pole_residue.set_order(order);
  pole_residue.set_pole_list(pole_list);
  pole_residue.set_residue_list(residue_list);
  return pole_residue;
}

bool TimingPropagator::solveParasiticArnoldiTridiagonalEigen(std::vector<double>& diagonal_list, std::vector<double>& off_diagonal_list,
                                                             std::vector<double>& eigenvalue_list,
                                                             std::vector<std::vector<double>>& eigenvector_list)
{
  std::size_t order = diagonal_list.size();
  if (order == 0 || order > 32) {
    return false;
  }
  eigenvalue_list = diagonal_list;
  eigenvector_list.assign(order, std::vector<double>(order, 0.0));
  for (std::size_t order_idx = 0; order_idx < order; order_idx++) {
    eigenvector_list[order_idx][order_idx] = 1.0;
  }

  std::vector<double> diagonal_off_list(order, 0.0);
  for (std::size_t order_idx = 0; order_idx + 1 < order; order_idx++) {
    diagonal_off_list[order_idx + 1] = off_diagonal_list[order_idx];
  }

  for (int32_t high_idx = static_cast<int32_t>(order) - 1; high_idx >= 1; high_idx--) {
    int32_t iter_num = 0;
    while (std::abs(diagonal_off_list[high_idx]) > 1E-9) {
      int32_t low_idx = 0;
      if (iter_num++ == 20) {
        return false;
      }
      double g = (eigenvalue_list[high_idx - 1] - eigenvalue_list[high_idx]) / (2.0 * diagonal_off_list[high_idx]);
      double r = std::sqrt(1.0 + g * g);
      g = eigenvalue_list[low_idx] - eigenvalue_list[high_idx]
          + diagonal_off_list[high_idx] / (g + (g < 0.0 ? -r : r));
      double s = 1.0;
      double c = 1.0;
      double p = 0.0;
      int32_t iter_idx = low_idx + 1;
      for (; iter_idx <= high_idx; iter_idx++) {
        double f = s * diagonal_off_list[iter_idx];
        double b = c * diagonal_off_list[iter_idx];
        diagonal_off_list[iter_idx - 1] = r = std::sqrt(f * f + g * g);
        if (r == 0.0) {
          eigenvalue_list[iter_idx - 1] -= p;
          diagonal_off_list[low_idx] = 0.0;
          break;
        }
        s = f / r;
        c = g / r;
        g = eigenvalue_list[iter_idx - 1] - p;
        r = (eigenvalue_list[iter_idx] - g) * s + 2.0 * c * b;
        eigenvalue_list[iter_idx - 1] = g + (p = s * r);
        g = c * r - b;
        for (std::size_t term_idx = 0; term_idx < order; term_idx++) {
          f = eigenvector_list[iter_idx - 1][term_idx];
          eigenvector_list[iter_idx - 1][term_idx] = s * eigenvector_list[iter_idx][term_idx] + c * f;
          eigenvector_list[iter_idx][term_idx] = c * eigenvector_list[iter_idx][term_idx] - s * f;
        }
      }
      if (r == 0.0 && iter_idx <= high_idx) {
        continue;
      }
      eigenvalue_list[high_idx] -= p;
      diagonal_off_list[high_idx] = g;
      diagonal_off_list[low_idx] = 0.0;
    }
  }

  for (std::size_t order_idx = 0; order_idx + 1 < order; order_idx++) {
    std::size_t max_idx = order_idx;
    double max_value = eigenvalue_list[max_idx];
    for (std::size_t next_idx = order_idx + 1; next_idx < order; next_idx++) {
      if (eigenvalue_list[next_idx] > max_value) {
        max_idx = next_idx;
        max_value = eigenvalue_list[next_idx];
      }
    }
    if (max_idx == order_idx) {
      continue;
    }
    std::swap(eigenvalue_list[max_idx], eigenvalue_list[order_idx]);
    for (std::size_t basis_idx = 0; basis_idx < order; basis_idx++) {
      std::swap(eigenvector_list[max_idx][basis_idx], eigenvector_list[order_idx][basis_idx]);
    }
  }
  return true;
}

void TimingPropagator::calcParasiticArnoldiThreshold(TimingArc& timing_arc, TransType trans_type, double& slew_derate, double& lower_threshold,
                                                     double& upper_threshold, double& voltage_log, double& min_slew_factor, double& x1, double& y1)
{
  slew_derate = timing_arc.get_slew_derate();
  if (trans_type == TransType::kFall) {
    lower_threshold = timing_arc.get_slew_lower_threshold_pct_fall();
    upper_threshold = timing_arc.get_slew_upper_threshold_pct_fall();
  } else {
    lower_threshold = timing_arc.get_slew_lower_threshold_pct_rise();
    upper_threshold = timing_arc.get_slew_upper_threshold_pct_rise();
  }
  lower_threshold = getNormalizedThreshold(lower_threshold);
  upper_threshold = getNormalizedThreshold(upper_threshold);
  if (!(lower_threshold > 0.01 && upper_threshold < 0.99 && upper_threshold > lower_threshold && slew_derate > 0.0)) {
    lower_threshold = 0.1;
    upper_threshold = 0.9;
    slew_derate = 0.8;
  }
  voltage_log = std::log(upper_threshold / lower_threshold);
  calcParasiticArnoldiThresholdFactor(lower_threshold, upper_threshold, min_slew_factor, x1, y1);
}

void TimingPropagator::calcParasiticArnoldiThreshold(TransType trans_type, double& slew_derate, double& lower_threshold,
                                                     double& upper_threshold, double& voltage_log, double& min_slew_factor, double& x1, double& y1)
{
  Database& database = STADM.getDatabase();
  TimingLibrary& timing_library = database.get_timing_library();
  slew_derate = timing_library.get_slew_derate_from_library();
  if (trans_type == TransType::kFall) {
    lower_threshold = timing_library.get_slew_lower_threshold_pct_fall();
    upper_threshold = timing_library.get_slew_upper_threshold_pct_fall();
  } else {
    lower_threshold = timing_library.get_slew_lower_threshold_pct_rise();
    upper_threshold = timing_library.get_slew_upper_threshold_pct_rise();
  }
  lower_threshold = getNormalizedThreshold(lower_threshold);
  upper_threshold = getNormalizedThreshold(upper_threshold);
  if (!(lower_threshold > 0.01 && upper_threshold < 0.99 && upper_threshold > lower_threshold && slew_derate > 0.0)) {
    lower_threshold = 0.1;
    upper_threshold = 0.9;
    slew_derate = 0.8;
  }
  voltage_log = std::log(upper_threshold / lower_threshold);
  calcParasiticArnoldiThresholdFactor(lower_threshold, upper_threshold, min_slew_factor, x1, y1);
}

void TimingPropagator::calcParasiticArnoldiThresholdFactor(double lower_threshold, double upper_threshold, double& min_slew_factor, double& x1,
                                                           double& y1)
{
  double upper_log = std::log(1.0 / upper_threshold);
  min_slew_factor = upper_log + calcParasiticArnoldiHInverse((1.0 - upper_threshold) / upper_threshold - upper_log);
  double lower_log = std::log(1.0 / lower_threshold);
  double lower_factor = lower_log + calcParasiticArnoldiHInverse((1.0 - lower_threshold) / lower_threshold - lower_log);
  double lower_ratio = (std::exp(lower_factor) - 1.0) / lower_factor;
  double denominator = std::log(lower_ratio / lower_threshold) - calcParasiticArnoldiHInverse((1.0 - upper_threshold) * lower_factor);
  x1 = (upper_threshold - lower_threshold) / denominator;
  y1 = lower_factor * x1;
}

double TimingPropagator::calcParasiticArnoldiHInverse(double value)
{
  double x = 0.0;
  if (value < 1.0) {
    x = std::sqrt(2.0 * value) + 0.4 * value;
    if (value < 1E-4) {
      return x;
    }
  } else {
    x = value + 1.0;
  }
  for (int32_t iter = 0; iter < 4; iter++) {
    double exp_x = std::exp(-x);
    double function_value = x + exp_x - 1.0 - value;
    x += function_value / (exp_x - 1.0);
  }
  return x;
}

double TimingPropagator::calcParasiticArnoldiTableResistance(TimingArc& timing_arc, TransType output_trans_type, double input_slew,
                                                             double total_capacitance, double voltage_log, double slew_derate,
                                                             double delay_resistance)
{
  if (!(total_capacitance > 0.0 && voltage_log > 0.0)) {
    return 0.0;
  }
  double table_slew = calcTimingArcSlewByRawLoad(timing_arc, output_trans_type, input_slew, total_capacitance);
  double transition_time = slew_derate * table_slew;
  double drive_resistance = transition_time / (voltage_log * total_capacitance);
  if (delay_resistance > 0.0 && drive_resistance > delay_resistance) {
    drive_resistance = delay_resistance;
  }
  return drive_resistance;
}

double TimingPropagator::calcParasiticArnoldiTableRamp(TimingArc& timing_arc, TransType output_trans_type, double input_slew,
                                                       double drive_resistance, double capacitance, double lower_threshold, double upper_threshold,
                                                       double voltage_log, double slew_derate, double min_slew_factor)
{
  if (!(drive_resistance > 0.0 && capacitance > 0.0 && voltage_log > 0.0)) {
    return 0.0;
  }
  double table_slew = calcTimingArcSlewByRawLoad(timing_arc, output_trans_type, input_slew, capacitance);
  double transition_time = slew_derate * table_slew;
  double min_slew = drive_resistance * capacitance * min_slew_factor;
  if (voltage_log * drive_resistance * capacitance >= transition_time) {
    return min_slew;
  }
  double ramp = min_slew + 0.3 * transition_time;
  solveParasiticArnoldiRamp(1.0 / (drive_resistance * capacitance), transition_time, lower_threshold, upper_threshold, ramp);
  return ramp;
}

void TimingPropagator::solveParasiticArnoldiRamp(double pole, double transition_time, double lower_threshold, double upper_threshold, double& ramp)
{
  for (int32_t iter = 0; iter < 5; iter++) {
    double lower_pole_time = 0.0;
    double lower_derivative = 0.0;
    double upper_pole_time = 0.0;
    double upper_derivative = 0.0;
    solveParasiticArnoldiRampPoint(pole * ramp, lower_threshold, lower_pole_time, lower_derivative);
    solveParasiticArnoldiRampPoint(pole * ramp, upper_threshold, upper_pole_time, upper_derivative);
    double function_value = (lower_pole_time - upper_pole_time) / pole - transition_time;
    double derivative = lower_derivative - upper_derivative;
    if (std::abs(derivative) < STA_ERROR) {
      return;
    }
    ramp -= function_value / derivative;
    if (std::abs(function_value) < 1E-6) {
      return;
    }
  }
}

double TimingPropagator::solveParasiticArnoldiRampTime(double pole, double ramp, double voltage)
{
  double pole_ramp = pole * ramp;
  if (pole_ramp > 30.0) {
    return (1.0 + pole_ramp * (1.0 - voltage)) / pole;
  }
  double exp_pole_ramp = std::exp(pole_ramp);
  if ((1.0 - pole_ramp * voltage) * exp_pole_ramp >= 1.0) {
    return std::log((exp_pole_ramp - 1.0) / (pole_ramp * voltage)) / pole;
  }
  return calcParasiticArnoldiHInverse((1.0 - voltage) * pole_ramp) / pole;
}

void TimingPropagator::solveParasiticArnoldiRampPoint(double pole_ramp, double voltage, double& pole_time, double& derivative)
{
  if (pole_ramp > 30.0) {
    pole_time = 1.0 + pole_ramp * (1.0 - voltage);
    derivative = 1.0 - voltage;
    return;
  }
  double exp_pole_ramp = std::exp(pole_ramp);
  if ((1.0 - pole_ramp * voltage) * exp_pole_ramp >= 1.0) {
    pole_time = std::log((exp_pole_ramp - 1.0) / (pole_ramp * voltage));
    derivative = exp_pole_ramp / (exp_pole_ramp - 1.0) - 1.0 / pole_ramp;
    return;
  }
  pole_time = calcParasiticArnoldiHInverse((1.0 - voltage) * pole_ramp);
  derivative = (1.0 - voltage) / (pole_time - (1.0 - voltage) * pole_ramp);
}

double TimingPropagator::calcParasiticArnoldiEffectiveCapacitance(double driver_ramp, double drive_resistance,
                                                                  ParasiticArnoldiPoleResidue& pole_residue,
                                                                  double effective_capacitance_time)
{
  if (!(drive_resistance > 0.0) || pole_residue.get_residue_list().empty()) {
    return 0.0;
  }
  std::vector<double>& pole_list = pole_residue.get_pole_list();
  std::vector<double>& residue_list = pole_residue.get_residue_list().front();
  double integrated_current = 0.0;
  for (std::size_t pole_idx = 0; pole_idx < pole_list.size(); pole_idx++) {
    double pole = pole_list[pole_idx];
    double pole_ramp = pole * driver_ramp;
    double pole_time = pole * effective_capacitance_time;
    double charge = 0.0;
    if (effective_capacitance_time <= driver_ramp) {
      double exp_time = pole_time > 40.0 ? 0.0 : std::exp(-pole_time);
      charge = exp_time - 1.0 + pole_time;
    } else {
      double exp_time = (pole_time - pole_ramp) > 40.0 ? 0.0 : std::exp(-(pole_time - pole_ramp));
      double exp_ramp = pole_ramp > 40.0 ? 0.0 : std::exp(-pole_ramp);
      charge = pole_ramp - (1.0 - exp_ramp) * exp_time;
    }
    charge /= pole_ramp * pole;
    integrated_current += residue_list[pole_idx] * charge;
  }
  integrated_current /= drive_resistance;

  double voltage = calcParasiticArnoldiWaveformVoltage(effective_capacitance_time, driver_ramp, pole_list, residue_list);
  if (std::abs(1.0 - voltage) < STA_ERROR) {
    return 0.0;
  }
  return integrated_current / (1.0 - voltage);
}

void TimingPropagator::solveParasiticArnoldiWaveformTime(double driver_ramp, ParasiticArnoldiPoleResidue& pole_residue, std::size_t term_idx,
                                                         double voltage, double& waveform_time)
{
  std::vector<double>& pole_list = pole_residue.get_pole_list();
  std::vector<double>& residue_list = pole_residue.get_residue_list()[term_idx];
  int32_t order = pole_residue.get_order();
  while (order > 1 && residue_list[order - 1] < 1E-8 && residue_list[order - 1] > -1E-8) {
    order--;
  }

  int32_t pole_idx = 0;
  if (residue_list[0] < 0.5) {
    for (int32_t idx = 1; idx < order; idx++) {
      if (residue_list[idx] > 0.3 && residue_list[idx] > residue_list[0]) {
        pole_idx = idx;
        break;
      }
    }
  }

  double waveform_at_ramp = 0.0;
  for (int32_t idx = 0; idx < order; idx++) {
    double pole_ramp = pole_list[idx] * driver_ramp;
    waveform_at_ramp += residue_list[idx] * (1.0 - std::exp(-pole_ramp)) / pole_ramp;
  }

  double lower_time = 0.0;
  double upper_time = 0.0;
  double lower_voltage = 0.0;
  double upper_voltage = 0.0;
  if (waveform_at_ramp < voltage) {
    double candidate_time = 0.5 * (1.0 + voltage) * driver_ramp;
    double candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    if (candidate_voltage < voltage) {
      upper_time = candidate_time;
      upper_voltage = candidate_voltage;
      candidate_time = voltage * driver_ramp;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      lower_time = candidate_time;
      lower_voltage = candidate_voltage;
    } else {
      lower_time = candidate_time;
      lower_voltage = candidate_voltage;
      candidate_time = driver_ramp;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      while (candidate_voltage > voltage) {
        lower_time = candidate_time;
        lower_voltage = candidate_voltage;
        candidate_time *= 2.0;
        candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      }
      upper_time = candidate_time;
      upper_voltage = candidate_voltage;
    }
  } else {
    double candidate_time = driver_ramp;
    double candidate_voltage = waveform_at_ramp;
    double search_pole = std::min(pole_list[pole_idx], 10.0);
    while (candidate_voltage >= voltage) {
      lower_time = candidate_time;
      lower_voltage = candidate_voltage;
      candidate_time += 1.0 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    }
    upper_time = candidate_time;
    upper_voltage = candidate_voltage;
  }
  waveform_time = solveParasiticArnoldiBracketedTime(driver_ramp, pole_list, residue_list, voltage, lower_time, upper_time, lower_voltage, upper_voltage);
}

void TimingPropagator::solveParasiticArnoldiWaveformTime(double driver_ramp, ParasiticArnoldiPoleResidue& pole_residue, std::size_t term_idx,
                                                         double upper_threshold, double& upper_time, double mid_threshold, double& mid_time,
                                                         double lower_threshold, double& lower_time)
{
  std::vector<double>& source_pole_list = pole_residue.get_pole_list();
  std::vector<double>& source_residue_list = pole_residue.get_residue_list()[term_idx];
  int32_t order = pole_residue.get_order();
  while (order > 1 && source_residue_list[order - 1] < 1E-8 && source_residue_list[order - 1] > -1E-8) {
    order--;
  }
  if (order <= 0) {
    upper_time = 0.0;
    mid_time = 0.0;
    lower_time = 0.0;
    return;
  }

  std::vector<double> pole_list;
  std::vector<double> residue_list;
  for (int32_t idx = 0; idx < order; idx++) {
    pole_list.push_back(source_pole_list[idx]);
    residue_list.push_back(source_residue_list[idx]);
  }

  int32_t pole_idx = 0;
  if (residue_list[0] < 0.5) {
    for (int32_t idx = 1; idx < order; idx++) {
      if (residue_list[idx] > 0.3 && residue_list[idx] > residue_list[0]) {
        pole_idx = idx;
        break;
      }
    }
  }
  double search_pole = std::min(pole_list[pole_idx], 10.0);
  double waveform_at_ramp = 0.0;
  for (int32_t idx = 0; idx < order; idx++) {
    double pole_ramp = pole_list[idx] * driver_ramp;
    waveform_at_ramp += residue_list[idx] * (1.0 - std::exp(-pole_ramp)) / pole_ramp;
  }

  double hi_min_time = 0.0;
  double hi_max_time = 0.0;
  double hi_min_voltage = 0.0;
  double hi_max_voltage = 0.0;
  double mid_min_time = 0.0;
  double mid_max_time = 0.0;
  double mid_min_voltage = 0.0;
  double mid_max_voltage = 0.0;
  double lo_min_time = 0.0;
  double lo_max_time = 0.0;
  double lo_min_voltage = 0.0;
  double lo_max_voltage = 0.0;

  if (waveform_at_ramp < lower_threshold) {
    lo_max_time = driver_ramp;
    lo_max_voltage = waveform_at_ramp;
    double candidate_time = upper_threshold * driver_ramp;
    double candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    if (candidate_voltage < mid_threshold) {
      hi_max_time = candidate_time;
      mid_max_time = candidate_time;
      lo_min_time = candidate_time;
      hi_max_voltage = candidate_voltage;
      mid_max_voltage = candidate_voltage;
      lo_min_voltage = candidate_voltage;
      candidate_time = mid_threshold * driver_ramp;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      if (candidate_voltage > upper_threshold) {
        hi_min_time = candidate_time;
        mid_min_time = candidate_time;
        lo_min_time = candidate_time;
        hi_min_voltage = candidate_voltage;
        mid_min_voltage = candidate_voltage;
        lo_min_voltage = candidate_voltage;
        if (candidate_voltage < mid_threshold) {
          mid_max_time = candidate_time;
          mid_max_voltage = candidate_voltage;
        } else {
          mid_min_time = candidate_time;
          mid_min_voltage = candidate_voltage;
        }
      } else {
        hi_max_time = candidate_time;
        mid_min_time = candidate_time;
        hi_max_voltage = candidate_voltage;
        mid_min_voltage = candidate_voltage;
        candidate_time = lower_threshold * driver_ramp;
        candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
        hi_min_time = candidate_time;
        hi_min_voltage = candidate_voltage;
      }
    } else {
      mid_min_time = candidate_time;
      lo_min_time = candidate_time;
      mid_min_voltage = candidate_voltage;
      lo_min_voltage = candidate_voltage;
      mid_max_time = lo_max_time;
      mid_max_voltage = lo_max_voltage;
      if (candidate_voltage > upper_threshold) {
        hi_min_time = mid_min_time;
        hi_min_voltage = mid_min_voltage;
        hi_max_time = mid_max_time;
        hi_max_voltage = mid_max_voltage;
      } else {
        hi_max_time = mid_min_time;
        hi_max_voltage = mid_min_voltage;
        candidate_time = lower_threshold * driver_ramp;
        candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
        hi_min_time = candidate_time;
        hi_min_voltage = candidate_voltage;
      }
    }
  } else if (waveform_at_ramp < mid_threshold) {
    hi_max_time = driver_ramp;
    mid_max_time = driver_ramp;
    lo_min_time = driver_ramp;
    hi_max_voltage = waveform_at_ramp;
    mid_max_voltage = waveform_at_ramp;
    lo_min_voltage = waveform_at_ramp;
    double candidate_time = driver_ramp + 1.6 / search_pole;
    double candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    while (candidate_voltage > lower_threshold) {
      lo_min_time = candidate_time;
      lo_min_voltage = candidate_voltage;
      candidate_time += 1.0 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    }
    lo_max_time = candidate_time;
    lo_max_voltage = candidate_voltage;
    candidate_time = mid_threshold * driver_ramp;
    candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    mid_min_time = candidate_time;
    mid_min_voltage = candidate_voltage;
    if (candidate_voltage > upper_threshold) {
      hi_min_time = candidate_time;
      hi_min_voltage = candidate_voltage;
    } else {
      hi_max_time = candidate_time;
      hi_max_voltage = candidate_voltage;
      candidate_time = lower_threshold * driver_ramp;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      hi_min_time = candidate_time;
      hi_min_voltage = candidate_voltage;
    }
  } else if (waveform_at_ramp < upper_threshold) {
    hi_max_time = driver_ramp;
    mid_min_time = driver_ramp;
    lo_min_time = driver_ramp;
    hi_max_voltage = waveform_at_ramp;
    mid_min_voltage = waveform_at_ramp;
    lo_min_voltage = waveform_at_ramp;
    double candidate_time = lower_threshold * driver_ramp;
    double candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    hi_min_time = candidate_time;
    hi_min_voltage = candidate_voltage;
    candidate_time = driver_ramp + 0.7 / search_pole;
    candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    while (candidate_voltage > mid_threshold) {
      mid_min_time = candidate_time;
      lo_min_time = candidate_time;
      mid_min_voltage = candidate_voltage;
      lo_min_voltage = candidate_voltage;
      candidate_time += 0.7 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    }
    mid_max_time = candidate_time;
    mid_max_voltage = candidate_voltage;
    if (candidate_voltage < lower_threshold) {
      lo_max_time = candidate_time;
      lo_max_voltage = candidate_voltage;
    } else {
      lo_min_time = candidate_time;
      lo_min_voltage = candidate_voltage;
      candidate_time += 1.0 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      while (candidate_voltage > lower_threshold) {
        lo_min_time = candidate_time;
        lo_min_voltage = candidate_voltage;
        candidate_time += 1.0 / search_pole;
        candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
      }
      lo_max_time = candidate_time;
      lo_max_voltage = candidate_voltage;
    }
  } else {
    double candidate_time = driver_ramp;
    double candidate_voltage = waveform_at_ramp;
    hi_min_time = candidate_time;
    mid_min_time = candidate_time;
    lo_min_time = candidate_time;
    hi_min_voltage = candidate_voltage;
    mid_min_voltage = candidate_voltage;
    lo_min_voltage = candidate_voltage;
    while (candidate_voltage > upper_threshold) {
      hi_min_time = candidate_time;
      mid_min_time = candidate_time;
      lo_min_time = candidate_time;
      hi_min_voltage = candidate_voltage;
      mid_min_voltage = candidate_voltage;
      lo_min_voltage = candidate_voltage;
      candidate_time += 1.0 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    }
    hi_max_time = candidate_time;
    hi_max_voltage = candidate_voltage;
    while (candidate_voltage > mid_threshold) {
      mid_min_time = candidate_time;
      lo_min_time = candidate_time;
      mid_min_voltage = candidate_voltage;
      lo_min_voltage = candidate_voltage;
      candidate_time += 1.0 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    }
    mid_max_time = candidate_time;
    mid_max_voltage = candidate_voltage;
    while (candidate_voltage > lower_threshold) {
      lo_min_time = candidate_time;
      lo_min_voltage = candidate_voltage;
      candidate_time += 1.0 / search_pole;
      candidate_voltage = calcParasiticArnoldiWaveformVoltage(candidate_time, driver_ramp, pole_list, residue_list);
    }
    lo_max_time = candidate_time;
    lo_max_voltage = candidate_voltage;
  }

  upper_time = solveParasiticArnoldiBracketedTime(driver_ramp, pole_list, residue_list, upper_threshold, hi_min_time, hi_max_time, hi_min_voltage,
                                                  hi_max_voltage);
  mid_time = solveParasiticArnoldiBracketedTime(driver_ramp, pole_list, residue_list, mid_threshold, mid_min_time, mid_max_time, mid_min_voltage,
                                                mid_max_voltage);
  lower_time = solveParasiticArnoldiBracketedTime(driver_ramp, pole_list, residue_list, lower_threshold, lo_min_time, lo_max_time, lo_min_voltage,
                                                  lo_max_voltage);
}

double TimingPropagator::calcParasiticArnoldiWaveformVoltage(double time, double driver_ramp, std::vector<double>& pole_list,
                                                            std::vector<double>& residue_list)
{
  double voltage = 0.0;
  for (std::size_t pole_idx = 0; pole_idx < pole_list.size(); pole_idx++) {
    double pole_time = pole_list[pole_idx] * time;
    double pole_ramp = pole_list[pole_idx] * driver_ramp;
    double response = 0.0;
    if (time < driver_ramp) {
      response = 1.0 - time / driver_ramp + (1.0 - std::exp(-pole_time)) / pole_ramp;
    } else {
      response = std::exp(pole_ramp - pole_time) * (1.0 - std::exp(-pole_ramp)) / pole_ramp;
    }
    voltage += residue_list[pole_idx] * response;
  }
  return voltage;
}

void TimingPropagator::calcParasiticArnoldiWaveformVoltageAndDerivative(double time, double driver_ramp, std::vector<double>& pole_list,
                                                                        std::vector<double>& residue_list, double& voltage, double& derivative)
{
  voltage = 0.0;
  derivative = 0.0;
  for (std::size_t pole_idx = 0; pole_idx < pole_list.size(); pole_idx++) {
    double pole = pole_list[pole_idx];
    double pole_time = pole * time;
    double pole_ramp = pole * driver_ramp;
    double response = 0.0;
    double response_derivative = 0.0;
    if (time < driver_ramp) {
      double ramp_response = (1.0 - std::exp(-pole_time)) / pole_ramp;
      response = 1.0 - time / driver_ramp + ramp_response;
      response_derivative = -pole * ramp_response;
    } else {
      response = std::exp(pole_ramp - pole_time) * (1.0 - std::exp(-pole_ramp)) / pole_ramp;
      response_derivative = -pole * response;
    }
    voltage += residue_list[pole_idx] * response;
    derivative += residue_list[pole_idx] * response_derivative;
  }
}

double TimingPropagator::solveParasiticArnoldiBracketedTime(double driver_ramp, std::vector<double>& pole_list,
                                                            std::vector<double>& residue_list, double voltage, double lower_time,
                                                            double upper_time, double lower_voltage, double upper_voltage)
{
  double lower_function = lower_voltage - voltage;
  double upper_function = upper_voltage - voltage;
  if (lower_function == 0.0) {
    return lower_time;
  }
  if (upper_function == 0.0) {
    return upper_time;
  }

  double result_time = (lower_function * upper_time - upper_function * lower_time) / (lower_function - upper_function);
  double low_time = lower_time;
  double high_time = upper_time;
  if (lower_function < upper_function) {
    low_time = lower_time;
    high_time = upper_time;
    if (0.0 < lower_function) {
      return lower_time;
    }
    if (upper_function < 0.0) {
      return upper_time;
    }
  } else {
    low_time = upper_time;
    high_time = lower_time;
    if (0.0 < upper_function) {
      return upper_time;
    }
    if (lower_function < 0.0) {
      return lower_time;
    }
  }

  double old_delta = std::abs(upper_time - lower_time);
  double delta = old_delta;
  double function_value = 0.0;
  double derivative = 0.0;
  calcParasiticArnoldiWaveformVoltageAndDerivative(result_time, driver_ramp, pole_list, residue_list, function_value, derivative);
  function_value -= voltage;
  double last_function_value = 0.0;
  for (int32_t iter = 1; iter < 10; iter++) {
    if ((((result_time - high_time) * derivative - function_value) * ((result_time - low_time) * derivative - function_value) >= 0.0)
        || (std::abs(2.0 * function_value) > std::abs(old_delta * derivative))) {
      old_delta = delta;
      delta = 0.5 * (high_time - low_time);
      if (last_function_value * function_value > 0.0) {
        if (function_value < 0.0) {
          delta = 0.9348 * (high_time - low_time);
        } else {
          delta = 0.0625 * (high_time - low_time);
        }
      }
      last_function_value = function_value;
      result_time = low_time + delta;
      if (low_time == result_time) {
        return result_time;
      }
    } else {
      old_delta = delta;
      delta = function_value / derivative;
      last_function_value = 0.0;
      double temp_time = result_time;
      result_time -= delta;
      if (temp_time == result_time) {
        return result_time;
      }
    }
    if (std::abs(delta) < 1E-6) {
      return result_time;
    }
    calcParasiticArnoldiWaveformVoltageAndDerivative(result_time, driver_ramp, pole_list, residue_list, function_value, derivative);
    function_value -= voltage;
    if (function_value < 0.0) {
      low_time = result_time;
    } else {
      high_time = result_time;
    }
  }
  if (std::abs(function_value) < 1E-6) {
    return result_time;
  }
  return 0.5 * (low_time + high_time);
}

double TimingPropagator::getParasiticTotalResistance(ParasiticNet& parasitic_net)
{
  double resistance = 0.0;
  for (ParasiticResistor& parasitic_resistor : parasitic_net.get_resistor_list()) {
    resistance += parasitic_resistor.get_resistance();
  }
  return resistance;
}

void TimingPropagator::propagateArrival()
{
  Database& database = STADM.getDatabase();
  initTimingPointList();
  markClockPointList();
  propagateClockArrival();
  seedStartPointList();
  propagateDataSlewDelay();

  for (std::string& pin_name : database.get_timing_order_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      if (shouldStopDataPropagation(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateArrivalArc(arc_idx);
    }
  }
}

bool TimingPropagator::isDisableArc(Arc& arc)
{
  return arc.get_is_disable_arc() || arc.get_is_loop_disable();
}

bool TimingPropagator::shouldStopDataPropagation(Arc& arc)
{
  return arc.get_type() == ArcType::kNet && isSequentialClockPin(arc.get_sink_pin());
}

bool TimingPropagator::isSequentialClockPin(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_clock_pin_name();
}

void TimingPropagator::initTimingPointList()
{
  Database& database = STADM.getDatabase();
  for (std::pair<const std::string, TimingPoint>& timing_pair : database.get_timing_point_map()) {
    timing_pair.second.set_arrival(-std::numeric_limits<double>::infinity());
    timing_pair.second.set_required(std::numeric_limits<double>::infinity());
    timing_pair.second.set_slack(0.0);
    timing_pair.second.set_launch_time(0.0);
    timing_pair.second.get_predecessor().clear();
    timing_pair.second.get_clock_name().clear();
    timing_pair.second.get_clock_arrival_map().clear();
    timing_pair.second.get_path_state_map().clear();
    timing_pair.second.get_data_slew_map().clear();
    timing_pair.second.get_clock_predecessor_map().clear();
    timing_pair.second.get_clock_predecessor_arc_delay_map().clear();
    timing_pair.second.get_clock_predecessor_trans_type_map().clear();
    timing_pair.second.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
    timing_pair.second.set_is_clock_point(false);
  }
}

void TimingPropagator::markClockPointList()
{
  Database& database = STADM.getDatabase();
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    for (std::string& clock_source : clock_pair.second.get_source_list()) {
      markClockPoint(clock_source);
    }
  }
}

void TimingPropagator::markClockPoint(std::string& clock_source)
{
  Database& database = STADM.getDatabase();
  if (database.get_timing_point_map().count(clock_source) == 0) {
    return;
  }

  std::queue<std::string> pin_queue;
  database.get_timing_point_map()[clock_source].set_is_clock_point(true);
  pin_queue.push(clock_source);

  while (!pin_queue.empty()) {
    std::string pin_name = pin_queue.front();
    pin_queue.pop();

    if (shouldStopClockPropagation(pin_name)) {
      continue;
    }
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      if (isDisableArc(arc)) {
        continue;
      }
      TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
      if (sink_point.get_is_clock_point()) {
        continue;
      }
      sink_point.set_is_clock_point(true);
      pin_queue.push(arc.get_sink_pin());
    }
  }
}

void TimingPropagator::propagateClockArrival()
{
  Database& database = STADM.getDatabase();
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    for (std::string& clock_source : clock_pair.second.get_source_list()) {
      seedClockArrival(clock_source);
    }
  }
  propagateClockSlewDelay();

  for (std::string& pin_name : database.get_timing_order_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
    if (!timing_point.get_is_clock_point()) {
      continue;
    }
    if (shouldStopClockPropagation(pin_name)) {
      continue;
    }
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateClockArrivalArc(arc_idx, AnalysisType::kMax);
      propagateClockArrivalArc(arc_idx, AnalysisType::kMin);
    }
  }
}

void TimingPropagator::seedClockArrival(std::string& clock_source)
{
  Database& database = STADM.getDatabase();
  if (database.get_timing_point_map().count(clock_source) == 0) {
    return;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[clock_source];
  timing_point.get_clock_arrival_map()[AnalysisType::kMax][TransType::kRise] = 0.0;
  timing_point.get_clock_arrival_map()[AnalysisType::kMax][TransType::kFall] = 0.0;
  timing_point.get_clock_arrival_map()[AnalysisType::kMin][TransType::kRise] = 0.0;
  timing_point.get_clock_arrival_map()[AnalysisType::kMin][TransType::kFall] = 0.0;
  timing_point.get_clock_slew_map()[AnalysisType::kMax][TransType::kRise] = 0.0;
  timing_point.get_clock_slew_map()[AnalysisType::kMax][TransType::kFall] = 0.0;
  timing_point.get_clock_slew_map()[AnalysisType::kMin][TransType::kRise] = 0.0;
  timing_point.get_clock_slew_map()[AnalysisType::kMin][TransType::kFall] = 0.0;
}

void TimingPropagator::propagateClockSlewDelay()
{
  Database& database = STADM.getDatabase();
  for (std::string& pin_name : database.get_timing_order_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
    if (!timing_point.get_is_clock_point() || shouldStopClockPropagation(pin_name)) {
      continue;
    }
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateClockSlewDelayArc(arc_idx, AnalysisType::kMax);
      propagateClockSlewDelayArc(arc_idx, AnalysisType::kMin);
    }
  }
}

void TimingPropagator::propagateClockSlewDelayArc(std::size_t arc_idx, AnalysisType analysis_type)
{
  propagateClockSlewDelayArc(arc_idx, analysis_type, TransType::kRise);
  propagateClockSlewDelayArc(arc_idx, analysis_type, TransType::kFall);
}

void TimingPropagator::propagateClockSlewDelayArc(std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type)
{
  Database& database = STADM.getDatabase();
  Arc& arc = database.get_arc_list()[arc_idx];
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (!source_point.get_is_clock_point() || !sink_point.get_is_clock_point()) {
    return;
  }
  if (source_point.get_clock_slew_map().count(analysis_type) == 0
      || source_point.get_clock_slew_map()[analysis_type].count(input_trans_type) == 0) {
    return;
  }
  if (!isClockArcTriggerTrans(arc, input_trans_type)) {
    return;
  }

  for (TransType output_trans_type : getOutputTransTypeList(arc, analysis_type, input_trans_type)) {
    updateClockSlewDelay(arc, source_point, sink_point, analysis_type, input_trans_type, output_trans_type);
  }
}

void TimingPropagator::updateClockSlewDelay(Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                                            TransType input_trans_type, TransType output_trans_type)
{
  double input_slew = source_point.get_clock_slew_map()[analysis_type][input_trans_type];
  double arc_delay = calcArcDelay(arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  double output_slew = calcArcSlew(arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  updateGraphArcDelay(arc, analysis_type, input_trans_type, output_trans_type, arc_delay);
  if (sink_point.get_clock_slew_map().count(analysis_type) == 0
      || sink_point.get_clock_slew_map()[analysis_type].count(output_trans_type) == 0
      || isBetterSlew(output_slew, sink_point.get_clock_slew_map()[analysis_type][output_trans_type], analysis_type)) {
    sink_point.get_clock_slew_map()[analysis_type][output_trans_type] = output_slew;
  }
}

void TimingPropagator::propagateClockArrivalArc(std::size_t arc_idx, AnalysisType analysis_type)
{
  propagateClockArrivalArc(arc_idx, analysis_type, TransType::kRise);
  propagateClockArrivalArc(arc_idx, analysis_type, TransType::kFall);
}

void TimingPropagator::propagateClockArrivalArc(std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type)
{
  Database& database = STADM.getDatabase();
  Arc& arc = database.get_arc_list()[arc_idx];
  if (isDisableArc(arc)) {
    return;
  }
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (!source_point.get_is_clock_point() || !sink_point.get_is_clock_point()) {
    return;
  }
  if (!hasClockArrival(source_point, analysis_type, input_trans_type)) {
    return;
  }
  if (source_point.get_clock_slew_map().count(analysis_type) == 0 || source_point.get_clock_slew_map()[analysis_type].count(input_trans_type) == 0) {
    return;
  }

  for (TransType output_trans_type : getOutputTransTypeList(arc, analysis_type, input_trans_type)) {
    updateClockPathState(arc, source_point, sink_point, analysis_type, input_trans_type, output_trans_type);
  }
}

void TimingPropagator::updateClockPathState(Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                                            TransType input_trans_type, TransType output_trans_type)
{
  double arc_delay = getArcDelay(arc, analysis_type, input_trans_type, output_trans_type);
  double candidate_arrival = roundTime(getClockArrival(source_point, analysis_type, input_trans_type) + arc_delay);
  if (!hasClockArrival(sink_point, analysis_type, output_trans_type)
      || isBetterArrival(candidate_arrival, getClockArrival(sink_point, analysis_type, output_trans_type), analysis_type)) {
    updateClockArrival(sink_point, analysis_type, output_trans_type, candidate_arrival);
    updateClockPredecessor(sink_point, analysis_type, output_trans_type, input_trans_type, arc, arc_delay);
  }
}

bool TimingPropagator::hasClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type)
{
  return timing_point.get_clock_arrival_map().count(analysis_type) > 0 && timing_point.get_clock_arrival_map()[analysis_type].count(trans_type) > 0;
}

double TimingPropagator::getClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type)
{
  if (!hasClockArrival(timing_point, analysis_type, trans_type)) {
    return 0.0;
  }
  return timing_point.get_clock_arrival_map()[analysis_type][trans_type];
}

void TimingPropagator::updateClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, double clock_arrival)
{
  timing_point.get_clock_arrival_map()[analysis_type][trans_type] = clock_arrival;
}

void TimingPropagator::updateClockPredecessor(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, TransType predecessor_trans_type,
                                              Arc& arc, double arc_delay)
{
  timing_point.get_clock_predecessor_map()[analysis_type][trans_type] = arc.get_source_pin();
  timing_point.get_clock_predecessor_arc_delay_map()[analysis_type][trans_type] = arc_delay;
  timing_point.get_clock_predecessor_trans_type_map()[analysis_type][trans_type] = predecessor_trans_type;
}

bool TimingPropagator::shouldStopClockPropagation(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_clock_pin_name();
}

double TimingPropagator::getClockArrival(std::string& pin_name, AnalysisType analysis_type)
{
  return getClockArrival(pin_name, analysis_type, TransType::kRise);
}

double TimingPropagator::getClockArrival(std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  if (database.get_timing_point_map().count(pin_name) == 0) {
    return 0.0;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  return getClockArrival(timing_point, analysis_type, trans_type);
}

double TimingPropagator::getClockSlew(std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  if (database.get_timing_point_map().count(pin_name) == 0) {
    return 0.0;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  if (timing_point.get_clock_slew_map().count(analysis_type) == 0 || timing_point.get_clock_slew_map()[analysis_type].count(trans_type) == 0) {
    return 0.0;
  }
  return timing_point.get_clock_slew_map()[analysis_type][trans_type];
}

void TimingPropagator::seedStartPointList()
{
  Database& database = STADM.getDatabase();
  for (std::string& start_point : database.get_start_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[start_point];
    timing_point.set_arrival(getStartPointArrival(start_point, AnalysisType::kMax));
    timing_point.set_launch_time(getStartPointLaunchTime(start_point, AnalysisType::kMax));
    timing_point.set_clock_name(getClockName(start_point));
    seedPathState(start_point, AnalysisType::kMax);
    seedPathState(start_point, AnalysisType::kMin);
  }
}

void TimingPropagator::propagateDataSlewDelay()
{
  Database& database = STADM.getDatabase();
  seedDataSlewList();
  for (std::string& pin_name : database.get_timing_order_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateDataSlewDelayArc(arc_idx);
    }
  }
}

void TimingPropagator::seedDataSlewList()
{
  Database& database = STADM.getDatabase();
  for (std::string& start_point : database.get_start_point_list()) {
    seedDataSlew(start_point, AnalysisType::kMax);
    seedDataSlew(start_point, AnalysisType::kMin);
  }
}

void TimingPropagator::seedDataSlew(std::string& start_point, AnalysisType analysis_type)
{
  seedDataSlew(start_point, analysis_type, TransType::kRise);
  seedDataSlew(start_point, analysis_type, TransType::kFall);
}

void TimingPropagator::seedDataSlew(std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  database.get_timing_point_map()[start_point].get_data_slew_map()[analysis_type][trans_type]
      = getStartPointSlew(start_point, analysis_type, trans_type);
}

void TimingPropagator::propagateDataSlewDelayArc(std::size_t arc_idx)
{
  propagateDataSlewDelayArc(arc_idx, AnalysisType::kMax, TransType::kRise);
  propagateDataSlewDelayArc(arc_idx, AnalysisType::kMax, TransType::kFall);
  propagateDataSlewDelayArc(arc_idx, AnalysisType::kMin, TransType::kRise);
  propagateDataSlewDelayArc(arc_idx, AnalysisType::kMin, TransType::kFall);
}

void TimingPropagator::propagateDataSlewDelayArc(std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type)
{
  Database& database = STADM.getDatabase();
  Arc& arc = database.get_arc_list()[arc_idx];
  if (isDisableArc(arc)) {
    return;
  }
  if (shouldStopDataPropagation(arc)) {
    return;
  }
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  if (!hasDataSlew(source_point, analysis_type, input_trans_type)) {
    return;
  }
  if (!isClockArcTriggerTrans(arc, input_trans_type)) {
    return;
  }

  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  for (TransType output_trans_type : getOutputTransTypeList(arc, analysis_type, input_trans_type)) {
    updateDataSlewDelay(arc, source_point, sink_point, analysis_type, input_trans_type, output_trans_type);
  }
}

bool TimingPropagator::isClockArcTriggerTrans(Arc& arc, TransType input_trans_type)
{
  if (!arc.get_is_clock_arc() || arc.get_timing_cell_arc() == nullptr) {
    return true;
  }
  return isClockArcTriggerTrans(*arc.get_timing_cell_arc(), input_trans_type);
}

void TimingPropagator::updateDataSlewDelay(Arc& arc, TimingPoint& source_point, TimingPoint& sink_point, AnalysisType analysis_type,
                                           TransType input_trans_type, TransType output_trans_type)
{
  double input_slew = getDataSlew(source_point, analysis_type, input_trans_type);
  double arc_delay = calcArcDelay(arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  double output_slew = calcArcSlew(arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  updateGraphArcDelay(arc, analysis_type, input_trans_type, output_trans_type, arc_delay);
  updateDataSlew(sink_point, analysis_type, output_trans_type, output_slew);
}

void TimingPropagator::updateGraphArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double arc_delay)
{
  if (arc.get_graph_delay_map().count(analysis_type) == 0 || arc.get_graph_delay_map()[analysis_type].count(input_trans_type) == 0
      || arc.get_graph_delay_map()[analysis_type][input_trans_type].count(output_trans_type) == 0
      || isBetterDelay(arc_delay, arc.get_graph_delay_map()[analysis_type][input_trans_type][output_trans_type], analysis_type)) {
    arc.get_graph_delay_map()[analysis_type][input_trans_type][output_trans_type] = arc_delay;
  }
}

void TimingPropagator::updateDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type, double data_slew)
{
  if (!hasDataSlew(timing_point, analysis_type, trans_type)
      || isBetterSlew(data_slew, timing_point.get_data_slew_map()[analysis_type][trans_type], analysis_type)) {
    timing_point.get_data_slew_map()[analysis_type][trans_type] = data_slew;
  }
}

bool TimingPropagator::hasDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type)
{
  return timing_point.get_data_slew_map().count(analysis_type) > 0 && timing_point.get_data_slew_map()[analysis_type].count(trans_type) > 0;
}

double TimingPropagator::getDataSlew(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type)
{
  if (!hasDataSlew(timing_point, analysis_type, trans_type)) {
    return 0.0;
  }
  return timing_point.get_data_slew_map()[analysis_type][trans_type];
}

bool TimingPropagator::isBetterDelay(double candidate_delay, double current_delay, AnalysisType analysis_type)
{
  if (analysis_type == AnalysisType::kMin) {
    return candidate_delay < current_delay - STA_ERROR;
  }
  return candidate_delay > current_delay + STA_ERROR;
}

bool TimingPropagator::isBetterSlew(double candidate_slew, double current_slew, AnalysisType analysis_type)
{
  if (analysis_type == AnalysisType::kMin) {
    return candidate_slew < current_slew - STA_ERROR;
  }
  return candidate_slew > current_slew + STA_ERROR;
}

double TimingPropagator::roundTime(double time)
{
  return std::round(time * 1E15) / 1E15;
}

double TimingPropagator::getStartPointArrival(std::string& start_point, AnalysisType analysis_type)
{
  return getStartPointArrival(start_point, analysis_type, TransType::kRise);
}

double TimingPropagator::getStartPointArrival(std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  if (isClockSourceStartPoint(start_point)) {
    return getStartPointClockEdge(start_point, analysis_type, trans_type);
  }
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port()) {
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (analysis_type == AnalysisType::kMin && port_constraint_map.count(start_point) > 0 && port_constraint_map[start_point].get_has_input_delay_min()) {
      return port_constraint_map[start_point].get_input_delay_min();
    }
    if (port_constraint_map.count(start_point) > 0 && port_constraint_map[start_point].get_has_input_delay_max()) {
      return port_constraint_map[start_point].get_input_delay_max();
    }
    return 0.0;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (instance.get_is_sequential() && start_point == instance.get_clock_pin_name()) {
    TransType clock_trans_type = getClockTransType(instance.get_clock_to_q_arc());
    return getClockArrival(instance.get_clock_pin_name(), analysis_type, clock_trans_type);
  }
  if (instance.get_is_sequential() && start_point == instance.get_output_pin_name()) {
    TransType clock_trans_type = getClockTransType(instance.get_clock_to_q_arc());
    double clock_slew = getClockSlew(instance.get_clock_pin_name(), analysis_type, clock_trans_type);
    double clock_to_q_delay
        = calcTimingCellArcDelay(start_point, instance.get_clock_to_q_arc(), analysis_type, clock_trans_type, trans_type, clock_slew);
    return getClockArrival(instance.get_clock_pin_name(), analysis_type, clock_trans_type) + clock_to_q_delay;
  }
  return 0.0;
}

bool TimingPropagator::isClockSourceStartPoint(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  return pin.get_is_port() && getStartPointClock(start_point) != nullptr;
}

TimingClock* TimingPropagator::getStartPointClock(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    for (std::string& clock_source : clock_pair.second.get_source_list()) {
      if (clock_source == start_point) {
        return &clock_pair.second;
      }
    }
  }
  return nullptr;
}

double TimingPropagator::getStartPointClockEdge(std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  TimingClock* timing_clock = getStartPointClock(start_point);
  if (timing_clock == nullptr) {
    return 0.0;
  }
  if (analysis_type == AnalysisType::kMax && trans_type == TransType::kFall) {
    return timing_clock->get_fall_edge();
  }
  return timing_clock->get_rise_edge();
}

double TimingPropagator::getStartPointSlew(std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port()) {
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (port_constraint_map.count(start_point) > 0 && port_constraint_map[start_point].get_has_input_transition()) {
      return port_constraint_map[start_point].get_input_transition();
    }
    return 0.0;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (instance.get_is_sequential() && start_point == instance.get_clock_pin_name()) {
    TransType clock_trans_type = getClockTransType(instance.get_clock_to_q_arc());
    return getClockSlew(instance.get_clock_pin_name(), analysis_type, clock_trans_type);
  }
  if (instance.get_is_sequential() && start_point == instance.get_output_pin_name()) {
    TransType clock_trans_type = getClockTransType(instance.get_clock_to_q_arc());
    double clock_slew = getClockSlew(instance.get_clock_pin_name(), analysis_type, clock_trans_type);
    return calcTimingCellArcSlew(start_point, instance.get_clock_to_q_arc(), analysis_type, clock_trans_type, trans_type, clock_slew);
  }
  return 0.0;
}

double TimingPropagator::calcTimingCellArcDelay(std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  double output_load = getOutputPinLoad(output_pin, analysis_type, output_trans_type);
  std::vector<double> delay_list;
  for (TimingArc* timing_arc : getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type)) {
    if (timing_arc->get_delay_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double delay = calcTimingArcDelay(output_pin, *timing_arc, analysis_type, output_trans_type, input_slew, output_load);
    delay_list.push_back(delay);
  }
  if (delay_list.empty()) {
    return timing_cell_arc.get_delay();
  }
  std::ranges::sort(delay_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcSlew(std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                               TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    return input_slew;
  }
  if (!isMatchTimingType(timing_cell_arc, output_trans_type)) {
    return input_slew;
  }
  double output_load = getOutputPinLoad(output_pin, analysis_type, output_trans_type);
  std::vector<double> slew_list;
  for (TimingArc* timing_arc : getCandidateTimingArcList(timing_cell_arc, input_trans_type, output_trans_type)) {
    if (timing_arc->get_slew_table_map().count(output_trans_type) == 0) {
      continue;
    }
    double slew = calcTimingArcSlew(output_pin, *timing_arc, analysis_type, output_trans_type, input_slew, output_load);
    slew_list.push_back(slew);
  }
  if (slew_list.empty()) {
    return input_slew;
  }
  std::ranges::sort(slew_list, std::greater<double>());
  if (analysis_type == AnalysisType::kMin) {
    return slew_list.back();
  }
  return slew_list.front();
}

double TimingPropagator::getStartPointLaunchTime(std::string& start_point, AnalysisType analysis_type)
{
  return getStartPointLaunchTime(start_point, analysis_type, TransType::kRise);
}

double TimingPropagator::getStartPointLaunchTime(std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  if (isClockSourceStartPoint(start_point)) {
    return getStartPointClockEdge(start_point, analysis_type, trans_type);
  }
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || (start_point != instance.get_output_pin_name() && start_point != instance.get_clock_pin_name())) {
    return 0.0;
  }
  return getClockArrival(instance.get_clock_pin_name(), analysis_type, getClockTransType(instance.get_clock_to_q_arc()));
}

std::string TimingPropagator::getStartPointCrprClockPin(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return "";
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || (start_point != instance.get_output_pin_name() && start_point != instance.get_clock_pin_name())) {
    return "";
  }
  std::string clock_pin_name = instance.get_clock_pin_name();
  return clock_pin_name;
}

TransType TimingPropagator::getStartPointCrprClockTransType(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return TransType::kNone;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || (start_point != instance.get_output_pin_name() && start_point != instance.get_clock_pin_name())) {
    return TransType::kNone;
  }
  return getClockTransType(instance.get_clock_to_q_arc());
}

TransType TimingPropagator::getClockTransType(TimingCellArc& timing_cell_arc)
{
  if (timing_cell_arc.get_timing_arc_list().empty()) {
    return TransType::kRise;
  }
  TimingArc& timing_arc = timing_cell_arc.get_timing_arc_list().front();
  if (timing_arc.get_trigger_trans_type() != TransType::kNone) {
    return timing_arc.get_trigger_trans_type();
  }
  return TransType::kRise;
}

std::string TimingPropagator::getClockName(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  TimingClock* timing_clock = getStartPointClock(pin_name);
  if (timing_clock != nullptr) {
    return timing_clock->get_clock_name();
  }
  Pin& pin = database.get_pin_map()[pin_name];
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
  if (pin.get_is_port()) {
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (port_constraint_map.count(pin_name) > 0 && !port_constraint_map[pin_name].get_clock_name().empty()) {
      return port_constraint_map[pin_name].get_clock_name();
    }
  }
  if (!clock_map.empty()) {
    return clock_map.begin()->first;
  }
  return "clk";
}

std::string TimingPropagator::getPathStateStartPoint(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return start_point;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (instance.get_is_sequential() && start_point == instance.get_clock_pin_name()) {
    return instance.get_output_pin_name();
  }
  return start_point;
}

void TimingPropagator::seedPathState(std::string& start_point, AnalysisType analysis_type)
{
  Database& database = STADM.getDatabase();
  PathSourceType source_type = getStartPointSourceType(start_point, analysis_type);
  if (source_type == PathSourceType::kNone) {
    return;
  }
  std::string path_state_start_point = getPathStateStartPoint(start_point);
  TimingPathState& rise_path_state
      = database.get_timing_point_map()[start_point].get_path_state_map()[analysis_type][source_type][TransType::kRise][path_state_start_point];
  rise_path_state.set_arrival(getStartPointArrival(start_point, analysis_type, TransType::kRise));
  rise_path_state.set_slew(getStartPointSlew(start_point, analysis_type, TransType::kRise));
  rise_path_state.set_launch_time(getStartPointLaunchTime(start_point, analysis_type, TransType::kRise));
  rise_path_state.set_start_point(path_state_start_point);
  rise_path_state.set_clock_name(getClockName(start_point));
  rise_path_state.set_crpr_clock_pin(getStartPointCrprClockPin(start_point));
  rise_path_state.get_predecessor().clear();
  rise_path_state.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
  rise_path_state.set_predecessor_arc_delay(0.0);
  rise_path_state.set_trans_type(TransType::kRise);
  rise_path_state.set_predecessor_trans_type(TransType::kNone);
  rise_path_state.set_crpr_clock_trans_type(getStartPointCrprClockTransType(start_point));
  TimingPathState& fall_path_state
      = database.get_timing_point_map()[start_point].get_path_state_map()[analysis_type][source_type][TransType::kFall][path_state_start_point];
  fall_path_state.set_arrival(getStartPointArrival(start_point, analysis_type, TransType::kFall));
  fall_path_state.set_slew(getStartPointSlew(start_point, analysis_type, TransType::kFall));
  fall_path_state.set_launch_time(getStartPointLaunchTime(start_point, analysis_type, TransType::kFall));
  fall_path_state.set_start_point(path_state_start_point);
  fall_path_state.set_clock_name(getClockName(start_point));
  fall_path_state.set_crpr_clock_pin(getStartPointCrprClockPin(start_point));
  fall_path_state.get_predecessor().clear();
  fall_path_state.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
  fall_path_state.set_predecessor_arc_delay(0.0);
  fall_path_state.set_trans_type(TransType::kFall);
  fall_path_state.set_predecessor_trans_type(TransType::kNone);
  fall_path_state.set_crpr_clock_trans_type(getStartPointCrprClockTransType(start_point));
}

PathSourceType TimingPropagator::getStartPointSourceType(std::string& start_point, AnalysisType analysis_type)
{
  if (isClockSourceStartPoint(start_point)) {
    return PathSourceType::kInput;
  }
  if (isInputStartPoint(start_point) && hasInputDelay(start_point, analysis_type)) {
    return PathSourceType::kInput;
  }
  if (isRegisterStartPoint(start_point)) {
    return PathSourceType::kRegister;
  }
  return PathSourceType::kNone;
}

bool TimingPropagator::hasInputDelay(std::string& start_point, AnalysisType analysis_type)
{
  Database& database = STADM.getDatabase();
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  if (port_constraint_map.count(start_point) == 0) {
    return false;
  }
  TimingPortConstraint& port_constraint = port_constraint_map[start_point];
  if (analysis_type == AnalysisType::kMin) {
    return port_constraint.get_has_input_delay_min() || port_constraint.get_has_input_delay_max();
  }
  return port_constraint.get_has_input_delay_max() || port_constraint.get_has_input_delay_min();
}

bool TimingPropagator::isInputStartPoint(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout);
}

bool TimingPropagator::isRegisterStartPoint(std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && (start_point == instance.get_output_pin_name() || start_point == instance.get_clock_pin_name())
         && hasClockPoint(instance.get_clock_pin_name());
}

bool TimingPropagator::hasClockPoint(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  return database.get_timing_point_map().count(pin_name) > 0 && database.get_timing_point_map()[pin_name].get_is_clock_point();
}

void TimingPropagator::propagateArrivalArc(std::size_t arc_idx)
{
  Database& database = STADM.getDatabase();
  const double kEpsilon = 1e-9;
  Arc& arc = database.get_arc_list()[arc_idx];
  if (isDisableArc(arc)) {
    return;
  }
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
  propagatePathStateArc(arc_idx, AnalysisType::kMax, PathSourceType::kInput);
  propagatePathStateArc(arc_idx, AnalysisType::kMax, PathSourceType::kRegister);
  propagatePathStateArc(arc_idx, AnalysisType::kMin, PathSourceType::kInput);
  propagatePathStateArc(arc_idx, AnalysisType::kMin, PathSourceType::kRegister);
}

void TimingPropagator::propagatePathStateArc(std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type)
{
  propagatePathStateArc(arc_idx, analysis_type, source_type, TransType::kRise);
  propagatePathStateArc(arc_idx, analysis_type, source_type, TransType::kFall);
}

void TimingPropagator::propagatePathStateArc(std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type,
                                             TransType input_trans_type)
{
  Database& database = STADM.getDatabase();
  if (isDisableArc(database.get_arc_list()[arc_idx])) {
    return;
  }
  if (!isClockArcTriggerTrans(database.get_arc_list()[arc_idx], input_trans_type)) {
    return;
  }
  for (TransType output_trans_type : getOutputTransTypeList(database.get_arc_list()[arc_idx], analysis_type, input_trans_type)) {
    propagatePathStateArc(arc_idx, analysis_type, source_type, input_trans_type, output_trans_type);
  }
}

void TimingPropagator::propagatePathStateArc(std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type,
                                             TransType input_trans_type, TransType output_trans_type)
{
  Database& database = STADM.getDatabase();
  Arc& arc = database.get_arc_list()[arc_idx];
  if (isDisableArc(arc)) {
    return;
  }
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  if (!hasPathState(source_point, analysis_type, source_type, input_trans_type)) {
    return;
  }

  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  std::map<std::string, TimingPathState>& source_path_state_map = getPathStateMap(source_point, analysis_type, source_type, input_trans_type);
  for (std::pair<const std::string, TimingPathState>& source_path_state_pair : source_path_state_map) {
    TimingPathState& source_path_state = source_path_state_pair.second;
    if (!isFinite(source_path_state.get_arrival())) {
      continue;
    }
    double arc_delay = getArcDelay(arc, analysis_type, input_trans_type, output_trans_type);
    double candidate_arrival = roundTime(source_path_state.get_arrival() + arc_delay);
    std::string& start_point = source_path_state.get_start_point();
    std::map<std::string, TimingPathState>& sink_path_state_map = getPathStateMap(sink_point, analysis_type, source_type, output_trans_type);
    if (sink_path_state_map.count(start_point) == 0 || isBetterArrival(candidate_arrival, sink_path_state_map[start_point].get_arrival(), analysis_type)) {
      TimingPathState& sink_path_state = sink_path_state_map[start_point];
      sink_path_state.set_arrival(candidate_arrival);
      sink_path_state.set_slew(getDataSlew(sink_point, analysis_type, output_trans_type));
      sink_path_state.set_start_point(start_point);
      sink_path_state.set_predecessor(arc.get_source_pin());
      sink_path_state.set_predecessor_arc_idx(arc_idx);
      sink_path_state.set_predecessor_arc_delay(arc_delay);
      sink_path_state.set_launch_time(source_path_state.get_launch_time());
      sink_path_state.set_clock_name(source_path_state.get_clock_name());
      sink_path_state.set_crpr_clock_pin(source_path_state.get_crpr_clock_pin());
      sink_path_state.set_trans_type(output_trans_type);
      sink_path_state.set_predecessor_trans_type(input_trans_type);
      sink_path_state.set_crpr_clock_trans_type(source_path_state.get_crpr_clock_trans_type());
    }
  }
}

std::vector<TransType> TimingPropagator::getOutputTransTypeList(Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  std::vector<TransType> output_trans_type_list;
  if (arc.get_input_output_delay_map().count(analysis_type) == 0 || arc.get_input_output_delay_map()[analysis_type].count(input_trans_type) == 0) {
    output_trans_type_list.push_back(getOutputTransType(arc, input_trans_type));
    return output_trans_type_list;
  }
  for (std::pair<const TransType, double>& delay_pair : arc.get_input_output_delay_map()[analysis_type][input_trans_type]) {
    output_trans_type_list.push_back(delay_pair.first);
  }
  return output_trans_type_list;
}

TransType TimingPropagator::getOutputTransType(Arc& arc, TransType input_trans_type)
{
  if (arc.get_trans_type_map().count(input_trans_type) > 0) {
    return arc.get_trans_type_map()[input_trans_type];
  }
  return input_trans_type;
}

double TimingPropagator::getArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  if (arc.get_trans_delay_map().count(analysis_type) > 0 && arc.get_trans_delay_map()[analysis_type].count(input_trans_type) > 0) {
    return arc.get_trans_delay_map()[analysis_type][input_trans_type];
  }
  if (analysis_type == AnalysisType::kMin) {
    return arc.get_delay_min();
  }
  return arc.get_delay_max();
}

double TimingPropagator::getArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type)
{
  if (arc.get_graph_delay_map().count(analysis_type) > 0 && arc.get_graph_delay_map()[analysis_type].count(input_trans_type) > 0
      && arc.get_graph_delay_map()[analysis_type][input_trans_type].count(output_trans_type) > 0) {
    return arc.get_graph_delay_map()[analysis_type][input_trans_type][output_trans_type];
  }
  if (arc.get_input_output_delay_map().count(analysis_type) > 0 && arc.get_input_output_delay_map()[analysis_type].count(input_trans_type) > 0
      && arc.get_input_output_delay_map()[analysis_type][input_trans_type].count(output_trans_type) > 0) {
    return arc.get_input_output_delay_map()[analysis_type][input_trans_type][output_trans_type];
  }
  return getArcDelay(arc, analysis_type, input_trans_type);
}

double TimingPropagator::calcArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                      double input_slew)
{
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(arc, analysis_type, output_trans_type, input_slew);
  }
  TimingCellArc* timing_cell_arc = getTimingCellArc(arc);
  if (timing_cell_arc != nullptr) {
    return calcTimingCellArcDelay(arc, *timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  }
  if (arc.get_input_output_delay_map().count(analysis_type) > 0 && arc.get_input_output_delay_map()[analysis_type].count(input_trans_type) > 0
      && arc.get_input_output_delay_map()[analysis_type][input_trans_type].count(output_trans_type) > 0) {
    return arc.get_input_output_delay_map()[analysis_type][input_trans_type][output_trans_type];
  }
  if (arc.get_trans_delay_map().count(analysis_type) > 0 && arc.get_trans_delay_map()[analysis_type].count(input_trans_type) > 0) {
    return arc.get_trans_delay_map()[analysis_type][input_trans_type];
  }
  if (analysis_type == AnalysisType::kMin) {
    return arc.get_delay_min();
  }
  return arc.get_delay_max();
}

double TimingPropagator::calcArcSlew(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                     double input_slew)
{
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcSlew(arc, analysis_type, output_trans_type, input_slew);
  }
  TimingCellArc* timing_cell_arc = getTimingCellArc(arc);
  if (timing_cell_arc != nullptr) {
    return calcTimingCellArcSlew(arc, *timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  }
  return input_slew;
}

double TimingPropagator::calcNetArcSlew(Arc& arc, AnalysisType analysis_type, TransType trans_type, double input_slew)
{
  Database& database = STADM.getDatabase();
  if (database.get_parasitic_library().get_net_map().count(arc.get_owner_name()) > 0) {
    return calcParasiticSlew(arc, analysis_type, trans_type, input_slew);
  }
  return input_slew;
}

double TimingPropagator::calcParasiticSlew(Arc& arc, AnalysisType analysis_type, TransType trans_type, double input_slew)
{
  Database& database = STADM.getDatabase();
  ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[arc.get_owner_name()];
  std::string source_node_name = getParasiticNodeName(parasitic_net, arc.get_source_pin());
  std::string sink_node_name = getParasiticNodeName(parasitic_net, arc.get_sink_pin());
  if (source_node_name.empty() || sink_node_name.empty()) {
    return input_slew;
  }

  std::optional<double> cached_load_slew = getParasiticArnoldiCachedLoadSlew(arc, analysis_type, trans_type, input_slew);
  if (cached_load_slew) {
    return *cached_load_slew;
  }
  cached_load_slew = getParasiticDmpCachedLoadSlew(arc, analysis_type, trans_type, input_slew);
  if (cached_load_slew) {
    return *cached_load_slew;
  }
  std::optional<double> input_port_slew
      = calcParasiticArnoldiInputPortSlew(parasitic_net, source_node_name, sink_node_name, analysis_type, trans_type, input_slew);
  if (input_port_slew) {
    return *input_port_slew;
  }

  buildParasiticDelayMap(parasitic_net, source_node_name, analysis_type, trans_type);
  if (_parasitic_impulse_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type].count(sink_node_name) == 0) {
    return input_slew;
  }
  double impulse = _parasitic_impulse_map_cache[parasitic_net.get_net_name()][analysis_type][trans_type][sink_node_name];
  double output_slew = std::sqrt(input_slew * input_slew + impulse);
  if (input_slew < 0.0) {
    return -output_slew;
  }
  return output_slew;
}

bool TimingPropagator::hasPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type)
{
  return hasPathState(timing_point, analysis_type, source_type, TransType::kRise) || hasPathState(timing_point, analysis_type, source_type, TransType::kFall);
}

bool TimingPropagator::hasPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type)
{
  return timing_point.get_path_state_map().count(analysis_type) > 0 && timing_point.get_path_state_map()[analysis_type].count(source_type) > 0
         && timing_point.get_path_state_map()[analysis_type][source_type].count(trans_type) > 0
         && !timing_point.get_path_state_map()[analysis_type][source_type][trans_type].empty();
}

std::map<std::string, TimingPathState>& TimingPropagator::getPathStateMap(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type,
                                                                          TransType trans_type)
{
  return timing_point.get_path_state_map()[analysis_type][source_type][trans_type];
}

TimingPathState& TimingPropagator::getPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                                                std::string& start_point)
{
  return timing_point.get_path_state_map()[analysis_type][source_type][trans_type][start_point];
}

TimingPathState* TimingPropagator::getWorstPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type)
{
  TimingPathState* best_path_state = nullptr;
  for (TransType trans_type : {TransType::kRise, TransType::kFall}) {
    TimingPathState* path_state = getWorstPathState(timing_point, analysis_type, source_type, trans_type);
    if (path_state == nullptr) {
      continue;
    }
    if (best_path_state == nullptr || isBetterArrival(path_state->get_arrival(), best_path_state->get_arrival(), analysis_type)) {
      best_path_state = path_state;
    }
  }
  return best_path_state;
}

TimingPathState* TimingPropagator::getWorstPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type)
{
  if (!hasPathState(timing_point, analysis_type, source_type, trans_type)) {
    return nullptr;
  }

  TimingPathState* best_path_state = nullptr;
  std::map<std::string, TimingPathState>& path_state_map = getPathStateMap(timing_point, analysis_type, source_type, trans_type);
  for (std::pair<const std::string, TimingPathState>& path_state_pair : path_state_map) {
    TimingPathState& path_state = path_state_pair.second;
    if (!isFinite(path_state.get_arrival())) {
      continue;
    }
    if (best_path_state == nullptr || isBetterArrival(path_state.get_arrival(), best_path_state->get_arrival(), analysis_type)) {
      best_path_state = &path_state;
    }
  }
  return best_path_state;
}

TransType TimingPropagator::getEndPointTransType(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type)
{
  TransType best_trans_type = TransType::kNone;
  for (TransType trans_type : {TransType::kRise, TransType::kFall}) {
    TimingPathState* path_state = getWorstPathState(timing_point, analysis_type, source_type, trans_type);
    if (path_state == nullptr) {
      continue;
    }
    if (best_trans_type == TransType::kNone
        || isBetterArrival(path_state->get_arrival(), getWorstPathState(timing_point, analysis_type, source_type, best_trans_type)->get_arrival(),
                           analysis_type)) {
      best_trans_type = trans_type;
    }
  }
  return best_trans_type;
}

bool TimingPropagator::isBetterArrival(double candidate_arrival, double current_arrival, AnalysisType analysis_type)
{
  if (!isFinite(current_arrival)) {
    return true;
  }
  if (analysis_type == AnalysisType::kMin) {
    return candidate_arrival < current_arrival - STA_ERROR;
  }
  return candidate_arrival > current_arrival + STA_ERROR;
}

bool TimingPropagator::isFinite(double value)
{
  return std::isfinite(value);
}

void TimingPropagator::propagateRequired()
{
  Database& database = STADM.getDatabase();
  const double required_time = resolveRequiredTime();

  seedEndPointRequired(required_time);

  for (auto iter = database.get_timing_order_list().rbegin(); iter != database.get_timing_order_list().rend(); ++iter) {
    std::string& pin_name = *iter;
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      if (shouldStopDataPropagation(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateRequiredArc(database.get_arc_list()[arc_idx]);
    }
  }

  updateSlack();
}

double TimingPropagator::resolveRequiredTime()
{
  Database& database = STADM.getDatabase();
  double worst_arrival = 0.0;
  for (std::string& end_point : database.get_end_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[end_point];
    if (isFinite(timing_point.get_arrival())) {
      worst_arrival = std::max(worst_arrival, timing_point.get_arrival());
    }
  }
  return worst_arrival;
}

void TimingPropagator::seedEndPointRequired(double required_time)
{
  Database& database = STADM.getDatabase();
  for (std::string& end_point : database.get_end_point_list()) {
    database.get_timing_point_map()[end_point].set_required(getEndPointRequired(end_point, required_time, AnalysisType::kMax));
  }
}

double TimingPropagator::getEndPointRequired(std::string& end_point, double default_required_time, AnalysisType analysis_type)
{
  Database& database = STADM.getDatabase();
  TimingPathState* end_path_state = getWorstPathState(database.get_timing_point_map()[end_point], analysis_type, PathSourceType::kInput);
  if (end_path_state == nullptr) {
    end_path_state = getWorstPathState(database.get_timing_point_map()[end_point], analysis_type, PathSourceType::kRegister);
  }
  if (end_path_state == nullptr) {
    return getEndPointRequired(end_point, default_required_time, analysis_type, TransType::kRise, 0.0);
  }
  return getEndPointRequired(end_point, default_required_time, analysis_type, end_path_state->get_trans_type(), end_path_state->get_slew());
}

double TimingPropagator::getEndPointRequired(std::string& end_point, double default_required_time, AnalysisType analysis_type,
                                             TransType data_trans_type, double data_slew)
{
  return getEndPointRequired(end_point, end_point, default_required_time, analysis_type, data_trans_type, data_slew);
}

double TimingPropagator::getEndPointRequired(std::string& start_point, std::string& end_point, double default_required_time,
                                             AnalysisType analysis_type, TransType data_trans_type, double data_slew)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port()) {
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (analysis_type == AnalysisType::kMin && port_constraint_map.count(end_point) > 0 && port_constraint_map[end_point].get_has_output_delay_min()) {
      return port_constraint_map[end_point].get_output_delay_min();
    }
    if (port_constraint_map.count(end_point) > 0 && port_constraint_map[end_point].get_has_output_delay_max()) {
      return getEndPointCaptureTime(end_point, analysis_type) - port_constraint_map[end_point].get_output_delay_max();
    }
    return default_required_time;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return default_required_time;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(end_point, analysis_type);
  if (instance.get_is_sequential() && timing_check_arc != nullptr && isMatchCheckTransType(*timing_check_arc, data_trans_type)) {
    std::string clock_name = getClockName(end_point);
    double check_time = getEndPointCheckTime(end_point, *timing_check_arc, analysis_type, data_trans_type, data_slew);
    std::string common_pin_name;
    double cppr = getClockReconvergencePessimism(start_point, end_point, analysis_type, common_pin_name);
    if (analysis_type == AnalysisType::kMin) {
      return roundTime(getEndPointCaptureTime(end_point, analysis_type) + check_time - cppr);
    }
    return roundTime(getEndPointCaptureTime(end_point, analysis_type) - check_time + cppr);
  }
  return default_required_time;
}

double TimingPropagator::getEndPointRequired(TimingPathState& end_path_state, std::string& end_point, double default_required_time,
                                             AnalysisType analysis_type)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port()) {
    return getEndPointRequired(end_path_state.get_start_point(), end_point, default_required_time, analysis_type, end_path_state.get_trans_type(),
                               end_path_state.get_slew());
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return default_required_time;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(end_point, analysis_type);
  if (instance.get_is_sequential() && timing_check_arc != nullptr && isMatchCheckTransType(*timing_check_arc, end_path_state.get_trans_type())) {
    double check_time = getEndPointCheckTime(end_point, *timing_check_arc, analysis_type, end_path_state.get_trans_type(), end_path_state.get_slew());
    std::string common_pin_name;
    double cppr = getClockReconvergencePessimism(end_path_state, end_point, analysis_type, common_pin_name);
    if (analysis_type == AnalysisType::kMin) {
      return roundTime(getEndPointCaptureTime(end_point, analysis_type) + check_time - cppr);
    }
    return roundTime(getEndPointCaptureTime(end_point, analysis_type) - check_time + cppr);
  }
  return default_required_time;
}

bool TimingPropagator::isMatchCheckTransType(TimingCheckArc& timing_check_arc, TransType data_trans_type)
{
  if (timing_check_arc.get_timing_arc_list().empty()) {
    return true;
  }
  for (TimingArc& timing_arc : timing_check_arc.get_timing_arc_list()) {
    if (timing_arc.get_check_table_map().count(data_trans_type) > 0) {
      return true;
    }
  }
  return false;
}

double TimingPropagator::getEndPointCheckTime(std::string& end_point, TimingCheckArc& timing_check_arc, AnalysisType analysis_type,
                                              TransType data_trans_type, double data_slew)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  TransType clock_trans_type = getClockTransType(timing_check_arc);
  AnalysisType capture_analysis_type = getCaptureAnalysisType(analysis_type);
  double clock_slew = getClockSlew(instance.get_clock_pin_name(), capture_analysis_type, clock_trans_type);
  return calcTimingCheckArcTime(timing_check_arc, analysis_type, clock_trans_type, data_trans_type, clock_slew, data_slew);
}

double TimingPropagator::calcTimingCheckArcTime(TimingCheckArc& timing_check_arc, AnalysisType analysis_type, TransType clock_trans_type,
                                                TransType data_trans_type, double clock_slew, double data_slew)
{
  std::vector<TimingArc*> candidate_arc_list = getCandidateTimingCheckArcList(timing_check_arc, clock_trans_type, data_trans_type);
  if (!candidate_arc_list.empty()) {
    std::vector<double> delay_list;
    for (TimingArc* timing_arc : candidate_arc_list) {
      if (timing_arc->get_check_table_map().count(data_trans_type) == 0) {
        continue;
      }
      double delay = timing_arc->get_check_table_map()[data_trans_type].findValue(clock_slew * timing_arc->get_time_unit_scale(),
                                                                                  data_slew * timing_arc->get_time_unit_scale());
      delay_list.push_back(delay / timing_arc->get_time_unit_scale());
    }
    if (delay_list.empty()) {
      return timing_check_arc.get_check_time();
    }
    std::ranges::sort(delay_list, std::greater<double>());
    if (analysis_type == AnalysisType::kMin) {
      return delay_list.back();
    }
    return delay_list.front();
  }
  return timing_check_arc.get_check_time();
}

AnalysisType TimingPropagator::getCaptureAnalysisType(AnalysisType analysis_type)
{
  if (analysis_type == AnalysisType::kMax) {
    return AnalysisType::kMin;
  }
  if (analysis_type == AnalysisType::kMin) {
    return AnalysisType::kMax;
  }
  return analysis_type;
}

TransType TimingPropagator::getClockTransType(TimingCheckArc& timing_check_arc)
{
  return timing_check_arc.get_clock_trans_type();
}

double TimingPropagator::getEndPointCaptureTime(std::string& end_point, AnalysisType analysis_type)
{
  AnalysisType capture_analysis_type = getCaptureAnalysisType(analysis_type);
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(end_point, analysis_type);
  TransType clock_trans_type = timing_check_arc == nullptr ? TransType::kRise : getClockTransType(*timing_check_arc);
  if (analysis_type == AnalysisType::kMax) {
    std::string clock_name = getClockName(end_point);
    return getClockPeriod(clock_name) + getEndPointClockArrival(end_point, capture_analysis_type, clock_trans_type);
  }
  return getEndPointClockArrival(end_point, capture_analysis_type, clock_trans_type);
}

double TimingPropagator::getEndPointClockArrival(std::string& end_point, AnalysisType analysis_type)
{
  return getEndPointClockArrival(end_point, analysis_type, TransType::kRise);
}

double TimingPropagator::getEndPointClockArrival(std::string& end_point, AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential()) {
    return 0.0;
  }
  return getClockArrival(instance.get_clock_pin_name(), analysis_type, trans_type);
}

double TimingPropagator::getClockReconvergencePessimism(TimingPathState& end_path_state, std::string& end_point, AnalysisType analysis_type,
                                                        std::string& common_pin_name)
{
  if (end_path_state.get_crpr_clock_pin().empty()) {
    return getClockReconvergencePessimism(end_path_state.get_start_point(), end_point, analysis_type, common_pin_name);
  }
  std::pair<std::string, TransType> launch_crpr_pin(end_path_state.get_crpr_clock_pin(), end_path_state.get_crpr_clock_trans_type());
  return getClockReconvergencePessimism(launch_crpr_pin, end_point, analysis_type, common_pin_name);
}

double TimingPropagator::getClockReconvergencePessimism(std::string& start_point, std::string& end_point, AnalysisType analysis_type,
                                                        std::string& common_pin_name)
{
  Database& database = STADM.getDatabase();
  if (!isRegisterStartPoint(start_point) || (!isRegisterEndPoint(end_point) && !isTimingCheckEndPoint(end_point))) {
    return 0.0;
  }
  Pin& start_pin = database.get_pin_map()[start_point];
  Instance& start_instance = database.get_instance_map()[start_pin.get_instance_name()];
  TransType launch_trans_type = getClockTransType(start_instance.get_clock_to_q_arc());
  std::pair<std::string, TransType> launch_crpr_pin(start_instance.get_clock_pin_name(), launch_trans_type);
  return getClockReconvergencePessimism(launch_crpr_pin, end_point, analysis_type, common_pin_name);
}

double TimingPropagator::getClockReconvergencePessimism(std::pair<std::string, TransType>& launch_crpr_pin, std::string& end_point,
                                                        AnalysisType analysis_type, std::string& common_pin_name)
{
  Database& database = STADM.getDatabase();
  if ((!isRegisterEndPoint(end_point) && !isTimingCheckEndPoint(end_point)) || database.get_pin_map().count(launch_crpr_pin.first) == 0) {
    return 0.0;
  }
  Pin& end_pin = database.get_pin_map()[end_point];
  Instance& end_instance = database.get_instance_map()[end_pin.get_instance_name()];
  AnalysisType launch_analysis_type = analysis_type;
  AnalysisType capture_analysis_type = getCaptureAnalysisType(analysis_type);
  TransType launch_trans_type = launch_crpr_pin.second == TransType::kNone ? TransType::kRise : launch_crpr_pin.second;
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(end_point, analysis_type);
  TransType capture_trans_type = timing_check_arc == nullptr ? TransType::kRise : getClockTransType(*timing_check_arc);
  std::vector<std::pair<std::string, TransType>> launch_clock_path
      = getClockPathPinList(launch_crpr_pin.first, launch_analysis_type, launch_trans_type);
  std::vector<std::pair<std::string, TransType>> capture_clock_path
      = getClockPathPinList(end_instance.get_clock_pin_name(), capture_analysis_type, capture_trans_type);
  shrinkClockPathToCrprPath(launch_clock_path);
  shrinkClockPathToCrprPath(capture_clock_path);
  std::pair<std::string, TransType> launch_common_pin;
  std::pair<std::string, TransType> capture_common_pin;
  std::size_t path_size = std::min(launch_clock_path.size(), capture_clock_path.size());
  for (std::size_t i = 0; i < path_size; i++) {
    if (launch_clock_path[i].first != capture_clock_path[i].first || launch_clock_path[i].second != capture_clock_path[i].second) {
      break;
    }
    launch_common_pin = launch_clock_path[i];
    capture_common_pin = capture_clock_path[i];
    common_pin_name = launch_clock_path[i].first;
  }
  if (common_pin_name.empty()) {
    return 0.0;
  }
  double launch_delay_delta = getClockCommonPathDelayDelta(launch_common_pin, launch_analysis_type);
  double capture_delay_delta = getClockCommonPathDelayDelta(capture_common_pin, capture_analysis_type);
  return std::min(launch_delay_delta, capture_delay_delta);
}

double TimingPropagator::getClockCommonPathDelayDelta(std::pair<std::string, TransType>& common_pin, AnalysisType analysis_type)
{
  AnalysisType other_analysis_type = getCaptureAnalysisType(analysis_type);
  double common_arrival = getClockCommonPathArrival(common_pin, analysis_type);
  double other_common_arrival = getClockCommonPathArrival(common_pin, other_analysis_type);
  return std::fabs(common_arrival - other_common_arrival);
}

void TimingPropagator::shrinkClockPathToCrprPath(std::vector<std::pair<std::string, TransType>>& clock_path)
{
  if (!clock_path.empty()) {
    clock_path.pop_back();
  }
  while (clock_path.size() >= 2 && isLeafClockDriverPin(clock_path.back().first)) {
    clock_path.pop_back();
    clock_path.pop_back();
  }
  while (clock_path.size() >= 3 && isLeafClockBufferDriverPin(clock_path)) {
    clock_path.pop_back();
    clock_path.pop_back();
  }
}

bool TimingPropagator::isLeafClockDriverPin(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(pin_name) == 0) {
    return false;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_net_name().empty() || database.get_net_map().count(pin.get_net_name()) == 0) {
    return false;
  }
  Net& net = database.get_net_map()[pin.get_net_name()];
  for (std::string& load_pin_name : net.get_load_pin_list()) {
    if (database.get_pin_map().count(load_pin_name) == 0) {
      continue;
    }
    Pin& load_pin = database.get_pin_map()[load_pin_name];
    if (load_pin.get_is_port() || database.get_instance_map().count(load_pin.get_instance_name()) == 0) {
      continue;
    }
    Instance& load_instance = database.get_instance_map()[load_pin.get_instance_name()];
    if (load_instance.get_is_sequential() && load_pin_name == load_instance.get_clock_pin_name()) {
      return true;
    }
  }
  return false;
}

bool TimingPropagator::isLeafClockBufferDriverPin(std::vector<std::pair<std::string, TransType>>& clock_path)
{
  std::string& pin_name = clock_path.back().first;
  std::string& parent_pin_name = clock_path[clock_path.size() - 3].first;
  if (!isLeafClockBufferDriverPin(pin_name) || isClockRootBufferDriverPin(parent_pin_name)) {
    return false;
  }
  if (hasSingleLeafClockBufferLoad(pin_name)) {
    return true;
  }
  return shouldShrinkLeafClockBufferLoad(pin_name);
}

bool TimingPropagator::hasSingleLeafClockBufferLoad(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  Net& net = database.get_net_map()[pin.get_net_name()];
  return net.get_load_pin_list().size() == 1 && isLeafClockBufferLoadPin(net.get_load_pin_list().front());
}

bool TimingPropagator::isClockRootBufferDriverPin(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(pin_name) == 0) {
    return false;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  std::string input_pin_name;
  for (std::string& instance_pin_name : instance.get_pin_name_list()) {
    if (database.get_pin_map().count(instance_pin_name) == 0) {
      continue;
    }
    Pin& instance_pin = database.get_pin_map()[instance_pin_name];
    if (instance_pin.get_direction() == PinDirection::kInput) {
      input_pin_name = instance_pin_name;
      break;
    }
  }
  if (input_pin_name.empty()) {
    return false;
  }
  Pin& input_pin = database.get_pin_map()[input_pin_name];
  if (input_pin.get_net_name().empty() || database.get_net_map().count(input_pin.get_net_name()) == 0) {
    return false;
  }
  Net& input_net = database.get_net_map()[input_pin.get_net_name()];
  if (input_net.get_driver_pin().empty() || database.get_pin_map().count(input_net.get_driver_pin()) == 0) {
    return false;
  }
  Pin& driver_pin = database.get_pin_map()[input_net.get_driver_pin()];
  return driver_pin.get_is_port() && getStartPointClock(input_net.get_driver_pin()) != nullptr;
}

bool TimingPropagator::isLeafClockBufferDriverPin(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(pin_name) == 0) {
    return false;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_net_name().empty() || database.get_net_map().count(pin.get_net_name()) == 0) {
    return false;
  }
  Net& net = database.get_net_map()[pin.get_net_name()];
  bool has_leaf_clock_buffer_load = false;
  for (std::string& load_pin_name : net.get_load_pin_list()) {
    if (!isLeafClockBufferLoadPin(load_pin_name)) {
      return false;
    }
    has_leaf_clock_buffer_load = true;
  }
  return has_leaf_clock_buffer_load;
}

bool TimingPropagator::isLeafClockBufferLoadPin(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(pin_name) == 0) {
    return false;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (instance.get_is_sequential()) {
    return false;
  }
  std::string output_pin_name;
  for (std::string& instance_pin_name : instance.get_pin_name_list()) {
    if (database.get_pin_map().count(instance_pin_name) == 0) {
      continue;
    }
    Pin& instance_pin = database.get_pin_map()[instance_pin_name];
    if (instance_pin.get_direction() == PinDirection::kOutput) {
      output_pin_name = instance_pin_name;
      break;
    }
  }
  return !output_pin_name.empty() && isLeafClockDriverPin(output_pin_name);
}

bool TimingPropagator::shouldShrinkLeafClockBufferLoad(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  double drive_resistance = getBufferDriveResistance(pin_name);
  if (drive_resistance <= 0.0) {
    return false;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  Net& net = database.get_net_map()[pin.get_net_name()];
  bool has_leaf_clock_buffer_load = false;
  bool has_stronger_leaf_clock_buffer_load = false;
  bool has_weaker_leaf_clock_buffer_load = false;
  for (std::string& load_pin_name : net.get_load_pin_list()) {
    Pin& load_pin = database.get_pin_map()[load_pin_name];
    if (load_pin.get_is_port() || database.get_instance_map().count(load_pin.get_instance_name()) == 0) {
      continue;
    }
    Instance& load_instance = database.get_instance_map()[load_pin.get_instance_name()];
    std::string output_pin_name = load_instance.get_output_pin_name();
    double load_drive_resistance = getBufferDriveResistance(output_pin_name);
    if (load_drive_resistance <= 0.0) {
      continue;
    }
    has_leaf_clock_buffer_load = true;
    if (load_drive_resistance < drive_resistance) {
      has_stronger_leaf_clock_buffer_load = true;
    }
    if (load_drive_resistance > drive_resistance) {
      has_weaker_leaf_clock_buffer_load = true;
    }
  }
  return has_leaf_clock_buffer_load && (!has_stronger_leaf_clock_buffer_load || has_weaker_leaf_clock_buffer_load);
}

double TimingPropagator::getBufferDriveResistance(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  if (database.get_pin_map().count(pin_name) == 0) {
    return 0.0;
  }
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return 0.0;
  }
  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  if (timing_cell.get_port_map().count(pin.get_pin_name()) == 0) {
    return 0.0;
  }
  return timing_cell.get_port_map()[pin.get_pin_name()].get_drive_resistance();
}

std::vector<std::pair<std::string, TransType>> TimingPropagator::getClockPathPinList(std::string& clock_pin_name,
                                                                                     AnalysisType analysis_type, TransType trans_type)
{
  Database& database = STADM.getDatabase();
  std::vector<std::pair<std::string, TransType>> clock_path_pin_list;
  std::string pin_name = clock_pin_name;
  TransType current_trans_type = trans_type;
  while (!pin_name.empty() && database.get_timing_point_map().count(pin_name) > 0) {
    clock_path_pin_list.emplace_back(pin_name, current_trans_type);
    TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
    if (timing_point.get_clock_predecessor_map().count(analysis_type) == 0
        || timing_point.get_clock_predecessor_map()[analysis_type].count(current_trans_type) == 0) {
      break;
    }
    TransType predecessor_trans_type = current_trans_type;
    if (timing_point.get_clock_predecessor_trans_type_map().count(analysis_type) > 0
        && timing_point.get_clock_predecessor_trans_type_map()[analysis_type].count(current_trans_type) > 0) {
      predecessor_trans_type = timing_point.get_clock_predecessor_trans_type_map()[analysis_type][current_trans_type];
    }
    pin_name = timing_point.get_clock_predecessor_map()[analysis_type][current_trans_type];
    current_trans_type = predecessor_trans_type;
  }
  std::reverse(clock_path_pin_list.begin(), clock_path_pin_list.end());
  return clock_path_pin_list;
}

double TimingPropagator::getClockCommonPathArrival(std::pair<std::string, TransType>& common_pin, AnalysisType analysis_type)
{
  Database& database = STADM.getDatabase();
  if (database.get_timing_point_map().count(common_pin.first) == 0) {
    return 0.0;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[common_pin.first];
  return getClockArrival(timing_point, analysis_type, common_pin.second);
}

TimingCheckArc* TimingPropagator::getEndPointCheckArc(std::string& end_point, AnalysisType analysis_type)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return nullptr;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  for (TimingCheckArc& timing_check_arc : instance.get_check_arc_list()) {
    if (timing_check_arc.get_data_port() == end_point && isMatchCheckType(timing_check_arc, analysis_type)) {
      return &timing_check_arc;
    }
  }
  return nullptr;
}

bool TimingPropagator::isMatchCheckType(TimingCheckArc& timing_check_arc, AnalysisType analysis_type)
{
  if (analysis_type == AnalysisType::kMin) {
    return timing_check_arc.get_check_type() == TimingCheckType::kHold || timing_check_arc.get_check_type() == TimingCheckType::kRemoval;
  }
  return timing_check_arc.get_check_type() == TimingCheckType::kSetup || timing_check_arc.get_check_type() == TimingCheckType::kRecovery;
}

double TimingPropagator::getClockPeriod(std::string& clock_name)
{
  Database& database = STADM.getDatabase();
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
  if (clock_map.count(clock_name) > 0) {
    return clock_map[clock_name].get_period();
  }
  if (!clock_map.empty()) {
    return clock_map.begin()->second.get_period();
  }
  return 0.0;
}

void TimingPropagator::propagateRequiredArc(Arc& arc)
{
  Database& database = STADM.getDatabase();
  if (isDisableArc(arc)) {
    return;
  }
  if (shouldStopDataPropagation(arc)) {
    return;
  }
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (isFinite(sink_point.get_required())) {
    source_point.set_required(std::min(source_point.get_required(), sink_point.get_required() - arc.get_delay()));
  }
}

void TimingPropagator::updateSlack()
{
  Database& database = STADM.getDatabase();
  for (std::pair<const std::string, TimingPoint>& timing_pair : database.get_timing_point_map()) {
    TimingPoint& timing_point = timing_pair.second;
    if (isFinite(timing_point.get_arrival()) && isFinite(timing_point.get_required())) {
      timing_point.set_slack(timing_point.get_required() - timing_point.get_arrival());
    }
  }
}

void TimingPropagator::analyzeEndPointList()
{
  Database& database = STADM.getDatabase();
  double worst_slack = std::numeric_limits<double>::infinity();
  std::string worst_end_point;
  std::size_t checked_end_point_num = 0;
  std::size_t unconstrained_end_point_num = 0;
  std::size_t violation_num = 0;
  double total_negative_slack = 0.0;
  std::map<std::string, TimingPathGroup> timing_path_group_map;

  database.get_timing_path_group_list().clear();
  for (std::string& end_point : database.get_end_point_list()) {
    std::vector<TimingPath> timing_path_list = buildTimingPathList(end_point);
    if (timing_path_list.empty()) {
      ++unconstrained_end_point_num;
      continue;
    }
    ++checked_end_point_num;
    for (TimingPath& timing_path : timing_path_list) {
      TimingPathGroup& timing_path_group = getTimingPathGroup(timing_path_group_map, timing_path);
      insertTimingPath(timing_path_group, timing_path);
      updateWorstSlack(timing_path, worst_slack, worst_end_point);
      updateViolation(timing_path, violation_num, total_negative_slack);
    }
  }

  if (!std::isfinite(worst_slack)) {
    worst_slack = 0.0;
  }
  updateSummary(timing_path_group_map, checked_end_point_num, unconstrained_end_point_num, violation_num, worst_slack, total_negative_slack,
                worst_end_point);
  for (std::pair<const std::string, TimingPathGroup>& timing_path_group_pair : timing_path_group_map) {
    database.get_timing_path_group_list().push_back(timing_path_group_pair.second);
  }
}

TimingPathGroup& TimingPropagator::getTimingPathGroup(std::map<std::string, TimingPathGroup>& timing_path_group_map, TimingPath& timing_path)
{
  std::string group_name = getTimingPathGroupName(timing_path);
  if (timing_path_group_map.count(group_name) == 0) {
    timing_path_group_map[group_name] = initTimingPathGroup(group_name);
  }
  return timing_path_group_map[group_name];
}

std::string TimingPropagator::getTimingPathGroupName(TimingPath& timing_path)
{
  Database& database = STADM.getDatabase();
  if (timing_path.get_check_type() == TimingCheckType::kRecovery || timing_path.get_check_type() == TimingCheckType::kRemoval) {
    return "**async_default**";
  }
  if (!timing_path.get_clock_name().empty()) {
    return timing_path.get_clock_name();
  }
  if (!database.get_timing_constraint().get_clock_map().empty()) {
    return database.get_timing_constraint().get_clock_map().begin()->first;
  }
  return "default";
}

TimingPathGroup TimingPropagator::initTimingPathGroup(std::string& group_name)
{
  TimingPathGroup timing_path_group;
  timing_path_group.set_group_name(group_name);
  return timing_path_group;
}

std::vector<TimingPath> TimingPropagator::buildTimingPathList(std::string& end_point)
{
  Database& database = STADM.getDatabase();
  std::vector<TimingPath> timing_path_list;
  if (!isConstrainedEndPoint(end_point)) {
    return timing_path_list;
  }
  buildPathDiversionList(end_point);
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMax, PathSourceType::kInput)) {
    TimingPathState* path_state = getWorstSlackPathState(end_point, AnalysisType::kMax, PathSourceType::kInput);
    if (path_state != nullptr) {
      timing_path_list.push_back(
          buildTimingPath(end_point, AnalysisType::kMax, PathSourceType::kInput, path_state->get_trans_type(), path_state->get_start_point()));
    }
  }
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMax, PathSourceType::kRegister)) {
    TimingPathState* path_state = getWorstSlackPathState(end_point, AnalysisType::kMax, PathSourceType::kRegister);
    if (path_state != nullptr) {
      timing_path_list.push_back(
          buildTimingPath(end_point, AnalysisType::kMax, PathSourceType::kRegister, path_state->get_trans_type(), path_state->get_start_point()));
    }
  }
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMin, PathSourceType::kInput)) {
    TimingPathState* path_state = getWorstSlackPathState(end_point, AnalysisType::kMin, PathSourceType::kInput);
    if (path_state != nullptr) {
      timing_path_list.push_back(
          buildTimingPath(end_point, AnalysisType::kMin, PathSourceType::kInput, path_state->get_trans_type(), path_state->get_start_point()));
    }
  }
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMin, PathSourceType::kRegister)) {
    TimingPathState* path_state = getWorstSlackPathState(end_point, AnalysisType::kMin, PathSourceType::kRegister);
    if (path_state != nullptr) {
      timing_path_list.push_back(
          buildTimingPath(end_point, AnalysisType::kMin, PathSourceType::kRegister, path_state->get_trans_type(), path_state->get_start_point()));
    }
  }
  return timing_path_list;
}

void TimingPropagator::buildPathDiversionList(std::string& end_point)
{
  buildPathDiversionList(end_point, AnalysisType::kMax, PathSourceType::kInput);
  buildPathDiversionList(end_point, AnalysisType::kMax, PathSourceType::kRegister);
  buildPathDiversionList(end_point, AnalysisType::kMin, PathSourceType::kInput);
  buildPathDiversionList(end_point, AnalysisType::kMin, PathSourceType::kRegister);
}

void TimingPropagator::buildPathDiversionList(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type)
{
  Database& database = STADM.getDatabase();
  TimingPoint& end_timing_point = database.get_timing_point_map()[end_point];
  TimingPathState* path_state = getWorstPathState(end_timing_point, analysis_type, source_type);
  if (path_state == nullptr) {
    return;
  }
  std::vector<std::string> path_pin_name_list;
  std::vector<TransType> path_trans_type_list;
  std::string& start_point = path_state->get_start_point();
  buildPathTrace(end_point, analysis_type, source_type, path_state->get_trans_type(), start_point, path_pin_name_list, path_trans_type_list);
  std::vector<std::size_t> path_arc_idx_list = getPathArcIdxList(path_pin_name_list, path_trans_type_list, analysis_type, source_type, start_point);
  buildPathDiversionList(end_point, analysis_type, source_type, path_pin_name_list, path_trans_type_list, path_arc_idx_list);
}

void TimingPropagator::buildPathDiversionList(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                                              std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                                              std::vector<std::size_t>& path_arc_idx_list)
{
  Database& database = STADM.getDatabase();
  for (std::size_t sink_idx = 1; sink_idx < path_pin_name_list.size(); sink_idx++) {
    std::string& sink_pin = path_pin_name_list[sink_idx];
    TransType sink_trans_type = path_trans_type_list[sink_idx];
    std::size_t path_arc_idx = path_arc_idx_list[sink_idx - 1];
    for (std::size_t diversion_arc_idx : database.get_incoming_arc_list_map()[sink_pin]) {
      if (diversion_arc_idx == path_arc_idx || isDisableArc(database.get_arc_list()[diversion_arc_idx])
          || shouldStopDataPropagation(database.get_arc_list()[diversion_arc_idx])) {
        continue;
      }
      Arc& diversion_arc = database.get_arc_list()[diversion_arc_idx];
      TimingPoint& source_point = database.get_timing_point_map()[diversion_arc.get_source_pin()];
      for (TransType input_trans_type : {TransType::kRise, TransType::kFall}) {
        if (!hasPathState(source_point, analysis_type, source_type, input_trans_type)
            || !isOutputTransType(diversion_arc, analysis_type, input_trans_type, sink_trans_type)) {
          continue;
        }
        std::map<std::string, TimingPathState>& source_path_state_map = getPathStateMap(source_point, analysis_type, source_type, input_trans_type);
        for (std::pair<const std::string, TimingPathState>& source_path_state_pair : source_path_state_map) {
          TimingPathState& source_path_state = source_path_state_pair.second;
          if (!isFinite(source_path_state.get_arrival())) {
            continue;
          }
          buildPathDiversionState(analysis_type, source_type, path_pin_name_list, path_trans_type_list, path_arc_idx_list, sink_idx,
                                  diversion_arc_idx, input_trans_type, source_path_state);
        }
      }
    }
  }
}

void TimingPropagator::buildPathDiversionState(AnalysisType analysis_type, PathSourceType source_type,
                                               std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                                               std::vector<std::size_t>& path_arc_idx_list, std::size_t sink_idx, std::size_t diversion_arc_idx,
                                               TransType input_trans_type, TimingPathState& source_path_state)
{
  Database& database = STADM.getDatabase();
  Arc& diversion_arc = database.get_arc_list()[diversion_arc_idx];
  TransType sink_trans_type = path_trans_type_list[sink_idx];
  double diversion_arc_delay = getArcDelay(diversion_arc, analysis_type, input_trans_type, sink_trans_type);
  double arrival = roundTime(source_path_state.get_arrival() + diversion_arc_delay);
  std::string predecessor = diversion_arc.get_source_pin();
  if (!updateDiversionPathState(path_pin_name_list[sink_idx], analysis_type, source_type, sink_trans_type, source_path_state, predecessor,
                                diversion_arc_idx, diversion_arc_delay, input_trans_type, arrival)) {
    return;
  }

  for (std::size_t path_idx = sink_idx + 1; path_idx < path_pin_name_list.size(); path_idx++) {
    Arc& path_arc = database.get_arc_list()[path_arc_idx_list[path_idx - 1]];
    TransType predecessor_trans_type = path_trans_type_list[path_idx - 1];
    TransType trans_type = path_trans_type_list[path_idx];
    double arc_delay = getArcDelay(path_arc, analysis_type, predecessor_trans_type, trans_type);
    arrival = roundTime(arrival + arc_delay);
    std::string& path_predecessor = path_pin_name_list[path_idx - 1];
    if (!updateDiversionPathState(path_pin_name_list[path_idx], analysis_type, source_type, trans_type, source_path_state, path_predecessor,
                                  path_arc_idx_list[path_idx - 1], arc_delay, predecessor_trans_type, arrival)) {
      return;
    }
  }
}

bool TimingPropagator::isOutputTransType(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type)
{
  if (!isClockArcTriggerTrans(arc, input_trans_type)) {
    return false;
  }
  for (TransType arc_output_trans_type : getOutputTransTypeList(arc, analysis_type, input_trans_type)) {
    if (arc_output_trans_type == output_trans_type) {
      return true;
    }
  }
  return false;
}

bool TimingPropagator::updateDiversionPathState(std::string& pin_name, AnalysisType analysis_type, PathSourceType source_type,
                                                TransType trans_type, TimingPathState& source_path_state, std::string& predecessor,
                                                std::size_t predecessor_arc_idx, double predecessor_arc_delay, TransType predecessor_trans_type, double arrival)
{
  Database& database = STADM.getDatabase();
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  std::string& start_point = source_path_state.get_start_point();
  std::map<std::string, TimingPathState>& path_state_map = getPathStateMap(timing_point, analysis_type, source_type, trans_type);
  if (path_state_map.count(start_point) > 0 && !isBetterArrival(arrival, path_state_map[start_point].get_arrival(), analysis_type)) {
    return false;
  }
  TimingPathState& path_state = path_state_map[start_point];
  path_state.set_arrival(arrival);
  path_state.set_slew(getDataSlew(timing_point, analysis_type, trans_type));
  path_state.set_start_point(start_point);
  path_state.set_predecessor(predecessor);
  path_state.set_predecessor_arc_idx(predecessor_arc_idx);
  path_state.set_predecessor_arc_delay(predecessor_arc_delay);
  path_state.set_launch_time(source_path_state.get_launch_time());
  path_state.set_clock_name(source_path_state.get_clock_name());
  path_state.set_crpr_clock_pin(source_path_state.get_crpr_clock_pin());
  path_state.set_trans_type(trans_type);
  path_state.set_predecessor_trans_type(predecessor_trans_type);
  path_state.set_crpr_clock_trans_type(source_path_state.get_crpr_clock_trans_type());
  return true;
}

TimingPathState* TimingPropagator::getWorstSlackPathState(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type)
{
  Database& database = STADM.getDatabase();
  TimingPoint& timing_point = database.get_timing_point_map()[end_point];
  TimingPathState* worst_path_state = nullptr;
  double worst_slack = std::numeric_limits<double>::infinity();
  for (TransType trans_type : {TransType::kRise, TransType::kFall}) {
    if (!hasPathState(timing_point, analysis_type, source_type, trans_type)) {
      continue;
    }
    std::map<std::string, TimingPathState>& path_state_map = getPathStateMap(timing_point, analysis_type, source_type, trans_type);
    for (std::pair<const std::string, TimingPathState>& path_state_pair : path_state_map) {
      TimingPathState& path_state = path_state_pair.second;
      if (!isFinite(path_state.get_arrival())) {
        continue;
      }
      TimingCheckArc* timing_check_arc = getEndPointCheckArc(end_point, analysis_type);
      if (timing_check_arc != nullptr && !isMatchCheckTransType(*timing_check_arc, path_state.get_trans_type())) {
        continue;
      }
      double required_time = calcPathRequiredTime(end_point, path_state, analysis_type);
      double slack = calcPathSlack(path_state, required_time, analysis_type);
      if (worst_path_state == nullptr || slack < worst_slack - STA_ERROR) {
        worst_slack = slack;
        worst_path_state = &path_state;
      }
    }
  }
  return worst_path_state;
}

double TimingPropagator::calcPathRequiredTime(std::string& end_point, TimingPathState& end_path_state, AnalysisType analysis_type)
{
  return getEndPointRequired(end_path_state, end_point, end_path_state.get_arrival(), analysis_type);
}

double TimingPropagator::calcPathSlack(TimingPathState& end_path_state, double required_time, AnalysisType analysis_type)
{
  if (analysis_type == AnalysisType::kMin) {
    return roundTime(end_path_state.get_arrival() - required_time);
  }
  return roundTime(required_time - end_path_state.get_arrival());
}

bool TimingPropagator::isConstrainedEndPoint(std::string& end_point)
{
  return isOutputEndPoint(end_point) || isRegisterEndPoint(end_point) || isTimingCheckEndPoint(end_point);
}

bool TimingPropagator::isOutputEndPoint(std::string& end_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kOutput || pin.get_direction() == PinDirection::kInout)
         && hasOutputDelay(end_point);
}

bool TimingPropagator::hasOutputDelay(std::string& end_point)
{
  Database& database = STADM.getDatabase();
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  if (port_constraint_map.count(end_point) == 0) {
    return false;
  }
  TimingPortConstraint& port_constraint = port_constraint_map[end_point];
  return port_constraint.get_has_output_delay_max() || port_constraint.get_has_output_delay_min();
}

bool TimingPropagator::isRegisterEndPoint(std::string& end_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && hasClockPoint(instance.get_clock_pin_name()) && end_point == instance.get_data_pin_name();
}

bool TimingPropagator::isTimingCheckEndPoint(std::string& end_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || !hasClockPoint(instance.get_clock_pin_name())) {
    return false;
  }
  for (TimingCheckArc& timing_check_arc : instance.get_check_arc_list()) {
    if (timing_check_arc.get_data_port() == end_point) {
      return true;
    }
  }
  return false;
}

TimingPath TimingPropagator::buildTimingPath(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                                             TransType trans_type, std::string& start_point)
{
  Database& database = STADM.getDatabase();
  std::vector<std::string> path_pin_name_list;
  std::vector<TransType> path_trans_type_list;
  buildPathTrace(end_point, analysis_type, source_type, trans_type, start_point, path_pin_name_list, path_trans_type_list);
  std::vector<std::size_t> path_arc_idx_list = getPathArcIdxList(path_pin_name_list, path_trans_type_list, analysis_type, source_type, start_point);
  TimingPathState& end_path_state = getPathState(database.get_timing_point_map()[end_point], analysis_type, source_type, trans_type, start_point);
  double required_time = getEndPointRequired(end_path_state, end_point, end_path_state.get_arrival(), analysis_type);
  double slack
      = analysis_type == AnalysisType::kMin ? roundTime(end_path_state.get_arrival() - required_time) : roundTime(required_time - end_path_state.get_arrival());
  TimingPath timing_path;
  timing_path.set_start_point(start_point);
  timing_path.set_end_point(end_point);
  timing_path.set_path_delay(end_path_state.get_arrival());
  timing_path.set_required_time(required_time);
  timing_path.set_slack(slack);
  timing_path.set_level(database.get_timing_point_map()[end_point].get_level());
  timing_path.set_analysis_type(analysis_type);
  timing_path.set_source_type(source_type);
  timing_path.set_trans_type(trans_type);
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(end_point, analysis_type);
  if (timing_check_arc != nullptr) {
    timing_path.set_check_type(timing_check_arc->get_check_type());
    timing_path.set_check_time(getEndPointCheckTime(end_point, *timing_check_arc, analysis_type, trans_type, end_path_state.get_slew()));
  }
  updateClockInfo(timing_path, analysis_type, source_type, trans_type, start_point);

  Arc* arc = nullptr;
  for (size_t i = 0; i < path_pin_name_list.size(); i++) {
    double arc_delay = 0.0;
    if (i > 0) {
      arc = &database.get_arc_list()[path_arc_idx_list[i - 1]];
      TimingPathState& path_state
          = getPathState(database.get_timing_point_map()[path_pin_name_list[i]], analysis_type, source_type, path_trans_type_list[i], start_point);
      arc_delay = path_state.get_predecessor_arc_delay();
      updatePathDelay(timing_path, arc, arc_delay);
    }
    timing_path.get_point_list().push_back(makeTimingPathPoint(path_pin_name_list[i], arc, analysis_type, source_type,
                                                               i > 0 ? path_trans_type_list[i - 1] : TransType::kNone, path_trans_type_list[i], start_point));
    if (i > 0) {
      timing_path.get_point_list().back().set_arc_delay(arc_delay);
    }
  }
  return timing_path;
}

void TimingPropagator::buildPathTrace(std::string& end_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type,
                                      std::string& start_point, std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list)
{
  Database& database = STADM.getDatabase();
  std::string pin_name = end_point;
  TransType current_trans_type = trans_type;
  while (!pin_name.empty()) {
    path_pin_name_list.push_back(pin_name);
    path_trans_type_list.push_back(current_trans_type);
    TimingPathState& path_state = getPathState(database.get_timing_point_map()[pin_name], analysis_type, source_type, current_trans_type, start_point);
    pin_name = path_state.get_predecessor();
    current_trans_type = path_state.get_predecessor_trans_type();
  }
  std::reverse(path_pin_name_list.begin(), path_pin_name_list.end());
  std::reverse(path_trans_type_list.begin(), path_trans_type_list.end());
}

std::vector<std::size_t> TimingPropagator::getPathArcIdxList(std::vector<std::string>& path_pin_name_list,
                                                             std::vector<TransType>& path_trans_type_list, AnalysisType analysis_type,
                                                             PathSourceType source_type, std::string& start_point)
{
  Database& database = STADM.getDatabase();
  std::vector<std::size_t> path_arc_idx_list;
  for (size_t i = 1; i < path_pin_name_list.size(); i++) {
    TimingPoint& timing_point = database.get_timing_point_map()[path_pin_name_list[i]];
    TimingPathState& path_state = getPathState(timing_point, analysis_type, source_type, path_trans_type_list[i], start_point);
    path_arc_idx_list.push_back(path_state.get_predecessor_arc_idx());
  }
  return path_arc_idx_list;
}

void TimingPropagator::updatePathDelay(TimingPath& timing_path, Arc* arc, double arc_delay)
{
  if (arc == nullptr) {
    return;
  }
  if (arc->get_type() == ArcType::kCell) {
    timing_path.set_cell_delay(timing_path.get_cell_delay() + arc_delay);
  } else if (arc->get_type() == ArcType::kNet) {
    timing_path.set_net_delay(timing_path.get_net_delay() + arc_delay);
  }
}

void TimingPropagator::updateClockInfo(TimingPath& timing_path, AnalysisType analysis_type, PathSourceType source_type,
                                       TransType trans_type, std::string& start_point)
{
  Database& database = STADM.getDatabase();
  TimingPathState& end_path_state
      = getPathState(database.get_timing_point_map()[timing_path.get_end_point()], analysis_type, source_type, trans_type, start_point);
  timing_path.set_launch_time(end_path_state.get_launch_time());
  timing_path.set_clock_name(end_path_state.get_clock_name());
  std::string common_pin_name;
  double cppr = getClockReconvergencePessimism(end_path_state, timing_path.get_end_point(), analysis_type, common_pin_name);
  if (analysis_type == AnalysisType::kMin) {
    cppr = -cppr;
  }
  timing_path.set_last_common_pin(common_pin_name);
  timing_path.set_capture_time(getEndPointCaptureTime(timing_path.get_end_point(), analysis_type) + cppr);
  timing_path.set_launch_clock_network_delay(end_path_state.get_launch_time());
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(timing_path.get_end_point(), analysis_type);
  TransType capture_trans_type = timing_check_arc == nullptr ? TransType::kRise : getClockTransType(*timing_check_arc);
  timing_path.set_capture_clock_network_delay(
      getEndPointClockArrival(timing_path.get_end_point(), getCaptureAnalysisType(analysis_type), capture_trans_type));
  timing_path.set_clock_reconvergence_pessimism(cppr);

  Pin& end_pin = database.get_pin_map()[timing_path.get_end_point()];
  if (end_pin.get_is_port() || database.get_instance_map().count(end_pin.get_instance_name()) == 0) {
    return;
  }
  Instance& instance = database.get_instance_map()[end_pin.get_instance_name()];
  if (instance.get_is_sequential() && isTimingCheckEndPoint(timing_path.get_end_point())) {
    timing_path.set_capture_clock_pin(instance.get_clock_pin_name());
    timing_path.set_setup_time(timing_path.get_check_time());
  }
}

TimingPathPoint TimingPropagator::makeTimingPathPoint(std::string& pin_name, Arc* arc, AnalysisType analysis_type,
                                                      PathSourceType source_type, TransType input_trans_type, TransType trans_type, std::string& start_point)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  TimingPathState& path_state = getPathState(timing_point, analysis_type, source_type, trans_type, start_point);
  TimingPathPoint path_point;
  path_point.set_pin_name(pin_name);
  path_point.set_instance_name(pin.get_instance_name());
  if (!pin.get_instance_name().empty()) {
    path_point.set_cell_name(database.get_instance_map()[pin.get_instance_name()].get_cell_name());
  }
  path_point.set_net_name(pin.get_net_name());
  path_point.set_arrival(path_state.get_arrival());
  path_point.set_required(timing_point.get_required());
  path_point.set_slack(timing_point.get_slack());
  path_point.set_trans_type(trans_type);
  if (arc != nullptr) {
    path_point.set_arc_name(arc->get_arc_name());
    path_point.set_source_pin(arc->get_source_pin());
    path_point.set_sink_pin(arc->get_sink_pin());
    path_point.set_arc_type(arc->get_type());
    path_point.set_arc_delay(path_state.get_predecessor_arc_delay());
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

void TimingPropagator::updateWorstSlack(TimingPath& timing_path, double& worst_slack, std::string& worst_end_point)
{
  if (timing_path.get_slack() < worst_slack) {
    worst_slack = timing_path.get_slack();
    worst_end_point = timing_path.get_end_point();
  }
}

void TimingPropagator::updateViolation(TimingPath& timing_path, std::size_t& violation_num, double& total_negative_slack)
{
  if (timing_path.get_slack() < 0.0) {
    ++violation_num;
    total_negative_slack += timing_path.get_slack();
  }
}

std::size_t TimingPropagator::getTimingPathNum(std::map<std::string, TimingPathGroup>& timing_path_group_map)
{
  std::size_t timing_path_num = 0;
  for (std::pair<const std::string, TimingPathGroup>& timing_path_group_pair : timing_path_group_map) {
    timing_path_num += getTimingPathNum(timing_path_group_pair.second);
  }
  return timing_path_num;
}

std::size_t TimingPropagator::getTimingPathNum(TimingPathGroup& timing_path_group)
{
  std::size_t timing_path_num = 0;
  for (auto& [end_point, timing_path_end] : timing_path_group.get_timing_path_end_map()) {
    timing_path_num += timing_path_end.get_timing_path_list().size();
  }
  return timing_path_num;
}

void TimingPropagator::updateSummary(std::map<std::string, TimingPathGroup>& timing_path_group_map, std::size_t checked_end_point_num,
                                     std::size_t unconstrained_end_point_num, std::size_t violation_num, double worst_slack, double total_negative_slack,
                                     std::string& worst_end_point)
{
  updateSummary(getTimingPathNum(timing_path_group_map), checked_end_point_num, unconstrained_end_point_num, violation_num, worst_slack,
                total_negative_slack, worst_end_point);
}

void TimingPropagator::updateSummary(std::size_t timing_path_num, std::size_t checked_end_point_num, std::size_t unconstrained_end_point_num,
                                     std::size_t violation_num, double worst_slack, double total_negative_slack, std::string& worst_end_point)
{
  Database& database = STADM.getDatabase();
  TPSummary& tp_summary = database.get_summary().tp_summary;
  tp_summary.timing_path_num = timing_path_num;
  tp_summary.checked_end_point_num = checked_end_point_num;
  tp_summary.unconstrained_end_point_num = unconstrained_end_point_num;
  tp_summary.violating_end_point_num = violation_num;
  tp_summary.worst_slack = worst_slack;
  tp_summary.total_negative_slack = total_negative_slack;
  tp_summary.worst_end_point = worst_end_point;
}

}  // namespace ista
