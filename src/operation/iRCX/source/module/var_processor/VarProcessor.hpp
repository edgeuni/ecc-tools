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

namespace ircx {

#define RCXVP (ircx::VarProcessor::getInst())

class VarProcessor
{
 public:
  static void initInst();
  static VarProcessor& getInst();
  static void destroyInst();
  // function
  void process();

 private:
  // self
  static VarProcessor* _vp_instance;

  VarProcessor() = default;
  VarProcessor(const VarProcessor& other) = delete;
  VarProcessor(VarProcessor&& other) = delete;
  ~VarProcessor() = default;
  VarProcessor& operator=(const VarProcessor& other) = delete;
  VarProcessor& operator=(VarProcessor&& other) = delete;
  // function
  void buildCornerNetEtchProfilePool();
  void buildNetEtchProfile(size_t corner_idx, size_t net_idx);
  void applyCornerNetEffectiveGeometryList();
  void applyNetEffectiveGeometry(size_t corner_idx, size_t net_idx);
  void applyEdgeEffectiveGeometry(ProcessConductor& conductor, EdgeEtchInterval& edge_interval);
  ProcessConductor* getProcessConductor(CornerData& corner_data, size_t design_layer_id);
};

}  // namespace ircx
