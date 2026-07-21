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

#include "DataManager.hpp"
#include "CornerData.hpp"
#include "EdgeEtchInterval.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "ProcessConductor.hpp"
#include "ProcessVia.hpp"
#include "RCXHeader.hpp"
#include "TopoEdge.hpp"

namespace ircx {

#define RCXRE (ircx::ResExtractor::getInst())

class ResExtractor
{
 public:
  static void initInst();
  static ResExtractor& getInst();
  static void destroyInst();
  // function
  void extract();

 private:
  // self
  static ResExtractor* _re_instance;

  ResExtractor() = default;
  ResExtractor(const ResExtractor& other) = delete;
  ResExtractor(ResExtractor&& other) = delete;
  ~ResExtractor() = default;
  ResExtractor& operator=(const ResExtractor& other) = delete;
  ResExtractor& operator=(ResExtractor&& other) = delete;
  // function
  void extractRes();
  void extractCornerRes(size_t corner_idx);
  void extractNetRes(size_t corner_idx, size_t net_idx);
  double extractWireRes(CornerData& corner_data, ProcessConductor& conductor, TopoEdge& edge,
                              std::span<EdgeEtchInterval> edge_interval_list);
  double extractViaRes(CornerData& corner_data, ProcessVia& via, TopoEdge& edge);
  double getTemperatureFactor(double temperature, double nominal_temperature, double temperature_coefficient1,
                           double temperature_coefficient2);
  ProcessVia* getProcessVia(CornerData& corner_data, size_t design_layer_id);
  ProcessConductor* getProcessConductor(CornerData& corner_data, size_t design_layer_id);
};

}  // namespace ircx
