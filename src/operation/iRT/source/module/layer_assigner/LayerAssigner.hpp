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

#include "ChangeType.hpp"
#include "Config.hpp"
#include "DataManager.hpp"
#include "Database.hpp"
#include "LAModel.hpp"
#include "LAPackage.hpp"
#include "RTHeader.hpp"

namespace irt {

#define RTLA (irt::LayerAssigner::getInst())

class LayerAssigner
{
 public:
  static void initInst();
  static LayerAssigner& getInst();
  static void destroyInst();
  // function
  void assign();

 private:
  struct LAOverflowSegment
  {
    PlanarCoord first_coord;
    PlanarCoord second_coord;
    PlanarCoord split_coord;
    int32_t total_overflow = 0;
    int32_t max_overflow = 0;
    bool is_split = false;
  };
  // self
  static LayerAssigner* _la_instance;

  LayerAssigner() = default;
  LayerAssigner(const LayerAssigner& other) = delete;
  LayerAssigner(LayerAssigner&& other) = delete;
  ~LayerAssigner() = default;
  LayerAssigner& operator=(const LayerAssigner& other) = delete;
  LayerAssigner& operator=(LayerAssigner&& other) = delete;
  // model
  LAModel initLAModel();
  std::vector<LANet> convertToLANetList(std::vector<Net>& net_list);
  LANet convertToLANet(Net& net);
  void initLATaskList(LAModel& la_model);
  void setLAComParam(LAModel& la_model);
  // routing edge
  RoutingEdge& getRoutingEdge(const LayerCoord& first_coord, const LayerCoord& second_coord);
  double getOverflowCost(RoutingEdge& routing_edge, double overflow_unit);
  // route
  void buildPlaneTree(LAModel& la_model);
  void routeLAModel(LAModel& la_model);
  void routeLATask(LAModel& la_model, LANet* la_task);
  void initSingleTask(LAModel& la_model, LANet* la_task);
  void routeSingleTask(LAModel& la_model);
  bool needRouting(LAModel& la_model);
  std::vector<LAOverflowSegment> getOverflowSegmentList(LAModel& la_model);
  void splitPlaneTreeByOverflow(LAModel& la_model, std::vector<LAOverflowSegment>& overflow_segment_list);
  // layer assignment
  void buildPillarTree(LAModel& la_model);
  LAPillar convertLAPillar(LayerCoord& layer_coord, std::map<PlanarCoord, std::set<int32_t>, CmpPlanarCoordByXASC>& coord_pin_layer_map);
  void assignPillarTree(LAModel& la_model);
  void buildSubtreeCost(LAModel& la_model);
  std::vector<int32_t> getCandidateLayerList(LAModel& la_model, LAPackage& la_package);
  double getPillarViaCost(LAModel& la_model, const std::set<int32_t>& layer_idx_set);
  double getSegmentCost(LAModel& la_model, LAPackage& la_package, int32_t candidate_layer_idx);
  void assignLayer(LAModel& la_model);
  // result
  void buildRoutingTree(LAModel& la_model);
  std::vector<Segment<LayerCoord>> getRoutingSegmentList(LAModel& la_model);
  MTree<LayerCoord> getCoordTree(LAModel& la_model, std::vector<Segment<LayerCoord>>& routing_segment_list);
  void uploadNetResult(LAModel& la_model);
  void resetSingleTask(LAModel& la_model);

  // environment
  void updateRoutingTreeToGraph(LAModel& la_model, ChangeType change_type);
  // exhibit
  void updateSummary(LAModel& la_model);
  void printSummary(LAModel& la_model);
  void outputGuide(LAModel& la_model);
  void outputNetCSV(LAModel& la_model);
  void outputOverflowCSV(LAModel& la_model);
  void outputJson(LAModel& la_model);
  std::string outputNetJson(LAModel& la_model);
  std::string outputOverflowJson(LAModel& la_model);
  std::string outputSummaryJson(LAModel& la_model);
  // debug
  void debugPlotLAModel(LAModel& la_model, std::string flag);
};

}  // namespace irt
