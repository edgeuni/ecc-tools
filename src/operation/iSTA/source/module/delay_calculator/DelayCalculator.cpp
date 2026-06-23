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
    return calcCellArcDelay();
  }
  if (arc.get_type() == ArcType::kNet) {
    return calcNetArcDelay(database, arc);
  }
  return 0.0;
}

double DelayCalculator::calcCellArcDelay()
{
  return 1.0;
}

double DelayCalculator::calcNetArcDelay(Database& database, Arc& arc)
{
  return 1.0 + calcManhattanDistance(database, arc.get_source_pin(), arc.get_sink_pin()) * 0.000001;
}

double DelayCalculator::calcManhattanDistance(Database& database, std::string& source_pin, std::string& sink_pin)
{
  Pin& source = database.get_pin_map()[source_pin];
  Pin& sink = database.get_pin_map()[sink_pin];
  return std::abs(source.get_x() - sink.get_x()) + std::abs(source.get_y() - sink.get_y());
}

}  // namespace ista
