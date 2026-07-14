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

#include <array>

#include "Config.hpp"
#include "DataManager.hpp"
#include "Database.hpp"
#include "Monitor.hpp"
#include "OpenQueue.hpp"
#include "PRCandidate.hpp"
#include "PRModel.hpp"
#include "PRSegmentTask.hpp"

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

 private:
  // self
  static PlanarRouter* _pr_instance;

  PlanarRouter() = default;
  PlanarRouter(const PlanarRouter& other) = delete;
  PlanarRouter(PlanarRouter&& other) = delete;
  ~PlanarRouter() = default;
  PlanarRouter& operator=(const PlanarRouter& other) = delete;
  PlanarRouter& operator=(PlanarRouter&& other) = delete;
  // function
  PRModel initPRModel();
  std::vector<PRNet> convertToPRNetList(std::vector<Net>& net_list);
  PRNet convertToPRNet(Net& net);
  void setPRComParam(PRModel& pr_model);
  void initPRTaskList(PRModel& pr_model);
  void buildPRNodeMap(PRModel& pr_model);
  void buildPRNodeNeighbor(PRModel& pr_model);
  void buildOrientSupply(PRModel& pr_model);
  void generatePRModel(PRModel& pr_model);
  void routePRTask(PRModel& pr_model, PRNet* pr_net);
  void initSingleTask(PRModel& pr_model, PRNet* pr_net);
  struct PRShadowDemandMap
  {
    GridMap<uint8_t>* orient_mask_map = nullptr;
    GridMap<int32_t>* stamp_map = nullptr;
    int32_t stamp = 0;
    std::vector<PlanarCoord> touched_coord_list;
    bool empty() const { return touched_coord_list.empty(); }
    bool isInside(int32_t x, int32_t y) const { return orient_mask_map != nullptr && orient_mask_map->isInside(x, y); }
    uint8_t getMask(int32_t x, int32_t y) const
    {
      if (!isInside(x, y) || stamp_map == nullptr || (*stamp_map)[x][y] != stamp) {
        return 0;
      }
      return (*orient_mask_map)[x][y];
    }
    void addMask(int32_t x, int32_t y, uint8_t mask)
    {
      if (mask == 0 || !isInside(x, y) || stamp_map == nullptr) {
        return;
      }
      if ((*stamp_map)[x][y] != stamp) {
        (*stamp_map)[x][y] = stamp;
        (*orient_mask_map)[x][y] = 0;
        touched_coord_list.emplace_back(x, y);
      }
      (*orient_mask_map)[x][y] |= mask;
    }
  };
  std::vector<Segment<PlanarCoord>> getRoutingSegmentList(PRModel& pr_model);
  std::vector<PRCandidate> getPRCandidateListByTopo(PRModel& pr_model, int32_t topo_idx, Segment<PlanarCoord>& planar_topo,
                                                    const PRShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<PRCandidate> getPRCandidateList(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& planar_topo_list);
  std::vector<Segment<PlanarCoord>> getPlanarTopoList(PRModel& pr_model);
  bool isLongObliqueTopo(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByStraight(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByZPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<int32_t> getMidIndexList(int32_t first_idx, int32_t second_idx);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByUPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByInner3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLowCostLane3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo,
                                                                                          const PRShadowDemandMap* shadow_demand_map = nullptr);
  double getPatternSegmentFastScore(PRModel& pr_model, Segment<PlanarCoord>& segment, const PRShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByOuter3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo);
  struct PRAStarNodeCostCache
  {
    PlanarRect route_window;
    GridMap<std::array<double, 2>> cost_map;
    GridMap<std::array<bool, 2>> valid_map;
  };
  PRShadowDemandMap initPRShadowDemandMap(PRModel& pr_model);
  bool isBetterCandidate(PRCandidate& candidate, PRCandidate& current_best, double corner_weight);
  void updatePRCandidate(PRModel& pr_model, PRCandidate& pr_candidate, const PRShadowDemandMap* shadow_demand_map = nullptr);
  MTree<PlanarCoord> getCoordTree(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void uploadNetResult(PRModel& pr_model, MTree<PlanarCoord>& coord_tree);
  void resetSingleTask(PRModel& pr_model);
  void reroutePRModel(PRModel& pr_model);
  void setPRIterParam(PRModel& pr_model);
  void rebuildDemandToGraph(PRModel& pr_model);
  void initNetGlobalResultMap(PRModel& pr_model);
  double getOverflow(PRModel& pr_model);
  double getCongestionRisk(PRModel& pr_model);
  double getHighUsage(PRModel& pr_model);
  double getWireLength(PRModel& pr_model);
  void updateCongestionRisk(PRModel& pr_model);
  void initPRMetric(PRModel& pr_model);
  void collectPRHotspotInfo(PRModel& pr_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set, std::set<int32_t>& overflow_net_set,
                            std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set, std::set<int32_t>& high_usage_net_set);
  std::vector<PRSegmentTask> initPRSegmentTaskList(PRModel& pr_model, bool include_overflow = true, bool include_high_usage = true,
                                                   bool high_usage_first = false);
  void routePRSegmentTaskListByHighUsage(PRModel& pr_model);
  bool routePRSegmentTask(PRModel& pr_model, PRSegmentTask& pr_segment_task, bool enable_true_local_accept);
  void routePRNetTaskListByPattern(PRModel& pr_model);
  bool routePRNetTaskByPattern(PRModel& pr_model, int32_t net_idx);
  bool routePRTopoEdgeByPattern(PRModel& pr_model, int32_t topo_idx, Segment<PlanarCoord>& topo_edge, PRShadowDemandMap& shadow_demand_map,
                                std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void routePRNetTaskListByAStar(PRModel& pr_model);
  std::vector<int32_t> initPRNetTaskList(PRModel& pr_model, bool include_high_usage, bool include_changed_net);
  bool routePRNetTaskByAStar(PRModel& pr_model, int32_t net_idx);
  bool routePRTopoEdgeByAStar(PRModel& pr_model, int32_t net_idx, Segment<PlanarCoord>& topo_edge, std::vector<Segment<PlanarCoord>>& routing_segment_list);
  std::vector<PlanarRect> getRouteWindowList(PRModel& pr_model, PRSegmentTask& pr_segment_task);
  PlanarRect getRouteWindow(PRModel& pr_model, PRSegmentTask& pr_segment_task, int32_t expand_size);
  PlanarRect getDieWindow(PRModel& pr_model);
  bool searchSegmentByAStar(PRModel& pr_model, PRSegmentTask& pr_segment_task, PlanarRect& route_window,
                            std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void initPathHead(PRModel& pr_model, PRNode* start_node, PRNode* end_node, std::vector<PRNode*>& visited_node_list, OpenQueue<PRNode>& open_queue);
  bool searchEnded(PRNode* path_head_node, PRNode* end_node);
  void expandSearching(PRModel& pr_model, PRSegmentTask& pr_segment_task, PlanarRect& route_window, PRNode* path_head_node, PRNode* end_node,
                       std::vector<PRNode*>& visited_node_list, OpenQueue<PRNode>& open_queue, PRAStarNodeCostCache& node_cost_cache);
  PRNode* popFromOpenList(OpenQueue<PRNode>& open_queue);
  void resetPathState(std::vector<PRNode*>& visited_node_list, OpenQueue<PRNode>& open_queue);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByNode(PRNode* node);
  double getKnownCost(PRModel& pr_model, PRSegmentTask& pr_segment_task, PRNode* start_node, PRNode* end_node, PRAStarNodeCostCache& node_cost_cache);
  double getNodeCost(PRModel& pr_model, PRSegmentTask& pr_segment_task, PRNode* curr_node, Direction direction, PRAStarNodeCostCache& node_cost_cache);
  double getEstimateCost(PRModel& pr_model, PRNode* start_node, PRNode* end_node);
  bool isSegmentCrossOverflow(PRModel& pr_model, Segment<LayerCoord>* segment, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set);
  bool isSegmentCrossHighUsage(PRModel& pr_model, Segment<LayerCoord>* segment, std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set);
  double getSegmentOverflow(PRModel& pr_model, Segment<LayerCoord>* segment);
  double getSegmentCongestionRisk(PRModel& pr_model, Segment<LayerCoord>* segment);
  double getSegmentHighUsage(PRModel& pr_model, Segment<LayerCoord>* segment);
  double getSegmentMaxUsageRatio(PRModel& pr_model, Segment<LayerCoord>* segment);
  void updateBestResult(PRModel& pr_model);
  void uploadBestResult(PRModel& pr_model);

#if 1  // update env
  void updateDemandToGraph(PRModel& pr_model, ChangeType change_type, MTree<PlanarCoord>& coord_tree);
  void updateDemandToGraph(PRModel& pr_model, ChangeType change_type, int32_t net_idx, std::vector<Segment<PlanarCoord>>& segment_list);
  void addCandidateToShadow(PRShadowDemandMap& shadow_map, PRCandidate& pr_candidate);
#endif

#if 1  // exhibit
  void updateSummary(PRModel& pr_model);
  void printSummary(PRModel& pr_model);
  void outputGuide(PRModel& pr_model);
  void outputNetCSV(PRModel& pr_model);
  void outputOverflowCSV(PRModel& pr_model);
  void outputCongestionSnapshotCSV(PRModel& pr_model, const std::string& suffix, int32_t iter);
  void outputCongestionCSV(PRModel& pr_model, const std::string& suffix = "", int32_t iter = -1);
  void outputJson(PRModel& pr_model);
  std::string outputNetJson(PRModel& pr_model);
  std::string outputOverflowJson(PRModel& pr_model);
  std::string outputSummaryJson(PRModel& pr_model);
#endif

#if 1  // debug
  void debugPlotPRModel(PRModel& pr_model, std::string flag);
  void debugCheckPRModel(PRModel& pr_model);
#endif
};

}  // namespace irt
