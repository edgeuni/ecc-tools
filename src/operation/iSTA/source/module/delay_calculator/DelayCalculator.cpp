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
#include "DelayCalculator.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void DelayCalculator::initInst()
{
  if (_dc_instance == nullptr) {
    _dc_instance = new DelayCalculator();
  }
}

DelayCalculator& DelayCalculator::getInst()
{
  if (_dc_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_dc_instance;
}

void DelayCalculator::destroyInst()
{
  if (_dc_instance != nullptr) {
    delete _dc_instance;
    _dc_instance = nullptr;
  }
}

// function

bool DelayCalculator::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  buildArcDelayList(database);

  STALOG.info(Loc::current(), "Calculate iSTA delay: arcs=", database.get_arc_list().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

double DelayCalculator::calcArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
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

double DelayCalculator::calcArcSlew(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type,
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

double DelayCalculator::calcTimingCellArcDelay(Database& database, std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                               TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  return calcTimingCellArcDelay(database, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew,
                                getOutputPinLoad(database, output_pin, analysis_type, output_trans_type));
}

double DelayCalculator::calcTimingCellArcSlew(Database& database, std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                              TransType input_trans_type, TransType output_trans_type, double input_slew)
{
  return calcTimingCellArcSlew(database, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, input_slew,
                               getOutputPinLoad(database, output_pin, analysis_type, output_trans_type));
}

double DelayCalculator::calcTimingCheckArcTime(TimingCheckArc& timing_check_arc, AnalysisType analysis_type, TransType clock_trans_type,
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

// private

DelayCalculator* DelayCalculator::_dc_instance = nullptr;

void DelayCalculator::buildArcDelayList(Database& database)
{
  for (Arc& arc : database.get_arc_list()) {
    buildArcDelay(database, arc);
  }
}

void DelayCalculator::buildArcDelay(Database& database, Arc& arc)
{
  buildAnalysisArcDelay(database, arc, AnalysisType::kMax);
  buildAnalysisArcDelay(database, arc, AnalysisType::kMin);
  arc.set_delay_max(std::max(arc.get_trans_delay_map()[AnalysisType::kMax][TransType::kRise],
                             arc.get_trans_delay_map()[AnalysisType::kMax][TransType::kFall]));
  arc.set_delay_min(std::min(arc.get_trans_delay_map()[AnalysisType::kMin][TransType::kRise],
                             arc.get_trans_delay_map()[AnalysisType::kMin][TransType::kFall]));
  arc.set_delay(arc.get_delay_max());
}

void DelayCalculator::buildAnalysisArcDelay(Database& database, Arc& arc, AnalysisType analysis_type)
{
  buildTransArcDelay(database, arc, analysis_type, TransType::kRise);
  buildTransArcDelay(database, arc, analysis_type, TransType::kFall);
}

void DelayCalculator::buildTransArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
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

bool DelayCalculator::isClockArcTriggerTrans(TimingCellArc& timing_cell_arc, TransType input_trans_type)
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

double DelayCalculator::calcArcDelay(Database& database, Arc& arc)
{
  if (arc.get_type() == ArcType::kCell) {
    return calcCellArcDelay(database, arc, AnalysisType::kMax);
  }
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(database, arc);
  }
  return 0.0;
}

double DelayCalculator::calcCellArcDelay(Database& database, Arc& arc, AnalysisType analysis_type)
{
  return calcCellArcDelay(database, arc, analysis_type, TransType::kRise);
}

double DelayCalculator::calcCellArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type)
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

TimingCellArc* DelayCalculator::getTimingCellArc(Database& database, Arc& arc)
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

double DelayCalculator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type)
{
  return calcTimingCellArcDelay(database, arc, timing_cell_arc, analysis_type, TransType::kRise);
}

double DelayCalculator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
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

double DelayCalculator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                               TransType input_trans_type, TransType output_trans_type)
{
  return calcTimingCellArcDelay(database, arc, timing_cell_arc, analysis_type, input_trans_type, output_trans_type, 0.0);
}

