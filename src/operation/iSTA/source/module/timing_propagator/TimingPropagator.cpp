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

  buildArcDelayList(database);
  propagateArrival(database);
  propagateRequired(database);
  analyzeEndPointList(database);

  std::size_t loop_vertex_num = database.get_pin_map().size() - database.get_timing_order_list().size();
  STALOG.info(Loc::current(), "Calculate iSTA delay: arcs=", database.get_arc_list().size());
  STALOG.info(Loc::current(), "Propagate iSTA timing: timing_order=", database.get_timing_order_list().size(), " loop_vertices=", loop_vertex_num);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

TimingPropagator* TimingPropagator::_tp_instance = nullptr;

void TimingPropagator::buildArcDelayList(Database& database)
{
  for (Arc& arc : database.get_arc_list()) {
    buildArcDelay(database, arc);
  }
}

void TimingPropagator::buildArcDelay(Database& database, Arc& arc)
{
  buildAnalysisArcDelay(database, arc, AnalysisType::kMax);
  buildAnalysisArcDelay(database, arc, AnalysisType::kMin);
  arc.set_delay_max(std::max(arc.get_trans_delay_map()[AnalysisType::kMax][TransType::kRise],
                             arc.get_trans_delay_map()[AnalysisType::kMax][TransType::kFall]));
  arc.set_delay_min(std::min(arc.get_trans_delay_map()[AnalysisType::kMin][TransType::kRise],
                             arc.get_trans_delay_map()[AnalysisType::kMin][TransType::kFall]));
  arc.set_delay(arc.get_delay_max());
}

void TimingPropagator::buildAnalysisArcDelay(Database& database, Arc& arc, AnalysisType analysis_type)
{
  buildTransArcDelay(database, arc, analysis_type, TransType::kRise);
  buildTransArcDelay(database, arc, analysis_type, TransType::kFall);
}

void TimingPropagator::buildTransArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  if (arc.get_type() == ArcType::kNet) {
    double delay = calcNetArcDelay(database, arc);
    arc.get_input_output_delay_map()[analysis_type][input_trans_type][input_trans_type] = delay;
    arc.get_trans_delay_map()[analysis_type][input_trans_type] = delay;
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    return;
  }

  TimingCellArc* timing_cell_arc = getTimingCellArc(database, arc);
  if (timing_cell_arc == nullptr || timing_cell_arc->get_lib_arc_set() == nullptr) {
    double delay = calcCellArcDelay(database, arc, analysis_type, input_trans_type);
    arc.get_input_output_delay_map()[analysis_type][input_trans_type][input_trans_type] = delay;
    arc.get_trans_delay_map()[analysis_type][input_trans_type] = delay;
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    return;
  }
  if (!isClockArcTriggerTrans(*timing_cell_arc, input_trans_type)) {
    return;
  }

  for (TransType output_trans_type : getOutputTransTypeList(*timing_cell_arc, input_trans_type)) {
    double delay = calcTimingCellArcDelay(database, arc, *timing_cell_arc, analysis_type, input_trans_type, output_trans_type);
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
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr || lib_arc_set->front() == nullptr) {
    return true;
  }
  idb::LibArc* lib_arc = lib_arc_set->front();
  if (lib_arc->isFallingTriggerArc()) {
    return input_trans_type == TransType::kFall;
  }
  return input_trans_type == TransType::kRise;
}

double TimingPropagator::calcArcDelay(Database& database, Arc& arc)
{
  if (arc.get_type() == ArcType::kCell) {
    return calcCellArcDelay(database, arc, AnalysisType::kMax);
  }
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(database, arc);
  }
  return 0.0;
}

double TimingPropagator::calcCellArcDelay(Database& database, Arc& arc, AnalysisType analysis_type)
{
  return calcCellArcDelay(database, arc, analysis_type, TransType::kRise);
}

double TimingPropagator::calcCellArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  if (arc.get_type() == ArcType::kNet) {
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    return calcNetArcDelay(database, arc);
  }
  TimingCellArc* timing_cell_arc = getTimingCellArc(database, arc);
  if (timing_cell_arc != nullptr) {
    return calcTimingCellArcDelay(database, arc, *timing_cell_arc, analysis_type, input_trans_type);
  }
  arc.get_trans_type_map()[input_trans_type] = input_trans_type;
  return 1.0;
}

TimingCellArc* TimingPropagator::getTimingCellArc(Database& database, Arc& arc)
{
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

double TimingPropagator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type)
{
  return calcTimingCellArcDelay(database, arc, timing_cell_arc, analysis_type, TransType::kRise);
}

double TimingPropagator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr) {
    arc.get_trans_type_map()[input_trans_type] = input_trans_type;
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  idb::TransType idb_input_trans_type = getIDBTransType(input_trans_type);
  idb::TransType idb_output_trans_type = getOutputTransType(lib_arc_set, idb_input_trans_type);
  TransType output_trans_type = getTransType(idb_output_trans_type);
  arc.get_trans_type_map()[input_trans_type] = output_trans_type;
  if (!lib_arc_set->isMatchTimingType(idb_output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  double input_slew = 0.0;
  double raw_output_load = getArcOutputLoad(database, arc, analysis_type, output_trans_type);
  double output_load = convertOutputLoad(lib_arc_set, raw_output_load);
  std::vector<double> delay_list = lib_arc_set->getDelayOrConstrainCheckNs(idb_input_trans_type, idb_output_trans_type, input_slew, output_load);
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type, TransType output_trans_type)
{
  return calcTimingCellArcDelay(database, arc, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, 0.0);
}

double TimingPropagator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr) {
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  idb::TransType idb_input_trans_type = getIDBTransType(input_trans_type);
  idb::TransType idb_output_trans_type = getIDBTransType(output_trans_type);
  if (!lib_arc_set->isMatchTimingType(idb_output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  double output_load = convertOutputLoad(lib_arc_set, getArcOutputLoad(database, arc, analysis_type, output_trans_type));
  std::vector<double> delay_list = lib_arc_set->getDelayOrConstrainCheckNs(idb_input_trans_type, idb_output_trans_type, input_slew, output_load);
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcSlew(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                               TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr) {
    return input_slew;
  }
  idb::TransType idb_output_trans_type = getIDBTransType(output_trans_type);
  if (!lib_arc_set->isMatchTimingType(idb_output_trans_type)) {
    return input_slew;
  }
  double output_load = getArcOutputLoad(database, arc, analysis_type, output_trans_type);
  double output_slew = calcTimingCellArcSlew(database, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew, output_load);
  return output_slew;
}

double TimingPropagator::calcTimingCellArcDelay(Database& database, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                                TransType input_trans_type, TransType output_trans_type, double input_slew, double output_load)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr) {
    if (analysis_type == AnalysisType::kMin) {
      return timing_cell_arc.get_delay_min();
    }
    return timing_cell_arc.get_delay_max();
  }
  idb::TransType idb_input_trans_type = getIDBTransType(input_trans_type);
  idb::TransType idb_output_trans_type = getIDBTransType(output_trans_type);
  if (!lib_arc_set->isMatchTimingType(idb_output_trans_type)) {
    return timing_cell_arc.get_delay();
  }
  double converted_output_load = convertOutputLoad(lib_arc_set, output_load);
  std::vector<double> delay_list = lib_arc_set->getDelayOrConstrainCheckNs(idb_input_trans_type, idb_output_trans_type, input_slew, converted_output_load);
  if (analysis_type == AnalysisType::kMin) {
    return delay_list.back();
  }
  return delay_list.front();
}

double TimingPropagator::calcTimingCellArcSlew(Database& database, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                               TransType input_trans_type, TransType output_trans_type, double input_slew, double output_load)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr) {
    return input_slew;
  }
  idb::TransType idb_input_trans_type = getIDBTransType(input_trans_type);
  idb::TransType idb_output_trans_type = getIDBTransType(output_trans_type);
  if (!lib_arc_set->isMatchTimingType(idb_output_trans_type)) {
    return input_slew;
  }
  double converted_output_load = convertOutputLoad(lib_arc_set, output_load);
  std::vector<double> slew_list = lib_arc_set->getSlewNs(idb_input_trans_type, idb_output_trans_type, input_slew, converted_output_load);
  if (analysis_type == AnalysisType::kMin) {
    return recoverTableSlew(lib_arc_set, slew_list.back());
  }
  return recoverTableSlew(lib_arc_set, slew_list.front());
}

