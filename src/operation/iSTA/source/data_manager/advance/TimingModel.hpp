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
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "Arc.hpp"
#include "Instance.hpp"
#include "Net.hpp"
#include "Pin.hpp"
#include "Summary.hpp"
#include "TimingPoint.hpp"

namespace ista {

class TimingModel
{
 public:
  void reset()
  {
    design_name.clear();
    report_directory = "iSTA_result";
    instances.clear();
    pins.clear();
    nets.clear();
    clearGraph();
  }

  void clearGraph()
  {
    arcs.clear();
    outgoing_arc_list.clear();
    incoming_arc_list.clear();
    startpoint_list.clear();
    endpoint_list.clear();
    clearTiming();
  }

  void clearTiming()
  {
    timing_points.clear();
    summary = Summary();
  }

  std::string design_name;
  std::string report_directory = "iSTA_result";

  std::unordered_map<std::string, Instance> instances;
  std::unordered_map<std::string, Pin> pins;
  std::unordered_map<std::string, Net> nets;
  std::vector<Arc> arcs;

  std::unordered_map<std::string, std::vector<std::size_t>> outgoing_arc_list;
  std::unordered_map<std::string, std::vector<std::size_t>> incoming_arc_list;
  std::vector<std::string> startpoint_list;
  std::vector<std::string> endpoint_list;

  std::unordered_map<std::string, TimingPoint> timing_points;
  Summary summary;
};

}  // namespace ista
