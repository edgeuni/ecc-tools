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
  Summary& summary = database.get_summary();

  summary.instance_num = database.get_instance_map().size();
  summary.port_num = 0;
  for (auto& [pin_name, pin] : database.get_pin_map()) {
    if (pin.get_is_port()) {
      summary.port_num++;
    }
  }
  summary.pin_num = database.get_pin_map().size();
  summary.net_num = database.get_net_map().size();
  summary.arc_num = database.get_arc_list().size();
  summary.start_point_num = database.get_start_point_list().size();
  summary.end_point_num = database.get_end_point_list().size();
  summary.worst_slack = std::numeric_limits<double>::infinity();
  summary.worst_end_point.clear();

  for (std::string& end_point : database.get_end_point_list()) {
    auto timing_iter = database.get_timing_point_map().find(end_point);
    if (timing_iter == database.get_timing_point_map().end()) {
      continue;
    }
    TimingPoint& timing_point = timing_iter->second;
    if (!std::isfinite(timing_point.get_arrival()) || !std::isfinite(timing_point.get_required())) {
      continue;
    }
    if (timing_point.get_slack() < summary.worst_slack) {
      summary.worst_slack = timing_point.get_slack();
      summary.worst_end_point = end_point;
    }
  }

  if (!std::isfinite(summary.worst_slack)) {
    summary.worst_slack = 0.0;
  }

  STALOG.info(Loc::current(), "Analyze iSTA timing: worst_slack=", summary.worst_slack, " end_point=", summary.worst_end_point);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

TimingAnalyzer* TimingAnalyzer::_ta_instance = nullptr;

}  // namespace ista
