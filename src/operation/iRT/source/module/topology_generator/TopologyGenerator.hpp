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
#include "TGCandidate.hpp"
#include "TGSegmentTask.hpp"
#include "TGModel.hpp"

namespace irt {

#define RTTG (irt::TopologyGenerator::getInst())

class TopologyGenerator
{
 public:
  static void initInst();
  static TopologyGenerator& getInst();
  static void destroyInst();
  // function
  void generate();

 private:
  // self
  static TopologyGenerator* _tg_instance;

  TopologyGenerator() = default;
  TopologyGenerator(const TopologyGenerator& other) = delete;
  TopologyGenerator(TopologyGenerator&& other) = delete;
  ~TopologyGenerator() = default;
  TopologyGenerator& operator=(const TopologyGenerator& other) = delete;
  TopologyGenerator& operator=(TopologyGenerator&& other) = delete;
  // function
  TGModel initTGModel();
  std::vector<TGNet> convertToTGNetList(std::vector<Net>& net_list);
  TGNet convertToTGNet(Net& net);
  void setTGComParam(TGModel& tg_model);
  void initTGTaskList(TGModel& tg_model);
  void buildTGNodeMap(TGModel& tg_model);
  void buildTGNodeNeighbor(TGModel& tg_model);
  void buildOrientSupply(TGModel& tg_model);
  void generateTGModel(TGModel& tg_model);
  void routeTGTask(TGModel& tg_model, TGNet* tg_net);
  void initSingleTask(TGModel& tg_model, TGNet* tg_net);
  struct TGShadowDemandMap
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
  std::vector<Segment<PlanarCoord>> getRoutingSegmentList(TGModel& tg_model);
  std::vector<TGCandidate> getTGCandidateListByTopo(TGModel& tg_model, int32_t topo_idx, Segment<PlanarCoord>& planar_topo,
                                                    const TGShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<TGCandidate> getTGCandidateList(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& planar_topo_list);
  std::vector<Segment<PlanarCoord>> getPlanarTopoList(TGModel& tg_model);
  bool isLongObliqueTopo(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByStraight(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByZPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<int32_t> getMidIndexList(int32_t first_idx, int32_t second_idx);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByUPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByInner3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLowCostLane3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo,
                                                                                          const TGShadowDemandMap* shadow_demand_map = nullptr);
  double getPatternSegmentFastScore(TGModel& tg_model, Segment<PlanarCoord>& segment, const TGShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByOuter3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  struct TGAStarNodeCostCache
  {
    PlanarRect route_window;
    GridMap<std::array<double, 2>> cost_map;
    GridMap<std::array<bool, 2>> valid_map;
  };
  TGShadowDemandMap initTGShadowDemandMap(TGModel& tg_model);
  bool isBetterCandidate(TGCandidate& candidate, TGCandidate& current_best, double corner_weight);
  void updateTGCandidate(TGModel& tg_model, TGCandidate& tg_candidate,
                         const TGShadowDemandMap* shadow_demand_map = nullptr);
  MTree<PlanarCoord> getCoordTree(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void uploadNetResult(TGModel& tg_model, MTree<PlanarCoord>& coord_tree);
  void resetSingleTask(TGModel& tg_model);
  void rerouteTGModel(TGModel& tg_model);
  void setTGIterParam(TGModel& tg_model);
  void rebuildDemandToGraph(TGModel& tg_model);
  void initNetGlobalResultMap(TGModel& tg_model);
  double getOverflow(TGModel& tg_model);
  double getCongestionRisk(TGModel& tg_model);
  double getHighUsage(TGModel& tg_model);
  double getWireLength(TGModel& tg_model);
  void updateCongestionRisk(TGModel& tg_model);
  void initTGMetric(TGModel& tg_model);
  void collectTGHotspotInfo(TGModel& tg_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set,
                            std::set<int32_t>& overflow_net_set,
                            std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set,
                            std::set<int32_t>& high_usage_net_set);
  std::vector<TGSegmentTask> initTGSegmentTaskList(TGModel& tg_model, bool include_overflow = true, bool include_high_usage = true,
                                                   bool high_usage_first = false);
  void routeTGSegmentTaskListByHighUsage(TGModel& tg_model);
  bool routeTGSegmentTask(TGModel& tg_model, TGSegmentTask& tg_segment_task, bool enable_true_local_accept);
  void routeTGNetTaskListByPattern(TGModel& tg_model);
  bool routeTGNetTaskByPattern(TGModel& tg_model, int32_t net_idx);
  bool routeTGTopoEdgeByPattern(TGModel& tg_model, int32_t topo_idx, Segment<PlanarCoord>& topo_edge,
                                TGShadowDemandMap& shadow_demand_map, std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void routeTGNetTaskListByAStar(TGModel& tg_model);
  std::vector<int32_t> initTGNetTaskList(TGModel& tg_model, bool include_high_usage, bool include_changed_net);
  bool routeTGNetTaskByAStar(TGModel& tg_model, int32_t net_idx);
  bool routeTGTopoEdgeByAStar(TGModel& tg_model, int32_t net_idx, Segment<PlanarCoord>& topo_edge,
                              std::vector<Segment<PlanarCoord>>& routing_segment_list);
  std::vector<PlanarRect> getRouteWindowList(TGModel& tg_model, TGSegmentTask& tg_segment_task);
  PlanarRect getRouteWindow(TGModel& tg_model, TGSegmentTask& tg_segment_task, int32_t expand_size);
  PlanarRect getDieWindow(TGModel& tg_model);
  bool searchSegmentByAStar(TGModel& tg_model, TGSegmentTask& tg_segment_task, PlanarRect& route_window,
                            std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void initPathHead(TGModel& tg_model, TGNode* start_node, TGNode* end_node, std::vector<TGNode*>& visited_node_list, OpenQueue<TGNode>& open_queue);
  bool searchEnded(TGNode* path_head_node, TGNode* end_node);
  void expandSearching(TGModel& tg_model, TGSegmentTask& tg_segment_task, PlanarRect& route_window, TGNode* path_head_node, TGNode* end_node,
                       std::vector<TGNode*>& visited_node_list, OpenQueue<TGNode>& open_queue, TGAStarNodeCostCache& node_cost_cache);
  TGNode* popFromOpenList(OpenQueue<TGNode>& open_queue);
  void resetPathState(std::vector<TGNode*>& visited_node_list, OpenQueue<TGNode>& open_queue);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByNode(TGNode* node);
  double getKnownCost(TGModel& tg_model, TGSegmentTask& tg_segment_task, TGNode* start_node, TGNode* end_node,
                      TGAStarNodeCostCache& node_cost_cache);
  double getNodeCost(TGModel& tg_model, TGSegmentTask& tg_segment_task, TGNode* curr_node, Direction direction,
                     TGAStarNodeCostCache& node_cost_cache);
  double getEstimateCost(TGModel& tg_model, TGNode* start_node, TGNode* end_node);
  bool isSegmentCrossOverflow(TGModel& tg_model, Segment<LayerCoord>* segment, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set);
  bool isSegmentCrossHighUsage(TGModel& tg_model, Segment<LayerCoord>* segment,
                               std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set);
  double getSegmentOverflow(TGModel& tg_model, Segment<LayerCoord>* segment);
  double getSegmentCongestionRisk(TGModel& tg_model, Segment<LayerCoord>* segment);
  double getSegmentHighUsage(TGModel& tg_model, Segment<LayerCoord>* segment);
  double getSegmentMaxUsageRatio(TGModel& tg_model, Segment<LayerCoord>* segment);
  void updateBestResult(TGModel& tg_model);
  void uploadBestResult(TGModel& tg_model);

#if 1  // update env
  void updateDemandToGraph(TGModel& tg_model, ChangeType change_type, MTree<PlanarCoord>& coord_tree);
  void updateDemandToGraph(TGModel& tg_model, ChangeType change_type, int32_t net_idx, std::vector<Segment<PlanarCoord>>& segment_list);
  void addCandidateToShadow(TGShadowDemandMap& shadow_map, TGCandidate& tg_candidate);
#endif

#if 1  // exhibit
  void updateSummary(TGModel& tg_model);
  void printSummary(TGModel& tg_model);
  void outputGuide(TGModel& tg_model);
  void outputNetCSV(TGModel& tg_model);
  void outputOverflowCSV(TGModel& tg_model);
  void outputCongestionSnapshotCSV(TGModel& tg_model, const std::string& suffix, int32_t iter);
  void outputCongestionCSV(TGModel& tg_model, const std::string& suffix = "", int32_t iter = -1);
  void outputJson(TGModel& tg_model);
  std::string outputNetJson(TGModel& tg_model);
  std::string outputOverflowJson(TGModel& tg_model);
  std::string outputSummaryJson(TGModel& tg_model);
#endif

#if 1  // debug
  void debugPlotTGModel(TGModel& tg_model, std::string flag);
  void debugCheckTGModel(TGModel& tg_model);
#endif
};

}  // namespace irt
