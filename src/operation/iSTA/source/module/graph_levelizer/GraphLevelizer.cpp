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
#include "GraphLevelizer.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void GraphLevelizer::initInst()
{
  if (_gl_instance == nullptr) {
    _gl_instance = new GraphLevelizer();
  }
}

GraphLevelizer& GraphLevelizer::getInst()
{
  if (_gl_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_gl_instance;
}

void GraphLevelizer::destroyInst()
{
  if (_gl_instance != nullptr) {
    delete _gl_instance;
    _gl_instance = nullptr;
  }
}

// function

bool GraphLevelizer::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  buildTimingOrder(database);
  printLoopInfo(database);

  STALOG.info(Loc::current(), "Levelize iSTA graph: timing_order=", database.get_timing_order_list().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphLevelizer* GraphLevelizer::_gl_instance = nullptr;

void GraphLevelizer::buildTimingOrder(Database& database)
{
  std::map<std::string, std::size_t> indegree_map = makeIndegreeMap(database);
  std::queue<std::string> pin_queue;
  pushRootPinList(database, indegree_map, pin_queue);

  database.get_timing_order_list().clear();
  while (!pin_queue.empty()) {
    std::string pin_name = pin_queue.front();
    pin_queue.pop();
    database.get_timing_order_list().push_back(pin_name);

    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      updateSinkLevel(database, arc);
      updateSinkIndegree(arc, indegree_map, pin_queue);
    }
  }
}

std::map<std::string, std::size_t> GraphLevelizer::makeIndegreeMap(Database& database)
{
  std::map<std::string, std::size_t> indegree_map;
  for (auto& timing_pair : database.get_timing_point_map()) {
    timing_pair.second.set_level(0);
    indegree_map[timing_pair.first] = database.get_incoming_arc_list_map()[timing_pair.first].size();
  }
  return indegree_map;
}

void GraphLevelizer::pushRootPinList(Database& database, std::map<std::string, std::size_t>& indegree_map,
                                     std::queue<std::string>& pin_queue)
{
  for (auto& timing_pair : database.get_timing_point_map()) {
    if (indegree_map[timing_pair.first] == 0) {
      database.get_timing_point_map()[timing_pair.first].set_level(1);
      pin_queue.push(timing_pair.first);
    }
  }
}

void GraphLevelizer::updateSinkLevel(Database& database, Arc& arc)
{
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  sink_point.set_level(std::max(sink_point.get_level(), source_point.get_level() + 1));
}

void GraphLevelizer::updateSinkIndegree(Arc& arc, std::map<std::string, std::size_t>& indegree_map, std::queue<std::string>& pin_queue)
{
  if (indegree_map[arc.get_sink_pin()] > 0) {
    --indegree_map[arc.get_sink_pin()];
  }
  if (indegree_map[arc.get_sink_pin()] == 0) {
    pin_queue.push(arc.get_sink_pin());
  }
}

void GraphLevelizer::printLoopInfo(Database& database)
{
  std::size_t loop_pin_num = database.get_timing_point_map().size() - database.get_timing_order_list().size();
  if (loop_pin_num > 0) {
    STALOG.warn(Loc::current(), "Detected ", loop_pin_num, " vertex(es) in combinational loop or unresolved dependency.");
  }
}

}  // namespace ista