double TimingPropagator::recoverTableSlew(idb::LibArcSet* lib_arc_set, double output_slew)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  idb::LibLibrary* lib_library = lib_arc->get_owner_cell()->get_owner_lib();
  return output_slew / lib_library->get_slew_derate_from_library();
}

idb::TransType TimingPropagator::getIDBTransType(TransType trans_type)
{
  if (trans_type == TransType::kFall) {
    return idb::TransType::kFall;
  }
  return idb::TransType::kRise;
}

TransType TimingPropagator::getTransType(idb::TransType trans_type)
{
  if (trans_type == idb::TransType::kFall) {
    return TransType::kFall;
  }
  return TransType::kRise;
}

idb::TransType TimingPropagator::getOutputTransType(idb::LibArcSet* lib_arc_set, idb::TransType input_trans_type)
{
  if (lib_arc_set->isNegativeArc()) {
    return input_trans_type == idb::TransType::kRise ? idb::TransType::kFall : idb::TransType::kRise;
  }
  return input_trans_type;
}

std::vector<TransType> TimingPropagator::getOutputTransTypeList(TimingCellArc& timing_cell_arc, TransType input_trans_type)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  std::vector<TransType> output_trans_type_list;
  if (!lib_arc_set->isUnateArc() || lib_arc_set->isTwoTypeSenseArcSet() || timing_cell_arc.get_is_clock_arc()) {
    for (TransType output_trans_type : {TransType::kRise, TransType::kFall}) {
      if (lib_arc_set->isMatchTimingType(getIDBTransType(output_trans_type))) {
        output_trans_type_list.push_back(output_trans_type);
      }
    }
    return output_trans_type_list;
  }

  TransType output_trans_type = getTransType(getOutputTransType(lib_arc_set, getIDBTransType(input_trans_type)));
  if (lib_arc_set->isMatchTimingType(getIDBTransType(output_trans_type))) {
    output_trans_type_list.push_back(output_trans_type);
  }
  return output_trans_type_list;
}

double TimingPropagator::convertOutputLoad(idb::LibArcSet* lib_arc_set, double output_load)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  idb::LibLibrary* lib_library = lib_arc->get_owner_cell()->get_owner_lib();
  if (lib_library->get_cap_unit() == idb::CapacitiveUnit::kFF) {
    return static_cast<int>(std::ceil(output_load * static_cast<int64_t>(idb::g_pf2ff)));
  }
  if (lib_library->get_cap_unit() == idb::CapacitiveUnit::kPF) {
    return output_load;
  }
  return output_load;
}

double TimingPropagator::getArcOutputLoad(Database& database, Arc& arc, AnalysisType analysis_type, TransType output_trans_type)
{
  std::string& sink_pin_name = arc.get_sink_pin();
  if (database.get_pin_map().count(sink_pin_name) == 0) {
    return 0.0;
  }
  Pin& sink_pin = database.get_pin_map()[sink_pin_name];
  if (sink_pin.get_net_name().empty() || database.get_net_map().count(sink_pin.get_net_name()) == 0) {
    return 0.0;
  }
  Net& net = database.get_net_map()[sink_pin.get_net_name()];
  return getNetOutputLoad(database, net, analysis_type, output_trans_type);
}

double TimingPropagator::getOutputPinLoad(Database& database, std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type)
{
  if (database.get_pin_map().count(output_pin) == 0) {
    return 0.0;
  }
  Pin& pin = database.get_pin_map()[output_pin];
  if (pin.get_net_name().empty() || database.get_net_map().count(pin.get_net_name()) == 0) {
    return 0.0;
  }
  Net& net = database.get_net_map()[pin.get_net_name()];
  return getNetOutputLoad(database, net, analysis_type, output_trans_type);
}

double TimingPropagator::getNetOutputLoad(Database& database, Net& net, AnalysisType analysis_type, TransType output_trans_type)
{
  double output_load = 0.0;
  for (std::string& load_pin_name : net.get_load_pin_list()) {
    output_load += getPinCapacitance(database, load_pin_name, analysis_type, output_trans_type);
  }
  return output_load;
}

