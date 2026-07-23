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

#include "Config.hpp"
#include "DataManager.hpp"
#include "Database.hpp"
#include "Monitor.hpp"
#include "PRCandidate.hpp"
#include "PRModel.hpp"

namespace irt {

#define RTPR (irt::PlanarRouter::getInst())

class PlanarRouter
{
 public:
  static void initInst();
  static PlanarRouter& getInst();
  static void destroyInst();
  // function
  void generate();
  bool repair();

 private:
  // self
  static PlanarRouter* _pr_instance;

  PlanarRouter() = default;
  PlanarRouter(const PlanarRouter& other) = delete;
  PlanarRouter(PlanarRouter&& other) = delete;
  ~PlanarRouter() = default;
  PlanarRouter& operator=(const PlanarRouter& other) = delete;
  PlanarRouter& operator=(PlanarRouter&& other) = delete;
  struct BlockageEdge
  {
    RoutingEdge* routing_edge = nullptr;
    int32_t x = -1;
    int32_t y = -1;
    bool is_horizontal = false;
  };

  static constexpr int32_t kBlockageBlockSize = 32;
  int32_t _blockage_block_y_size = 0;
  std::vector<std::vector<BlockageEdge>> _blockage_edge_list_list;
  // model
  PRModel initPRModel();
  std::vector<PRNet> convertToPRNetList(std::vector<Net>& net_list);
  PRNet convertToPRNet(Net& net);
  void setPRComParam(PRModel& pr_model);
  void initPRTaskList(PRModel& pr_model);
  void buildPlanarRoutingEdgeMap();
  void updateLayerCongestion(PRModel& pr_model);
  std::vector<PRNet*> buildPRResult(PRModel& pr_model);

  struct PREdgeCost
  {
    double usage_cost = 0.0;
    double saturation_cost = 0.0;
    double hotspot_cost = 0.0;
    double overflow_cost = 0.0;
    double congestion_cost = 0.0;
    double overflow = 0.0;
    double max_usage_ratio = 0.0;
    bool is_saturated = false;
    bool is_hotspot = false;
    bool is_overflow = false;

    double getTotalCost() const { return usage_cost + saturation_cost + hotspot_cost + overflow_cost + congestion_cost; }
  };

  // routing edge
  RoutingEdge& getPlanarRoutingEdge(const PlanarCoord& first_coord, const PlanarCoord& second_coord);
  PREdgeCost getRoutingEdgeCost(RoutingEdge& routing_edge, double overflow_unit);
  void updateRoutingSegmentListToGraph(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list,
                                       ChangeType change_type, std::set<RoutingEdge*>& routing_edge_set);

  // route
  enum class PRRouteMode
  {
    kLZPattern,
    kAllPattern,
    kAStar
  };

  void runRouteFlow(PRModel& pr_model);
  void routePRNetList(PRModel& pr_model, const std::vector<PRNet*>& pr_net_list, const char* route_mode, PRRouteMode pr_route_mode);
  void routePRNet(PRModel& pr_model, PRNet* pr_net, PRRouteMode pr_route_mode);
  void initSingleTask(PRModel& pr_model, PRNet* pr_net);
  bool routeSingleTask(PRModel& pr_model, PRRouteMode pr_route_mode);
  void resetSingleTask(PRModel& pr_model);
  bool routePlanarTopoList(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list, PRRouteMode pr_route_mode);
  void updateCongestion(PRModel& pr_model);
  void updateBlockageEdgeList();
  std::vector<PRNet*> getOverflowPRNetList(PRModel& pr_model);
  std::vector<PRNet*> getHighUsagePRNetList(PRModel& pr_model);
  bool isBetterCandidate(PRModel& pr_model, PRCandidate& candidate, PRCandidate& current_best);
  std::vector<PRCandidate> getPRCandidateListByTopo(PRModel& pr_model, Segment<PlanarCoord>& planar_topo, PRRouteMode pr_route_mode);
  std::vector<Segment<PlanarCoord>> getPlanarTopoList(PRModel& pr_model);

  struct PRAStarNodeState
  {
    uint64_t search_stamp = 0;
    bool closed = false;
    int32_t parent_idx = -1;
    double known_cost = DBL_MAX;
  };
  struct PRAStarQueueNode
  {
    int32_t node_idx = -1;
    double known_cost = 0;
    double estimated_cost = 0;
    double getTotalCost() const { return known_cost + estimated_cost; }
  };
  struct PRAStarWorkspace
  {
    PlanarRect workspace_rect;
    int32_t x_size = 0;
    int32_t y_size = 0;
    uint64_t search_stamp = 0;
    std::vector<PRAStarNodeState> node_state_list;
    std::vector<PRAStarQueueNode> open_heap;
  };

  // A* route
  PRAStarWorkspace _astar_workspace;
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByAStar(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  bool prepareAStarWorkspace(const PlanarRect& workspace_rect, PRAStarWorkspace& workspace);
  int32_t getAStarNodeIndex(const PRAStarWorkspace& workspace, const PlanarCoord& coord);
  PlanarCoord getAStarNodeCoord(const PRAStarWorkspace& workspace, int32_t node_idx);
  PRAStarNodeState& getAStarNodeState(PRAStarWorkspace& workspace, int32_t node_idx);
  bool searchRoutingSegmentByAStar(PRModel& pr_model, const PlanarCoord& start_coord, const PlanarCoord& end_coord, PRAStarWorkspace& workspace,
                                   std::vector<Segment<PlanarCoord>>& routing_segment_list);
  PlanarRect getAStarSearchRect(Segment<PlanarCoord>& planar_topo, int32_t search_margin);
  double getAStarStepCost(PRModel& pr_model, const PlanarCoord& start_coord, const PlanarCoord& end_coord,
                          const PlanarCoord& parent_coord);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByCoordList(std::vector<PlanarCoord>& coord_list);

  // pattern route
  bool isLongObliqueTopo(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByStraight(Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLPattern(Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByZPattern(Segment<PlanarCoord>& planar_topo);
  std::vector<int32_t> getMidIndexList(int32_t first_idx, int32_t second_idx);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByUPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByInner3Bends(Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByOuter3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  void updatePRCandidate(PRModel& pr_model, PRCandidate& pr_candidate);

  // result
  MTree<PlanarCoord> getCoordTree(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void uploadNetResult(PRNet& pr_net);

  // exhibit
  void updateSummary(PRModel& pr_model);
  void printSummary(PRModel& pr_model);
  void outputGuide(PRModel& pr_model);
  void outputNetCSV(PRModel& pr_model);
  void outputOverflowCSV(PRModel& pr_model);
  void outputJson(PRModel& pr_model);
  std::string outputNetJson(PRModel& pr_model);
  std::string outputOverflowJson(PRModel& pr_model);
  std::string outputSummaryJson(PRModel& pr_model);

  // debug
  void debugPlotPRModel(PRModel& pr_model, std::string flag);
};

}  // namespace irt
