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

#include <cmath>
#include <limits>

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

  TimingModel& timing_model = STADM.getTimingModel();
  Summary& summary = timing_model.summary;

  summary.instance_num = timing_model.instances.size();
  summary.port_num = 0;
  for (const auto& [pin_name, pin] : timing_model.pins) {
    if (pin.is_port) {
      ++summary.port_num;
    }
  }
  summary.pin_num = timing_model.pins.size();
  summary.net_num = timing_model.nets.size();
  summary.arc_num = timing_model.arcs.size();
  summary.startpoint_num = timing_model.startpoint_list.size();
  summary.endpoint_num = timing_model.endpoint_list.size();
  summary.worst_slack = std::numeric_limits<double>::infinity();
  summary.worst_endpoint.clear();

  for (const std::string& endpoint : timing_model.endpoint_list) {
    auto timing_iter = timing_model.timing_points.find(endpoint);
    if (timing_iter == timing_model.timing_points.end()) {
      continue;
    }
    const TimingPoint& timing_point = timing_iter->second;
    if (!std::isfinite(timing_point.arrival) || !std::isfinite(timing_point.required)) {
      continue;
    }
    if (timing_point.slack < summary.worst_slack) {
      summary.worst_slack = timing_point.slack;
      summary.worst_endpoint = endpoint;
    }
  }

  if (!std::isfinite(summary.worst_slack)) {
    summary.worst_slack = 0.0;
  }

  STALOG.info(Loc::current(), "Analyze iSTA timing: worst_slack=", summary.worst_slack, " endpoint=", summary.worst_endpoint);
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

TimingAnalyzer* TimingAnalyzer::_ta_instance = nullptr;

}  // namespace ista
