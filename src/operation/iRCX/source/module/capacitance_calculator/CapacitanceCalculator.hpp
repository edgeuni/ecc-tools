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
#include "CapTableConfig.hpp"
#include "CornerData.hpp"
#include "CrossOverlapSub.hpp"
#include "EdgeEnvironmentInterval.hpp"
#include "EdgeEtchInterval.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "ProcessConductor.hpp"
#include "RCXHeader.hpp"
#include "CCModel.hpp"
#include "TopoEdge.hpp"

namespace ircx {

#define RCXCC (ircx::CapacitanceCalculator::getInst())

class CapacitanceCalculator
{
 public:
  static void initInst();
  static CapacitanceCalculator& getInst();
  static void destroyInst();
  // function
  void calculate();

 private:
  // self
  static CapacitanceCalculator* _cc_instance;

  CapacitanceCalculator() = default;
  CapacitanceCalculator(const CapacitanceCalculator& other) = delete;
  CapacitanceCalculator(CapacitanceCalculator&& other) = delete;
  ~CapacitanceCalculator() = default;
  CapacitanceCalculator& operator=(const CapacitanceCalculator& other) = delete;
  CapacitanceCalculator& operator=(CapacitanceCalculator&& other) = delete;
  // function
  CCModel initCCModel();
  void calculateCCModel(CCModel& cc_model);
  void calculateCapacitance();
  void calculateCornerCapacitance(Size corner_idx);
  void calculateNetCapacitance(Size corner_idx, Size net_idx);
  void calculateEdgeCapacitance(Size corner_idx, Size net_idx, Size edge_idx);
  void calculateEdgeIntervalCapacitance(Size corner_idx, Size net_idx, Size edge_idx, Size interval_idx);
  void calculateCapacitanceSpan(Size corner_idx, Size net_idx, Size edge_idx, Size interval_idx, Dbu start_coordinate, Dbu end_coordinate);
  void getCrossLayerName(std::vector<CrossOverlapSub>& cross_overlap_sub_list, Dbu start_coordinate, Dbu end_coordinate,
                         std::string& below_layer_name, std::string& above_layer_name);
  void addGroundCapacitance(Size corner_idx, Size net_idx, Size edge_idx, TopoEdge* adjacent_edge, F64 ground_capacitance);
  void addCouplingCapacitance(Size corner_idx, Size net_idx, Size edge_idx, TopoEdge* adjacent_edge, F64 coupling_capacitance);
  ProcessConductor* getProcessConductor(CornerData& corner_data, Size design_layer_id);
  CapTableConfig* getCapTableConfig(CornerData& corner_data, std::string& process_layer_name, std::string& below_layer_name,
                                    std::string& above_layer_name);
  void getCapacitance(CapTableConfig& cap_table_config, Micron spacing, F64& coupling_capacitance, F64& ground_capacitance);
  void getFarthestCapacitance(CapTableConfig& cap_table_config, F64& coupling_capacitance, F64& ground_capacitance);
};

}  // namespace ircx
