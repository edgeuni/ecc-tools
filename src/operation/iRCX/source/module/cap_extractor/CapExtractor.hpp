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

#include "CapTableConfig.hpp"
#include "CornerData.hpp"
#include "CrossOverlapSub.hpp"
#include "DataManager.hpp"
#include "EdgeEnvInterval.hpp"
#include "EdgeEtchInterval.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "ProcessConductor.hpp"
#include "RCXHeader.hpp"
#include "TopoEdge.hpp"

namespace ircx {

#define RCXCE (ircx::CapExtractor::getInst())

class CapExtractor
{
 public:
  static void initInst();
  static CapExtractor& getInst();
  static void destroyInst();
  // function
  void extract();

 private:
  // self
  static CapExtractor* _ce_instance;

  CapExtractor() = default;
  CapExtractor(const CapExtractor& other) = delete;
  CapExtractor(CapExtractor&& other) = delete;
  ~CapExtractor() = default;
  CapExtractor& operator=(const CapExtractor& other) = delete;
  CapExtractor& operator=(CapExtractor&& other) = delete;
  // function
  void extractCap();
  void extractCornerCap(int32_t corner_idx);
  void extractNetCap(int32_t corner_idx, int32_t net_idx);
  void extractEdgeCap(int32_t corner_idx, int32_t net_idx, int32_t edge_idx);
  void extractEdgeIntervalCap(int32_t corner_idx, int32_t net_idx, int32_t edge_idx, int32_t interval_idx);
  void extractCapSpan(int32_t corner_idx, int32_t net_idx, int32_t edge_idx, int32_t interval_idx, int32_t start_coordinate,
                      int32_t end_coordinate);
  void getCrossLayerName(std::vector<CrossOverlapSub>& cross_overlap_sub_list, int32_t start_coordinate, int32_t end_coordinate,
                         std::string& below_layer_name, std::string& above_layer_name);
  void addGroundCap(int32_t corner_idx, int32_t net_idx, int32_t edge_idx, TopoEdge* adjacent_edge, double ground_cap);
  void addCouplingCap(int32_t corner_idx, int32_t net_idx, int32_t edge_idx, TopoEdge* adjacent_edge, double coupling_cap);
  ProcessConductor* getProcessConductor(CornerData& corner_data, int32_t design_layer_id);
  CapTableConfig* getCapTableConfig(CornerData& corner_data, std::string& process_layer_name, std::string& below_layer_name,
                                    std::string& above_layer_name);
  void getCap(CapTableConfig& cap_table_config, double spacing, double& coupling_cap, double& ground_cap);
  void getFarthestCap(CapTableConfig& cap_table_config, double& coupling_cap, double& ground_cap);
};

}  // namespace ircx
