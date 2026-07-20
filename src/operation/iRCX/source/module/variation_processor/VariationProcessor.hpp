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
#include "RCXHeader.hpp"
#include "VPModel.hpp"

namespace ircx {

#define RCXVP (ircx::VariationProcessor::getInst())

class VariationProcessor
{
 public:
  static void initInst();
  static VariationProcessor& getInst();
  static void destroyInst();
  // function
  void process();

 private:
  // self
  static VariationProcessor* _vp_instance;

  VariationProcessor() = default;
  VariationProcessor(const VariationProcessor& other) = delete;
  VariationProcessor(VariationProcessor&& other) = delete;
  ~VariationProcessor() = default;
  VariationProcessor& operator=(const VariationProcessor& other) = delete;
  VariationProcessor& operator=(VariationProcessor&& other) = delete;
  // function
  VPModel initVPModel();
  void processVPModel(VPModel& vp_model);
  void buildCornerNetEtchProfilePool();
  void buildNetEtchProfile(Size corner_idx, Size net_idx);
  void applyCornerNetEffectiveGeometryList();
  void applyNetEffectiveGeometry(Size corner_idx, Size net_idx);
  void applyEdgeEffectiveGeometry(ProcessConductor& conductor, EdgeEtchInterval& edge_interval);
  ProcessConductor* getProcessConductor(CornerData& corner_data, Size design_layer_id);
};

}  // namespace ircx
