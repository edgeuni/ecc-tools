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
#include "RCModel.hpp"
#include "TopoEdge.hpp"

namespace ircx {

#define RCXRC (ircx::ResistanceCalculator::getInst())

class ResistanceCalculator
{
 public:
  static void initInst();
  static ResistanceCalculator& getInst();
  static void destroyInst();
  // function
  void calculate();

 private:
  // self
  static ResistanceCalculator* _rc_instance;

  ResistanceCalculator() = default;
  ResistanceCalculator(const ResistanceCalculator& other) = delete;
  ResistanceCalculator(ResistanceCalculator&& other) = delete;
  ~ResistanceCalculator() = default;
  ResistanceCalculator& operator=(const ResistanceCalculator& other) = delete;
  ResistanceCalculator& operator=(ResistanceCalculator&& other) = delete;
  // function
  RCModel initRCModel();
  void calculateRCModel(RCModel& rc_model);
  void calculateResistance();
  void calculateCornerResistance(Size corner_idx);
  void calculateNetResistance(Size corner_idx, Size net_idx);
  F64 calculateWireResistance(CornerData& corner_data, ProcessConductor& conductor, TopoEdge& edge,
                              std::span<EdgeEtchInterval> edge_interval_list);
  F64 calculateViaResistance(CornerData& corner_data, ProcessVia& via, TopoEdge& edge);
  F64 getTemperatureFactor(F64 temperature, F64 nominal_temperature, F64 temperature_coefficient1,
                           F64 temperature_coefficient2);
  ProcessVia* getProcessVia(CornerData& corner_data, Size design_layer_id);
  ProcessConductor* getProcessConductor(CornerData& corner_data, Size design_layer_id);
};

}  // namespace ircx
