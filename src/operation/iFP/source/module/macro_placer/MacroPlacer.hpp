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

#include "MPModel.hpp"

namespace ifp {

#define FPMP (ifp::MacroPlacer::getInst())

class MacroPlacer
{
 public:
  static void initInst();
  static MacroPlacer& getInst();
  static void destroyInst();
  // function
  void place();

 private:
  // self
  static MacroPlacer* _mp_instance;

  MacroPlacer() = default;
  MacroPlacer(const MacroPlacer& other) = delete;
  MacroPlacer(MacroPlacer&& other) = delete;
  ~MacroPlacer() = default;
  MacroPlacer& operator=(const MacroPlacer& other) = delete;
  MacroPlacer& operator=(MacroPlacer&& other) = delete;
  // function

#if 1  // build

  void buildPlacementBlockage();
  void buildPlacementHalo();
  void buildRoutingBlockage();
  void buildRoutingHalo();
  void buildModel(MPModel& mp_model);
  void buildNodeList(MPModel& mp_model);
  void buildNetList(MPModel& mp_model);
  void buildBlockageRectList(MPModel& mp_model);

#endif

#if 1  // optimize

  void optimize(MPModel& mp_model);
  void initializeNodeLocation(MPModel& mp_model);
  double calculateCost(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);
  double calculateWirelength(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);
  double calculateOverlap(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);
  MPRect getNodeRect(const MPNode& mp_node);
  double calculateBlockageOverlap(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);
  double calculateOutOfBound(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);
  double calculatePeriphery(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);
  double calculateIODistance(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list);

#endif

#if 1  // output

  void writeModel(MPModel& mp_model);
  void writePlacementHalo();

#endif
};

}  // namespace ifp
