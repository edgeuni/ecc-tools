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

// private

DelayCalculator* DelayCalculator::_dc_instance = nullptr;

void DelayCalculator::buildArcDelayList(Database& database)
{
  for (Arc& arc : database.get_arc_list()) {
    arc.set_delay(calcArcDelay(database, arc));
  }
}

double DelayCalculator::calcArcDelay(Database& database, Arc& arc)
{
  if (arc.get_type() == ArcType::kCell) {
    return calcCellArcDelay(database, arc);
  }
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(database, arc);
  }
  return 0.0;
}

double DelayCalculator::calcCellArcDelay(Database& database, Arc& arc)
{
  TimingCellArc* timing_cell_arc = getTimingCellArc(database, arc);
  if (timing_cell_arc != nullptr) {
    return timing_cell_arc->get_delay();
  }
  return 1.0;
}

TimingCellArc* DelayCalculator::getTimingCellArc(Database& database, Arc& arc)
{
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

double DelayCalculator::calcNetArcDelay(Database& database, Arc& arc)
{
  if (database.get_parasitic_library().get_net_map().count(arc.get_owner_name()) > 0) {
    return calcParasiticDelay(database, arc);
  }
  return 1.0 + calcManhattanDistance(database, arc.get_source_pin(), arc.get_sink_pin()) * 0.000001;
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

double DelayCalculator::calcManhattanDistance(Database& database, std::string& source_pin, std::string& sink_pin)
{
  Pin& source = database.get_pin_map()[source_pin];
  Pin& sink = database.get_pin_map()[sink_pin];
  return std::abs(source.get_x() - sink.get_x()) + std::abs(source.get_y() - sink.get_y());
}

}  // namespace ista