double DelayCalculator::calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
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

double DelayCalculator::calcTimingCellArcSlew(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
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

double DelayCalculator::calcTimingCellArcDelay(Database& database, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
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

double DelayCalculator::calcTimingCellArcSlew(Database& database, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                              TransType output_trans_type, double input_slew, double output_load)
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

double DelayCalculator::recoverTableSlew(idb::LibArcSet* lib_arc_set, double output_slew)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  idb::LibLibrary* lib_library = lib_arc->get_owner_cell()->get_owner_lib();
  return output_slew / lib_library->get_slew_derate_from_library();
}

idb::TransType DelayCalculator::getIDBTransType(TransType trans_type)
{
  if (trans_type == TransType::kFall) {
    return idb::TransType::kFall;
  }
  return idb::TransType::kRise;
}

TransType DelayCalculator::getTransType(idb::TransType trans_type)
{
  if (trans_type == idb::TransType::kFall) {
    return TransType::kFall;
  }
  return TransType::kRise;
}

idb::TransType DelayCalculator::getOutputTransType(idb::LibArcSet* lib_arc_set, idb::TransType input_trans_type)
{
  if (lib_arc_set->isNegativeArc()) {
    return input_trans_type == idb::TransType::kRise ? idb::TransType::kFall : idb::TransType::kRise;
  }
  return input_trans_type;
}

std::vector<TransType> DelayCalculator::getOutputTransTypeList(TimingCellArc& timing_cell_arc, TransType input_trans_type)
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

double DelayCalculator::convertOutputLoad(idb::LibArcSet* lib_arc_set, double output_load)
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

double DelayCalculator::getArcOutputLoad(Database& database, Arc& arc, AnalysisType analysis_type, TransType output_trans_type)
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

double DelayCalculator::getOutputPinLoad(Database& database, std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type)
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

double DelayCalculator::getNetOutputLoad(Database& database, Net& net, AnalysisType analysis_type, TransType output_trans_type)
{
  double output_load = 0.0;
  for (std::string& load_pin_name : net.get_load_pin_list()) {
    output_load += getPinCapacitance(database, load_pin_name, analysis_type, output_trans_type);
  }
  if (net.get_driver_pin_list().size() > 1) {
    output_load /= static_cast<double>(net.get_driver_pin_list().size());
  }
  return output_load;
}

double DelayCalculator::getPinCapacitance(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type)
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

double DelayCalculator::convertCheckSlewForLookup(TimingCheckArc& timing_check_arc, double data_slew)
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

double DelayCalculator::calcNetArcDelay(Database& database, Arc& arc)
{
  if (database.get_parasitic_library().get_net_map().count(arc.get_owner_name()) > 0) {
    return calcParasiticDelay(database, arc);
  }
  return 0.0;
}

double DelayCalculator::calcParasiticDelay(Database& database, Arc& arc)
{
  ParasiticNet& parasitic_net = database.get_parasitic_library().get_net_map()[arc.get_owner_name()];
  double source_capacitance = getParasiticNodeCapacitance(parasitic_net, arc.get_source_pin());
  double sink_capacitance = getParasiticNodeCapacitance(parasitic_net, arc.get_sink_pin());
  double resistance = getParasiticTotalResistance(parasitic_net);
  return resistance * (source_capacitance + sink_capacitance) * 0.5;
}

double DelayCalculator::getParasiticNodeCapacitance(ParasiticNet& parasitic_net, std::string& pin_name)
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

double DelayCalculator::getParasiticTotalResistance(ParasiticNet& parasitic_net)
{
  double resistance = 0.0;
  for (ParasiticResistor& parasitic_resistor : parasitic_net.get_resistor_list()) {
    resistance += parasitic_resistor.get_resistance();
  }
  return resistance;
}

}  // namespace ista