double TimingPropagator::getPinCapacitance(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
{
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

double TimingPropagator::convertCheckSlewForLookup(TimingCheckArc& timing_check_arc, double data_slew)
{
  idb::LibArc* lib_arc = timing_check_arc.get_lib_arc();
  if (lib_arc == nullptr && timing_check_arc.get_lib_arc_set() != nullptr) {
    lib_arc = timing_check_arc.get_lib_arc_set()->front();
  }
  if (lib_arc == nullptr) {
    return data_slew;
  }
  idb::LibLibrary* lib_library = lib_arc->get_owner_cell()->get_owner_lib();
  if (lib_library->get_time_unit() == idb::TimeUnit::kPS) {
    return data_slew * 1e3;
  }
  if (lib_library->get_time_unit() == idb::TimeUnit::kFS) {
    return data_slew * 1e6;
  }
  return data_slew;
}

double TimingPropagator::calcNetArcDelay(Database& database, Arc& arc)
{
  if (database.get_parasitic_library().get_net_map().count(arc.get_owner_name()) > 0) {
    return calcParasiticDelay(database, arc);
  }
  return 0.0;
}

double TimingPropagator::calcParasiticDelay(Database& database, Arc& arc)
{
  ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[arc.get_owner_name()];
  double source_capacitance = getParasiticNodeCapacitance(parasitic_net, arc.get_source_pin());
  double sink_capacitance = getParasiticNodeCapacitance(parasitic_net, arc.get_sink_pin());
  double resistance = getParasiticTotalResistance(parasitic_net);
  return resistance * (source_capacitance + sink_capacitance) * 0.5;
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

double TimingPropagator::getParasiticTotalResistance(ParasiticNet& parasitic_net)
{
  double resistance = 0.0;
  for (ParasiticResistor& parasitic_resistor : parasitic_net.get_resistor_list()) {
    resistance += parasitic_resistor.get_resistance();
  }
  return resistance;
}

void TimingPropagator::propagateArrival(Database& database)
{
  initTimingPointList(database);
  markClockPointList(database);
  propagateClockArrival(database);
  seedStartPointList(database);
  propagateDataSlewDelay(database);

  for (std::string& pin_name : database.get_timing_order_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      if (shouldStopDataPropagation(database, database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateArrivalArc(database, arc_idx);
    }
  }
}

bool TimingPropagator::isDisableArc(Arc& arc)
{
  return arc.get_is_disable_arc() || arc.get_is_loop_disable();
}

bool TimingPropagator::shouldStopDataPropagation(Database& database, Arc& arc)
{
  return arc.get_type() == ArcType::kNet && isSequentialClockPin(database, arc.get_sink_pin());
}

bool TimingPropagator::isSequentialClockPin(Database& database, std::string& pin_name)
{
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_clock_pin_name();
}

void TimingPropagator::initTimingPointList(Database& database)
{
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

void TimingPropagator::markClockPointList(Database& database)
{
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    for (std::string& clock_source : clock_pair.second.get_source_list()) {
      markClockPoint(database, clock_source);
    }
  }
}

void TimingPropagator::markClockPoint(Database& database, std::string& clock_source)
{
  if (database.get_timing_point_map().count(clock_source) == 0) {
    return;
  }

  std::queue<std::string> pin_queue;
  database.get_timing_point_map()[clock_source].set_is_clock_point(true);
  pin_queue.push(clock_source);

  while (!pin_queue.empty()) {
    std::string pin_name = pin_queue.front();
    pin_queue.pop();

    if (shouldStopClockPropagation(database, pin_name)) {
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

void TimingPropagator::propagateClockArrival(Database& database)
{
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    for (std::string& clock_source : clock_pair.second.get_source_list()) {
      seedClockArrival(database, clock_source);
    }
  }

  for (std::string& pin_name : database.get_timing_order_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
    if (!timing_point.get_is_clock_point()) {
      continue;
    }
    if (shouldStopClockPropagation(database, pin_name)) {
      continue;
    }
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateClockArrivalArc(database, arc_idx, AnalysisType::kMax);
      propagateClockArrivalArc(database, arc_idx, AnalysisType::kMin);
    }
  }
}

void TimingPropagator::seedClockArrival(Database& database, std::string& clock_source)
{
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

void TimingPropagator::propagateClockArrivalArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type)
{
  propagateClockArrivalArc(database, arc_idx, analysis_type, TransType::kRise);
  propagateClockArrivalArc(database, arc_idx, analysis_type, TransType::kFall);
}

void TimingPropagator::propagateClockArrivalArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type)
{
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
    updateClockPathState(database, arc, source_point, sink_point, analysis_type, input_trans_type, output_trans_type);
  }
}

void TimingPropagator::updateClockPathState(Database& database, Arc& arc, TimingPoint& source_point, TimingPoint& sink_point,
                                            AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type)
{
  double source_slew = source_point.get_clock_slew_map()[analysis_type][input_trans_type];
  double arc_delay = calcArcDelay(database, arc, analysis_type, input_trans_type, output_trans_type, source_slew);
  double candidate_arrival = roundTime(getClockArrival(source_point, analysis_type, input_trans_type) + arc_delay);
  if (!hasClockArrival(sink_point, analysis_type, output_trans_type)
      || isBetterArrival(candidate_arrival, getClockArrival(sink_point, analysis_type, output_trans_type), analysis_type)) {
    updateClockArrival(sink_point, analysis_type, output_trans_type, candidate_arrival);
    updateClockPredecessor(sink_point, analysis_type, output_trans_type, input_trans_type, arc, arc_delay);
    sink_point.get_clock_slew_map()[analysis_type][output_trans_type] = calcArcSlew(database, arc, analysis_type, input_trans_type, output_trans_type,
                                                                                    source_slew);
  }
}

bool TimingPropagator::hasClockArrival(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type)
{
  return timing_point.get_clock_arrival_map().count(analysis_type) > 0
         && timing_point.get_clock_arrival_map()[analysis_type].count(trans_type) > 0;
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

void TimingPropagator::updateClockPredecessor(TimingPoint& timing_point, AnalysisType analysis_type, TransType trans_type,
                                              TransType predecessor_trans_type, Arc& arc, double arc_delay)
{
  timing_point.get_clock_predecessor_map()[analysis_type][trans_type] = arc.get_source_pin();
  timing_point.get_clock_predecessor_arc_delay_map()[analysis_type][trans_type] = arc_delay;
  timing_point.get_clock_predecessor_trans_type_map()[analysis_type][trans_type] = predecessor_trans_type;
}

bool TimingPropagator::shouldStopClockPropagation(Database& database, std::string& pin_name)
{
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_clock_pin_name();
}

double TimingPropagator::getClockArrival(Database& database, std::string& pin_name, AnalysisType analysis_type)
{
  return getClockArrival(database, pin_name, analysis_type, TransType::kRise);
}

double TimingPropagator::getClockArrival(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
{
  if (database.get_timing_point_map().count(pin_name) == 0) {
    return 0.0;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  return getClockArrival(timing_point, analysis_type, trans_type);
}

double TimingPropagator::getClockSlew(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
{
  if (database.get_timing_point_map().count(pin_name) == 0) {
    return 0.0;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[pin_name];
  if (timing_point.get_clock_slew_map().count(analysis_type) == 0 || timing_point.get_clock_slew_map()[analysis_type].count(trans_type) == 0) {
    return 0.0;
  }
  return timing_point.get_clock_slew_map()[analysis_type][trans_type];
}

void TimingPropagator::seedStartPointList(Database& database)
{
  for (std::string& start_point : database.get_start_point_list()) {
    TimingPoint& timing_point = database.get_timing_point_map()[start_point];
    timing_point.set_arrival(getStartPointArrival(database, start_point, AnalysisType::kMax));
    timing_point.set_launch_time(getStartPointLaunchTime(database, start_point, AnalysisType::kMax));
    timing_point.set_clock_name(getClockName(database, start_point));
    seedPathState(database, start_point, AnalysisType::kMax);
    seedPathState(database, start_point, AnalysisType::kMin);
  }
}

void TimingPropagator::propagateDataSlewDelay(Database& database)
{
  seedDataSlewList(database);
  for (std::string& pin_name : database.get_timing_order_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      propagateDataSlewDelayArc(database, arc_idx);
    }
  }
}

void TimingPropagator::seedDataSlewList(Database& database)
{
  for (std::string& start_point : database.get_start_point_list()) {
    seedDataSlew(database, start_point, AnalysisType::kMax);
    seedDataSlew(database, start_point, AnalysisType::kMin);
  }
}

void TimingPropagator::seedDataSlew(Database& database, std::string& start_point, AnalysisType analysis_type)
{
  seedDataSlew(database, start_point, analysis_type, TransType::kRise);
  seedDataSlew(database, start_point, analysis_type, TransType::kFall);
}

void TimingPropagator::seedDataSlew(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  database.get_timing_point_map()[start_point].get_data_slew_map()[analysis_type][trans_type]
      = getStartPointSlew(database, start_point, analysis_type, trans_type);
}

void TimingPropagator::propagateDataSlewDelayArc(Database& database, std::size_t arc_idx)
{
  propagateDataSlewDelayArc(database, arc_idx, AnalysisType::kMax, TransType::kRise);
  propagateDataSlewDelayArc(database, arc_idx, AnalysisType::kMax, TransType::kFall);
  propagateDataSlewDelayArc(database, arc_idx, AnalysisType::kMin, TransType::kRise);
  propagateDataSlewDelayArc(database, arc_idx, AnalysisType::kMin, TransType::kFall);
}

void TimingPropagator::propagateDataSlewDelayArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, TransType input_trans_type)
{
  Arc& arc = database.get_arc_list()[arc_idx];
  if (isDisableArc(arc)) {
    return;
  }
  if (shouldStopDataPropagation(database, arc)) {
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
    updateDataSlewDelay(database, arc, source_point, sink_point, analysis_type, input_trans_type, output_trans_type);
  }
}

bool TimingPropagator::isClockArcTriggerTrans(Arc& arc, TransType input_trans_type)
{
  if (!arc.get_is_clock_arc() || arc.get_timing_cell_arc() == nullptr || arc.get_timing_cell_arc()->get_lib_arc_set() == nullptr
      || arc.get_timing_cell_arc()->get_lib_arc_set()->front() == nullptr) {
    return true;
  }
  idb::LibArc* lib_arc = arc.get_timing_cell_arc()->get_lib_arc_set()->front();
  if (lib_arc->isFallingTriggerArc()) {
    return input_trans_type == TransType::kFall;
  }
  return input_trans_type == TransType::kRise;
}

void TimingPropagator::updateDataSlewDelay(Database& database, Arc& arc, TimingPoint& source_point, TimingPoint& sink_point,
                                           AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type)
{
  double input_slew = getDataSlew(source_point, analysis_type, input_trans_type);
  double arc_delay = calcArcDelay(database, arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  double output_slew = calcArcSlew(database, arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  updateGraphArcDelay(arc, analysis_type, input_trans_type, output_trans_type, arc_delay);
  updateDataSlew(sink_point, analysis_type, output_trans_type, output_slew);
}

void TimingPropagator::updateGraphArcDelay(Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                           double arc_delay)
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

double TimingPropagator::getStartPointArrival(Database& database, std::string& start_point, AnalysisType analysis_type)
{
  return getStartPointArrival(database, start_point, analysis_type, TransType::kRise);
}

double TimingPropagator::getStartPointArrival(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  if (isClockSourceStartPoint(database, start_point)) {
    return getStartPointClockEdge(database, start_point, analysis_type, trans_type);
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
    return getClockArrival(database, instance.get_clock_pin_name(), analysis_type, clock_trans_type);
  }
  if (instance.get_is_sequential() && start_point == instance.get_output_pin_name()) {
    TransType clock_trans_type = getClockTransType(instance.get_clock_to_q_arc());
    double clock_slew = getClockSlew(database, instance.get_clock_pin_name(), analysis_type, clock_trans_type);
    double clock_to_q_delay
        = calcTimingCellArcDelay(database, start_point, instance.get_clock_to_q_arc(), analysis_type, clock_trans_type, trans_type, clock_slew);
    return getClockArrival(database, instance.get_clock_pin_name(), analysis_type, clock_trans_type) + clock_to_q_delay;
  }
  return 0.0;
}

bool TimingPropagator::isClockSourceStartPoint(Database& database, std::string& start_point)
{
  Pin& pin = database.get_pin_map()[start_point];
  return pin.get_is_port() && getStartPointClock(database, start_point) != nullptr;
}

TimingClock* TimingPropagator::getStartPointClock(Database& database, std::string& start_point)
{
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    for (std::string& clock_source : clock_pair.second.get_source_list()) {
      if (clock_source == start_point) {
        return &clock_pair.second;
      }
    }
  }
  return nullptr;
}

double TimingPropagator::getStartPointClockEdge(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  TimingClock* timing_clock = getStartPointClock(database, start_point);
  if (timing_clock == nullptr) {
    return 0.0;
  }
  if (analysis_type == AnalysisType::kMax && trans_type == TransType::kFall) {
    return timing_clock->get_fall_edge();
  }
  return timing_clock->get_rise_edge();
}

double TimingPropagator::getStartPointSlew(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
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
    return getClockSlew(database, instance.get_clock_pin_name(), analysis_type, clock_trans_type);
  }
  if (instance.get_is_sequential() && start_point == instance.get_output_pin_name()) {
    TransType clock_trans_type = getClockTransType(instance.get_clock_to_q_arc());
    double clock_slew = getClockSlew(database, instance.get_clock_pin_name(), analysis_type, clock_trans_type);
    return calcTimingCellArcSlew(database, start_point, instance.get_clock_to_q_arc(), analysis_type, clock_trans_type, trans_type, clock_slew);
  }
  return 0.0;
}

double TimingPropagator::calcTimingCellArcDelay(Database& database, std::string& output_pin, TimingCellArc& timing_cell_arc,
                                                AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                                double input_slew)
{
  return calcTimingCellArcDelay(database, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew,
                                getOutputPinLoad(database, output_pin, analysis_type, output_trans_type));
}

double TimingPropagator::calcTimingCellArcSlew(Database& database, std::string& output_pin, TimingCellArc& timing_cell_arc,
                                               AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                               double input_slew)
{
  return calcTimingCellArcSlew(database, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew,
                               getOutputPinLoad(database, output_pin, analysis_type, output_trans_type));
}

double TimingPropagator::getStartPointLaunchTime(Database& database, std::string& start_point, AnalysisType analysis_type)
{
  return getStartPointLaunchTime(database, start_point, analysis_type, TransType::kRise);
}

double TimingPropagator::getStartPointLaunchTime(Database& database, std::string& start_point, AnalysisType analysis_type, TransType trans_type)
{
  if (isClockSourceStartPoint(database, start_point)) {
    return getStartPointClockEdge(database, start_point, analysis_type, trans_type);
  }
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || (start_point != instance.get_output_pin_name() && start_point != instance.get_clock_pin_name())) {
    return 0.0;
  }
  return getClockArrival(database, instance.get_clock_pin_name(), analysis_type, getClockTransType(instance.get_clock_to_q_arc()));
}

TransType TimingPropagator::getClockTransType(TimingCellArc& timing_cell_arc)
{
  idb::LibArcSet* lib_arc_set = timing_cell_arc.get_lib_arc_set();
  if (lib_arc_set == nullptr) {
    return TransType::kRise;
  }
  idb::LibArc* lib_arc = lib_arc_set->front();
  if (lib_arc->isFallingTriggerArc()) {
    return TransType::kFall;
  }
  return TransType::kRise;
}

std::string TimingPropagator::getClockName(Database& database, std::string& pin_name)
{
  TimingClock* timing_clock = getStartPointClock(database, pin_name);
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

std::string TimingPropagator::getPathStateStartPoint(Database& database, std::string& start_point)
{
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

void TimingPropagator::seedPathState(Database& database, std::string& start_point, AnalysisType analysis_type)
{
  PathSourceType source_type = getStartPointSourceType(database, start_point, analysis_type);
  if (source_type == PathSourceType::kNone) {
    return;
  }
  std::string path_state_start_point = getPathStateStartPoint(database, start_point);
  TimingPathState& rise_path_state
      = database.get_timing_point_map()[start_point].get_path_state_map()[analysis_type][source_type][TransType::kRise][path_state_start_point];
  rise_path_state.set_arrival(getStartPointArrival(database, start_point, analysis_type, TransType::kRise));
  rise_path_state.set_slew(getStartPointSlew(database, start_point, analysis_type, TransType::kRise));
  rise_path_state.set_launch_time(getStartPointLaunchTime(database, start_point, analysis_type, TransType::kRise));
  rise_path_state.set_start_point(path_state_start_point);
  rise_path_state.set_clock_name(getClockName(database, start_point));
  rise_path_state.get_predecessor().clear();
  rise_path_state.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
  rise_path_state.set_predecessor_arc_delay(0.0);
  rise_path_state.set_trans_type(TransType::kRise);
  rise_path_state.set_predecessor_trans_type(TransType::kNone);
  TimingPathState& fall_path_state
      = database.get_timing_point_map()[start_point].get_path_state_map()[analysis_type][source_type][TransType::kFall][path_state_start_point];
  fall_path_state.set_arrival(getStartPointArrival(database, start_point, analysis_type, TransType::kFall));
  fall_path_state.set_slew(getStartPointSlew(database, start_point, analysis_type, TransType::kFall));
  fall_path_state.set_launch_time(getStartPointLaunchTime(database, start_point, analysis_type, TransType::kFall));
  fall_path_state.set_start_point(path_state_start_point);
  fall_path_state.set_clock_name(getClockName(database, start_point));
  fall_path_state.get_predecessor().clear();
  fall_path_state.set_predecessor_arc_idx(std::numeric_limits<std::size_t>::max());
  fall_path_state.set_predecessor_arc_delay(0.0);
  fall_path_state.set_trans_type(TransType::kFall);
  fall_path_state.set_predecessor_trans_type(TransType::kNone);
}

PathSourceType TimingPropagator::getStartPointSourceType(Database& database, std::string& start_point, AnalysisType analysis_type)
{
  if (isClockSourceStartPoint(database, start_point)) {
    return PathSourceType::kInput;
  }
  if (isInputStartPoint(database, start_point) && hasInputDelay(database, start_point, analysis_type)) {
    return PathSourceType::kInput;
  }
  if (isRegisterStartPoint(database, start_point)) {
    return PathSourceType::kRegister;
  }
  return PathSourceType::kNone;
}

bool TimingPropagator::hasInputDelay(Database& database, std::string& start_point, AnalysisType analysis_type)
{
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

bool TimingPropagator::isInputStartPoint(Database& database, std::string& start_point)
{
  Pin& pin = database.get_pin_map()[start_point];
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout);
}

bool TimingPropagator::isRegisterStartPoint(Database& database, std::string& start_point)
{
  Pin& pin = database.get_pin_map()[start_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && (start_point == instance.get_output_pin_name() || start_point == instance.get_clock_pin_name())
         && hasClockPoint(database, instance.get_clock_pin_name());
}

bool TimingPropagator::hasClockPoint(Database& database, std::string& pin_name)
{
  return database.get_timing_point_map().count(pin_name) > 0 && database.get_timing_point_map()[pin_name].get_is_clock_point();
}

void TimingPropagator::propagateArrivalArc(Database& database, std::size_t arc_idx)
{
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
  propagatePathStateArc(database, arc_idx, AnalysisType::kMax, PathSourceType::kInput);
  propagatePathStateArc(database, arc_idx, AnalysisType::kMax, PathSourceType::kRegister);
  propagatePathStateArc(database, arc_idx, AnalysisType::kMin, PathSourceType::kInput);
  propagatePathStateArc(database, arc_idx, AnalysisType::kMin, PathSourceType::kRegister);
}

void TimingPropagator::propagatePathStateArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type)
{
  propagatePathStateArc(database, arc_idx, analysis_type, source_type, TransType::kRise);
  propagatePathStateArc(database, arc_idx, analysis_type, source_type, TransType::kFall);
}

void TimingPropagator::propagatePathStateArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type,
                                             TransType input_trans_type)
{
  if (isDisableArc(database.get_arc_list()[arc_idx])) {
    return;
  }
  if (!isClockArcTriggerTrans(database.get_arc_list()[arc_idx], input_trans_type)) {
    return;
  }
  for (TransType output_trans_type : getOutputTransTypeList(database.get_arc_list()[arc_idx], analysis_type, input_trans_type)) {
    propagatePathStateArc(database, arc_idx, analysis_type, source_type, input_trans_type, output_trans_type);
  }
}

void TimingPropagator::propagatePathStateArc(Database& database, std::size_t arc_idx, AnalysisType analysis_type, PathSourceType source_type,
                                             TransType input_trans_type, TransType output_trans_type)
{
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
      sink_path_state.set_trans_type(output_trans_type);
      sink_path_state.set_predecessor_trans_type(input_trans_type);
    }
  }
}

std::vector<TransType> TimingPropagator::getOutputTransTypeList(Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
{
  std::vector<TransType> output_trans_type_list;
  if (arc.get_input_output_delay_map().count(analysis_type) == 0
      || arc.get_input_output_delay_map()[analysis_type].count(input_trans_type) == 0) {
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

double TimingPropagator::calcArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                      double input_slew)
{
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(database, arc);
  }
  TimingCellArc* timing_cell_arc = getTimingCellArc(database, arc);
  if (timing_cell_arc != nullptr) {
    return calcTimingCellArcDelay(database, arc, *timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew);
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

double TimingPropagator::calcArcSlew(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
                                     double input_slew)
{
  if (arc.get_type() == ArcType::kNet) {
    return input_slew;
  }
  TimingCellArc* timing_cell_arc = getTimingCellArc(database, arc);
  if (timing_cell_arc != nullptr) {
    return calcTimingCellArcSlew(database, arc, *timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew);
  }
  return input_slew;
}

bool TimingPropagator::hasPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type)
{
  return hasPathState(timing_point, analysis_type, source_type, TransType::kRise)
         || hasPathState(timing_point, analysis_type, source_type, TransType::kFall);
}

bool TimingPropagator::hasPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type, TransType trans_type)
{
  return timing_point.get_path_state_map().count(analysis_type) > 0 && timing_point.get_path_state_map()[analysis_type].count(source_type) > 0
         && timing_point.get_path_state_map()[analysis_type][source_type].count(trans_type) > 0
         && !timing_point.get_path_state_map()[analysis_type][source_type][trans_type].empty();
}

std::map<std::string, TimingPathState>& TimingPropagator::getPathStateMap(TimingPoint& timing_point, AnalysisType analysis_type,
                                                                          PathSourceType source_type, TransType trans_type)
{
  return timing_point.get_path_state_map()[analysis_type][source_type][trans_type];
}

TimingPathState& TimingPropagator::getPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type,
                                                TransType trans_type, std::string& start_point)
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

TimingPathState* TimingPropagator::getWorstPathState(TimingPoint& timing_point, AnalysisType analysis_type, PathSourceType source_type,
                                                     TransType trans_type)
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
        || isBetterArrival(path_state->get_arrival(),
                           getWorstPathState(timing_point, analysis_type, source_type, best_trans_type)->get_arrival(), analysis_type)) {
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

void TimingPropagator::propagateRequired(Database& database)
{
  const double required_time = resolveRequiredTime(database);

  seedEndPointRequired(database, required_time);

  for (auto iter = database.get_timing_order_list().rbegin(); iter != database.get_timing_order_list().rend(); ++iter) {
    std::string& pin_name = *iter;
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      if (isDisableArc(database.get_arc_list()[arc_idx])) {
        continue;
      }
      if (shouldStopDataPropagation(database, database.get_arc_list()[arc_idx])) {
        continue;
      }
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
    database.get_timing_point_map()[end_point].set_required(getEndPointRequired(database, end_point, required_time, AnalysisType::kMax));
  }
}

double TimingPropagator::getEndPointRequired(Database& database, std::string& end_point, double default_required_time, AnalysisType analysis_type)
{
  TimingPathState* end_path_state = getWorstPathState(database.get_timing_point_map()[end_point], analysis_type, PathSourceType::kInput);
  if (end_path_state == nullptr) {
    end_path_state = getWorstPathState(database.get_timing_point_map()[end_point], analysis_type, PathSourceType::kRegister);
  }
  if (end_path_state == nullptr) {
    return getEndPointRequired(database, end_point, default_required_time, analysis_type, TransType::kRise, 0.0);
  }
  return getEndPointRequired(database, end_point, default_required_time, analysis_type, end_path_state->get_trans_type(), end_path_state->get_slew());
}

double TimingPropagator::getEndPointRequired(Database& database, std::string& end_point, double default_required_time, AnalysisType analysis_type,
                                             TransType data_trans_type, double data_slew)
{
  return getEndPointRequired(database, end_point, end_point, default_required_time, analysis_type, data_trans_type, data_slew);
}

double TimingPropagator::getEndPointRequired(Database& database, std::string& start_point, std::string& end_point, double default_required_time,
                                             AnalysisType analysis_type, TransType data_trans_type, double data_slew)
{
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port()) {
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    if (analysis_type == AnalysisType::kMin && port_constraint_map.count(end_point) > 0 && port_constraint_map[end_point].get_has_output_delay_min()) {
      return port_constraint_map[end_point].get_output_delay_min();
    }
    if (port_constraint_map.count(end_point) > 0 && port_constraint_map[end_point].get_has_output_delay_max()) {
      return getEndPointCaptureTime(database, end_point, analysis_type) - port_constraint_map[end_point].get_output_delay_max();
    }
    return default_required_time;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return default_required_time;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(database, end_point, analysis_type);
  if (instance.get_is_sequential() && timing_check_arc != nullptr && isMatchCheckTransType(*timing_check_arc, data_trans_type)) {
    std::string clock_name = getClockName(database, end_point);
    double check_time = getEndPointCheckTime(database, end_point, *timing_check_arc, analysis_type, data_trans_type, data_slew);
    std::string common_pin_name;
    double cppr = getClockReconvergencePessimism(database, start_point, end_point, analysis_type, common_pin_name);
    if (analysis_type == AnalysisType::kMin) {
      return roundTime(getEndPointCaptureTime(database, end_point, analysis_type) + check_time - cppr);
    }
    return roundTime(getEndPointCaptureTime(database, end_point, analysis_type) - check_time + cppr);
  }
  return default_required_time;
}

bool TimingPropagator::isMatchCheckTransType(TimingCheckArc& timing_check_arc, TransType data_trans_type)
{
  idb::LibArc* lib_arc = timing_check_arc.get_lib_arc();
  if (lib_arc == nullptr && timing_check_arc.get_lib_arc_set() != nullptr) {
    lib_arc = timing_check_arc.get_lib_arc_set()->front();
  }
  if (lib_arc == nullptr || lib_arc->get_table_model() == nullptr) {
    return true;
  }
  if (data_trans_type == TransType::kFall) {
    return lib_arc->get_table_model()->getTable(CAST_TYPE_TO_INDEX(idb::LibTable::TableType::kFallConstrain)) != nullptr;
  }
  return lib_arc->get_table_model()->getTable(CAST_TYPE_TO_INDEX(idb::LibTable::TableType::kRiseConstrain)) != nullptr;
}

double TimingPropagator::getEndPointCheckTime(Database& database, std::string& end_point, TimingCheckArc& timing_check_arc, AnalysisType analysis_type,
                                              TransType data_trans_type, double data_slew)
{
  Pin& pin = database.get_pin_map()[end_point];
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  TransType clock_trans_type = getClockTransType(timing_check_arc);
  AnalysisType capture_analysis_type = getCaptureAnalysisType(analysis_type);
  double clock_slew = getClockSlew(database, instance.get_clock_pin_name(), capture_analysis_type, clock_trans_type);
  return calcTimingCheckArcTime(timing_check_arc, analysis_type, clock_trans_type, data_trans_type, clock_slew, data_slew);
}

double TimingPropagator::calcTimingCheckArcTime(TimingCheckArc& timing_check_arc, AnalysisType analysis_type, TransType clock_trans_type,
                                                TransType data_trans_type, double clock_slew, double data_slew)
{
  idb::LibArcSet* lib_arc_set = timing_check_arc.get_lib_arc_set();
  if (lib_arc_set != nullptr) {
    std::vector<double> delay_list = lib_arc_set->getDelayOrConstrainCheckNs(
        getIDBTransType(clock_trans_type), getIDBTransType(data_trans_type), clock_slew, convertCheckSlewForLookup(timing_check_arc, data_slew));
    if (analysis_type == AnalysisType::kMin) {
      return delay_list.back();
    }
    return delay_list.front();
  }
  idb::LibArc* lib_arc = timing_check_arc.get_lib_arc();
  if (lib_arc != nullptr) {
    return lib_arc->getDelayOrConstrainCheckNs(getIDBTransType(data_trans_type), clock_slew, convertCheckSlewForLookup(timing_check_arc, data_slew));
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
  idb::LibArc* lib_arc = timing_check_arc.get_lib_arc();
  if (lib_arc == nullptr && timing_check_arc.get_lib_arc_set() != nullptr) {
    lib_arc = timing_check_arc.get_lib_arc_set()->front();
  }
  if (lib_arc == nullptr) {
    return TransType::kRise;
  }
  if (lib_arc->isFallingEdgeCheck()) {
    return TransType::kFall;
  }
  return TransType::kRise;
}

double TimingPropagator::getEndPointCaptureTime(Database& database, std::string& end_point, AnalysisType analysis_type)
{
  AnalysisType capture_analysis_type = getCaptureAnalysisType(analysis_type);
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(database, end_point, analysis_type);
  TransType clock_trans_type = timing_check_arc == nullptr ? TransType::kRise : getClockTransType(*timing_check_arc);
  if (analysis_type == AnalysisType::kMax) {
    std::string clock_name = getClockName(database, end_point);
    return getClockPeriod(database, clock_name) + getEndPointClockArrival(database, end_point, capture_analysis_type, clock_trans_type);
  }
  return getEndPointClockArrival(database, end_point, capture_analysis_type, clock_trans_type);
}

double TimingPropagator::getEndPointClockArrival(Database& database, std::string& end_point, AnalysisType analysis_type)
{
  return getEndPointClockArrival(database, end_point, analysis_type, TransType::kRise);
}

double TimingPropagator::getEndPointClockArrival(Database& database, std::string& end_point, AnalysisType analysis_type, TransType trans_type)
{
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return 0.0;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential()) {
    return 0.0;
  }
  return getClockArrival(database, instance.get_clock_pin_name(), analysis_type, trans_type);
}

double TimingPropagator::getClockReconvergencePessimism(Database& database, std::string& start_point, std::string& end_point,
                                                        AnalysisType analysis_type, std::string& common_pin_name)
{
  if (!isRegisterStartPoint(database, start_point) || !isRegisterEndPoint(database, end_point)) {
    return 0.0;
  }
  Pin& start_pin = database.get_pin_map()[start_point];
  Pin& end_pin = database.get_pin_map()[end_point];
  Instance& start_instance = database.get_instance_map()[start_pin.get_instance_name()];
  Instance& end_instance = database.get_instance_map()[end_pin.get_instance_name()];
  AnalysisType launch_analysis_type = analysis_type;
  AnalysisType capture_analysis_type = getCaptureAnalysisType(analysis_type);
  TransType launch_trans_type = getClockTransType(start_instance.get_clock_to_q_arc());
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(database, end_point, analysis_type);
  TransType capture_trans_type = timing_check_arc == nullptr ? TransType::kRise : getClockTransType(*timing_check_arc);
  std::vector<std::pair<std::string, TransType>> launch_clock_path
      = getClockPathPinList(database, start_instance.get_clock_pin_name(), launch_analysis_type, launch_trans_type);
  std::vector<std::pair<std::string, TransType>> capture_clock_path
      = getClockPathPinList(database, end_instance.get_clock_pin_name(), capture_analysis_type, capture_trans_type);
  shrinkClockPathToCrprPath(database, launch_clock_path);
  shrinkClockPathToCrprPath(database, capture_clock_path);
  std::pair<std::string, TransType> launch_common_pin;
  std::pair<std::string, TransType> capture_common_pin;
  std::size_t path_size = std::min(launch_clock_path.size(), capture_clock_path.size());
  for (std::size_t i = 0; i < path_size; i++) {
    if (launch_clock_path[i].first != capture_clock_path[i].first) {
      break;
    }
    launch_common_pin = launch_clock_path[i];
    capture_common_pin = capture_clock_path[i];
    common_pin_name = launch_clock_path[i].first;
  }
  if (common_pin_name.empty()) {
    return 0.0;
  }
  double launch_common_arrival = getClockCommonPathArrival(database, launch_common_pin, launch_analysis_type);
  double capture_common_arrival = getClockCommonPathArrival(database, capture_common_pin, capture_analysis_type);
  return std::fabs(launch_common_arrival - capture_common_arrival);
}

void TimingPropagator::shrinkClockPathToCrprPath(Database& database, std::vector<std::pair<std::string, TransType>>& clock_path)
{
  if (!clock_path.empty()) {
    clock_path.pop_back();
  }
  while (clock_path.size() >= 2 && isLeafClockDriverPin(database, clock_path.back().first)) {
    clock_path.pop_back();
    clock_path.pop_back();
  }
}

bool TimingPropagator::isLeafClockDriverPin(Database& database, std::string& pin_name)
{
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

std::vector<std::pair<std::string, TransType>> TimingPropagator::getClockPathPinList(Database& database, std::string& clock_pin_name,
                                                                                    AnalysisType analysis_type, TransType trans_type)
{
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

double TimingPropagator::getClockCommonPathArrival(Database& database, std::pair<std::string, TransType>& common_pin, AnalysisType analysis_type)
{
  if (database.get_timing_point_map().count(common_pin.first) == 0) {
    return 0.0;
  }
  TimingPoint& timing_point = database.get_timing_point_map()[common_pin.first];
  return getClockArrival(timing_point, analysis_type, common_pin.second);
}

TimingCheckArc* TimingPropagator::getEndPointCheckArc(Database& database, std::string& end_point, AnalysisType analysis_type)
{
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

double TimingPropagator::getClockPeriod(Database& database, std::string& clock_name)
{
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
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
  if (isDisableArc(arc)) {
    return;
  }
  if (shouldStopDataPropagation(database, arc)) {
    return;
  }
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  if (isFinite(sink_point.get_required())) {
    source_point.set_required(std::min(source_point.get_required(), sink_point.get_required() - arc.get_delay()));
  }
}

void TimingPropagator::updateSlack(Database& database)
{
  for (std::pair<const std::string, TimingPoint>& timing_pair : database.get_timing_point_map()) {
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
    std::vector<TimingPath> timing_path_list = buildTimingPathList(database, end_point);
    if (timing_path_list.empty()) {
      ++unconstrained_end_point_num;
      continue;
    }
    ++checked_end_point_num;
    for (TimingPath& timing_path : timing_path_list) {
      insertTimingPath(timing_path_group, timing_path);
      updateWorstSlack(timing_path, worst_slack, worst_end_point);
      updateViolation(timing_path, violation_num, total_negative_slack);
    }
  }

  if (!std::isfinite(worst_slack)) {
    worst_slack = 0.0;
  }
  updateSummary(database, timing_path_group, checked_end_point_num, unconstrained_end_point_num, violation_num, worst_slack, total_negative_slack,
                worst_end_point);
  database.get_timing_path_group_list().push_back(timing_path_group);

  STALOG.info(Loc::current(), "Analyze iSTA timing: timing_paths=", getTimingPathNum(timing_path_group), " checked_end_points=", checked_end_point_num,
              " unconstrained_end_points=", unconstrained_end_point_num, " violating_end_points=", violation_num, " worst_slack=", worst_slack,
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

std::vector<TimingPath> TimingPropagator::buildTimingPathList(Database& database, std::string& end_point)
{
  std::vector<TimingPath> timing_path_list;
  if (!isConstrainedEndPoint(database, end_point)) {
    return timing_path_list;
  }
  buildPathDiversionList(database, end_point);
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMax, PathSourceType::kInput)) {
    TimingPathState* path_state = getWorstSlackPathState(database, end_point, AnalysisType::kMax, PathSourceType::kInput);
    if (path_state != nullptr) {
      timing_path_list.push_back(
          buildTimingPath(database, end_point, AnalysisType::kMax, PathSourceType::kInput, path_state->get_trans_type(), path_state->get_start_point()));
    }
  }
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMax, PathSourceType::kRegister)) {
    TimingPathState* path_state = getWorstSlackPathState(database, end_point, AnalysisType::kMax, PathSourceType::kRegister);
    if (path_state != nullptr) {
      timing_path_list.push_back(buildTimingPath(database, end_point, AnalysisType::kMax, PathSourceType::kRegister, path_state->get_trans_type(),
                                                path_state->get_start_point()));
    }
  }
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMin, PathSourceType::kInput)) {
    TimingPathState* path_state = getWorstSlackPathState(database, end_point, AnalysisType::kMin, PathSourceType::kInput);
    if (path_state != nullptr) {
      timing_path_list.push_back(
          buildTimingPath(database, end_point, AnalysisType::kMin, PathSourceType::kInput, path_state->get_trans_type(), path_state->get_start_point()));
    }
  }
  if (hasPathState(database.get_timing_point_map()[end_point], AnalysisType::kMin, PathSourceType::kRegister)) {
    TimingPathState* path_state = getWorstSlackPathState(database, end_point, AnalysisType::kMin, PathSourceType::kRegister);
    if (path_state != nullptr) {
      timing_path_list.push_back(buildTimingPath(database, end_point, AnalysisType::kMin, PathSourceType::kRegister, path_state->get_trans_type(),
                                                path_state->get_start_point()));
    }
  }
  return timing_path_list;
}

void TimingPropagator::buildPathDiversionList(Database& database, std::string& end_point)
{
  buildPathDiversionList(database, end_point, AnalysisType::kMax, PathSourceType::kInput);
  buildPathDiversionList(database, end_point, AnalysisType::kMax, PathSourceType::kRegister);
  buildPathDiversionList(database, end_point, AnalysisType::kMin, PathSourceType::kInput);
  buildPathDiversionList(database, end_point, AnalysisType::kMin, PathSourceType::kRegister);
}

void TimingPropagator::buildPathDiversionList(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type)
{
  TimingPoint& end_timing_point = database.get_timing_point_map()[end_point];
  TimingPathState* path_state = getWorstPathState(end_timing_point, analysis_type, source_type);
  if (path_state == nullptr) {
    return;
  }
  std::vector<std::string> path_pin_name_list;
  std::vector<TransType> path_trans_type_list;
  std::string& start_point = path_state->get_start_point();
  buildPathTrace(database, end_point, analysis_type, source_type, path_state->get_trans_type(), start_point, path_pin_name_list, path_trans_type_list);
  std::vector<std::size_t> path_arc_idx_list = getPathArcIdxList(database, path_pin_name_list, path_trans_type_list, analysis_type, source_type, start_point);
  buildPathDiversionList(database, end_point, analysis_type, source_type, path_pin_name_list, path_trans_type_list, path_arc_idx_list);
}

void TimingPropagator::buildPathDiversionList(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                                              std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                                              std::vector<std::size_t>& path_arc_idx_list)
{
  for (std::size_t sink_idx = 1; sink_idx < path_pin_name_list.size(); sink_idx++) {
    std::string& sink_pin = path_pin_name_list[sink_idx];
    TransType sink_trans_type = path_trans_type_list[sink_idx];
    std::size_t path_arc_idx = path_arc_idx_list[sink_idx - 1];
    for (std::size_t diversion_arc_idx : database.get_incoming_arc_list_map()[sink_pin]) {
      if (diversion_arc_idx == path_arc_idx || isDisableArc(database.get_arc_list()[diversion_arc_idx])
          || shouldStopDataPropagation(database, database.get_arc_list()[diversion_arc_idx])) {
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
          buildPathDiversionState(database, analysis_type, source_type, path_pin_name_list, path_trans_type_list, path_arc_idx_list, sink_idx,
                                  diversion_arc_idx, input_trans_type, source_path_state);
        }
      }
    }
  }
}

void TimingPropagator::buildPathDiversionState(Database& database, AnalysisType analysis_type, PathSourceType source_type,
                                               std::vector<std::string>& path_pin_name_list, std::vector<TransType>& path_trans_type_list,
                                               std::vector<std::size_t>& path_arc_idx_list, std::size_t sink_idx, std::size_t diversion_arc_idx,
                                               TransType input_trans_type, TimingPathState& source_path_state)
{
  Arc& diversion_arc = database.get_arc_list()[diversion_arc_idx];
  TransType sink_trans_type = path_trans_type_list[sink_idx];
  double diversion_arc_delay = getArcDelay(diversion_arc, analysis_type, input_trans_type, sink_trans_type);
  double arrival = roundTime(source_path_state.get_arrival() + diversion_arc_delay);
  std::string predecessor = diversion_arc.get_source_pin();
  if (!updateDiversionPathState(database, path_pin_name_list[sink_idx], analysis_type, source_type, sink_trans_type, source_path_state, predecessor,
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
    if (!updateDiversionPathState(database, path_pin_name_list[path_idx], analysis_type, source_type, trans_type, source_path_state, path_predecessor,
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

bool TimingPropagator::updateDiversionPathState(Database& database, std::string& pin_name, AnalysisType analysis_type, PathSourceType source_type,
                                                TransType trans_type, TimingPathState& source_path_state, std::string& predecessor,
                                                std::size_t predecessor_arc_idx, double predecessor_arc_delay, TransType predecessor_trans_type,
                                                double arrival)
{
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
  path_state.set_trans_type(trans_type);
  path_state.set_predecessor_trans_type(predecessor_trans_type);
  return true;
}

TimingPathState* TimingPropagator::getWorstSlackPathState(Database& database, std::string& end_point, AnalysisType analysis_type,
                                                          PathSourceType source_type)
{
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
      TimingCheckArc* timing_check_arc = getEndPointCheckArc(database, end_point, analysis_type);
      if (timing_check_arc != nullptr && !isMatchCheckTransType(*timing_check_arc, path_state.get_trans_type())) {
        continue;
      }
      double required_time = calcPathRequiredTime(database, end_point, path_state, analysis_type);
      double slack = calcPathSlack(path_state, required_time, analysis_type);
      if (worst_path_state == nullptr || slack < worst_slack - STA_ERROR) {
        worst_slack = slack;
        worst_path_state = &path_state;
      }
    }
  }
  return worst_path_state;
}

double TimingPropagator::calcPathRequiredTime(Database& database, std::string& end_point, TimingPathState& end_path_state, AnalysisType analysis_type)
{
  return getEndPointRequired(database, end_path_state.get_start_point(), end_point, end_path_state.get_arrival(), analysis_type,
                             end_path_state.get_trans_type(), end_path_state.get_slew());
}

double TimingPropagator::calcPathSlack(TimingPathState& end_path_state, double required_time, AnalysisType analysis_type)
{
  if (analysis_type == AnalysisType::kMin) {
    return roundTime(end_path_state.get_arrival() - required_time);
  }
  return roundTime(required_time - end_path_state.get_arrival());
}

bool TimingPropagator::isConstrainedEndPoint(Database& database, std::string& end_point)
{
  return isOutputEndPoint(database, end_point) || isRegisterEndPoint(database, end_point) || isTimingCheckEndPoint(database, end_point);
}

bool TimingPropagator::isOutputEndPoint(Database& database, std::string& end_point)
{
  Pin& pin = database.get_pin_map()[end_point];
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kOutput || pin.get_direction() == PinDirection::kInout)
         && hasOutputDelay(database, end_point);
}

bool TimingPropagator::hasOutputDelay(Database& database, std::string& end_point)
{
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  if (port_constraint_map.count(end_point) == 0) {
    return false;
  }
  TimingPortConstraint& port_constraint = port_constraint_map[end_point];
  return port_constraint.get_has_output_delay_max() || port_constraint.get_has_output_delay_min();
}

bool TimingPropagator::isRegisterEndPoint(Database& database, std::string& end_point)
{
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && hasClockPoint(database, instance.get_clock_pin_name()) && end_point == instance.get_data_pin_name();
}

bool TimingPropagator::isTimingCheckEndPoint(Database& database, std::string& end_point)
{
  Pin& pin = database.get_pin_map()[end_point];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || !hasClockPoint(database, instance.get_clock_pin_name())) {
    return false;
  }
  for (TimingCheckArc& timing_check_arc : instance.get_check_arc_list()) {
    if (timing_check_arc.get_data_port() == end_point) {
      return true;
    }
  }
  return false;
}

TimingPath TimingPropagator::buildTimingPath(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                                             TransType trans_type, std::string& start_point)
{
  std::vector<std::string> path_pin_name_list;
  std::vector<TransType> path_trans_type_list;
  buildPathTrace(database, end_point, analysis_type, source_type, trans_type, start_point, path_pin_name_list, path_trans_type_list);
  std::vector<std::size_t> path_arc_idx_list = getPathArcIdxList(database, path_pin_name_list, path_trans_type_list, analysis_type, source_type, start_point);
  TimingPathState& end_path_state = getPathState(database.get_timing_point_map()[end_point], analysis_type, source_type, trans_type, start_point);
  double required_time = getEndPointRequired(database, start_point, end_point, end_path_state.get_arrival(), analysis_type, trans_type,
                                             end_path_state.get_slew());
  double slack = analysis_type == AnalysisType::kMin ? roundTime(end_path_state.get_arrival() - required_time)
                                                     : roundTime(required_time - end_path_state.get_arrival());
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
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(database, end_point, analysis_type);
  if (timing_check_arc != nullptr) {
    timing_path.set_check_type(timing_check_arc->get_check_type());
    timing_path.set_check_time(getEndPointCheckTime(database, end_point, *timing_check_arc, analysis_type, trans_type, end_path_state.get_slew()));
  }
  updateClockInfo(database, timing_path, analysis_type, source_type, trans_type, start_point);

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
    timing_path.get_point_list().push_back(makeTimingPathPoint(database, path_pin_name_list[i], arc, analysis_type, source_type,
                                                               i > 0 ? path_trans_type_list[i - 1] : TransType::kNone, path_trans_type_list[i],
                                                               start_point));
    if (i > 0) {
      timing_path.get_point_list().back().set_arc_delay(arc_delay);
    }
  }
  return timing_path;
}

void TimingPropagator::buildPathTrace(Database& database, std::string& end_point, AnalysisType analysis_type, PathSourceType source_type,
                                      TransType trans_type, std::string& start_point, std::vector<std::string>& path_pin_name_list,
                                      std::vector<TransType>& path_trans_type_list)
{
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

std::vector<std::size_t> TimingPropagator::getPathArcIdxList(Database& database, std::vector<std::string>& path_pin_name_list,
                                                             std::vector<TransType>& path_trans_type_list, AnalysisType analysis_type,
                                                             PathSourceType source_type, std::string& start_point)
{
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

void TimingPropagator::updateClockInfo(Database& database, TimingPath& timing_path, AnalysisType analysis_type, PathSourceType source_type,
                                       TransType trans_type, std::string& start_point)
{
  TimingPathState& end_path_state
      = getPathState(database.get_timing_point_map()[timing_path.get_end_point()], analysis_type, source_type, trans_type, start_point);
  timing_path.set_launch_time(end_path_state.get_launch_time());
  timing_path.set_clock_name(end_path_state.get_clock_name());
  std::string common_pin_name;
  double cppr = getClockReconvergencePessimism(database, start_point, timing_path.get_end_point(), analysis_type, common_pin_name);
  if (analysis_type == AnalysisType::kMin) {
    cppr = -cppr;
  }
  timing_path.set_last_common_pin(common_pin_name);
  timing_path.set_capture_time(getEndPointCaptureTime(database, timing_path.get_end_point(), analysis_type) + cppr);
  timing_path.set_launch_clock_network_delay(end_path_state.get_launch_time());
  TimingCheckArc* timing_check_arc = getEndPointCheckArc(database, timing_path.get_end_point(), analysis_type);
  TransType capture_trans_type = timing_check_arc == nullptr ? TransType::kRise : getClockTransType(*timing_check_arc);
  timing_path.set_capture_clock_network_delay(
      getEndPointClockArrival(database, timing_path.get_end_point(), getCaptureAnalysisType(analysis_type), capture_trans_type));
  timing_path.set_clock_reconvergence_pessimism(cppr);

  Pin& end_pin = database.get_pin_map()[timing_path.get_end_point()];
  if (end_pin.get_is_port() || database.get_instance_map().count(end_pin.get_instance_name()) == 0) {
    return;
  }
  Instance& instance = database.get_instance_map()[end_pin.get_instance_name()];
  if (instance.get_is_sequential() && isTimingCheckEndPoint(database, timing_path.get_end_point())) {
    timing_path.set_capture_clock_pin(instance.get_clock_pin_name());
    timing_path.set_setup_time(timing_path.get_check_time());
  }
}

TimingPathPoint TimingPropagator::makeTimingPathPoint(Database& database, std::string& pin_name, Arc* arc, AnalysisType analysis_type,
                                                      PathSourceType source_type, TransType input_trans_type, TransType trans_type,
                                                      std::string& start_point)
{
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

std::size_t TimingPropagator::getTimingPathNum(TimingPathGroup& timing_path_group)
{
  std::size_t timing_path_num = 0;
  for (auto& [end_point, timing_path_end] : timing_path_group.get_timing_path_end_map()) {
    timing_path_num += timing_path_end.get_timing_path_list().size();
  }
  return timing_path_num;
}

void TimingPropagator::updateSummary(Database& database, TimingPathGroup& timing_path_group, std::size_t checked_end_point_num,
                                     std::size_t unconstrained_end_point_num, std::size_t violation_num, double worst_slack, double total_negative_slack,
                                     std::string& worst_end_point)
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
