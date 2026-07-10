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
#include "TGCandidate.hpp"
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
  void buildTGMacroRegion(TGModel& tg_model);
  void generateTGModel(TGModel& tg_model);
  void routeTGTask(TGModel& tg_model, TGNet* tg_net);
  void initSingleTask(TGModel& tg_model, TGNet* tg_net);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentList(TGModel& tg_model);
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
  TGShadowDemandMap initTGShadowDemandMap(TGModel& tg_model);
  bool isBetterCandidate(TGModel& tg_model, TGCandidate& candidate, TGCandidate& current_best);
  std::vector<TGCandidate> getTGCandidateListByTopo(TGModel& tg_model, int32_t topo_idx, Segment<PlanarCoord>& planar_topo,
                                                    const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
                                                    const TGShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<TGCandidate> getTGCandidateList(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& planar_topo_list);
  std::vector<Segment<PlanarCoord>> getPlanarTopoList(TGModel& tg_model);
  std::vector<Segment<PlanarCoord>> legalizePlanarTopoByMacro(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& raw_topo_list);
  std::set<PlanarCoord, CmpPlanarCoordByXASC> getCurrTerminalCoordSet(TGModel& tg_model);
  PlanarCoord getNearestLegalMacroBoundaryCoord(TGModel& tg_model, PlanarCoord coord);
  bool isMacroForbiddenCoord(TGModel& tg_model, const PlanarCoord& coord);
  bool isSameMacroBodyCoord(TGModel& tg_model, const PlanarCoord& first_coord, const PlanarCoord& second_coord);
  int32_t getTGMacroRegionId(TGModel& tg_model, const PlanarCoord& coord);
  bool isMacroBlockedSegment(TGModel& tg_model, Segment<PlanarCoord>& planar_segment,
                             const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set);
  bool isMacroBlockedRoutingSegmentList(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& routing_segment_list,
                                        const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set);
  struct TGAStarEscapeNode
  {
    PlanarCoord terminal_coord;
    PlanarCoord route_coord;
    std::vector<Segment<PlanarCoord>> stub_segment_list;
    double cost = 0;
  };
  std::vector<TGAStarEscapeNode> getAStarEscapeNodeList(TGModel& tg_model, const PlanarCoord& terminal_coord,
                                                        const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
                                                        const TGShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByAStarWithEscape(
      TGModel& tg_model, Segment<PlanarCoord>& planar_topo,
      const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
      const TGShadowDemandMap* shadow_demand_map = nullptr);
  double getRoutingSegmentListScore(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& routing_segment_list,
                                    const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
                                    const TGShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByAStar(TGModel& tg_model, Segment<PlanarCoord>& planar_topo,
                                                                 const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
                                                                 const TGShadowDemandMap* shadow_demand_map = nullptr);
  PlanarRect getAStarSearchRect(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  bool isAStarAccessibleCoord(TGModel& tg_model, const PlanarCoord& coord, Segment<PlanarCoord>& planar_topo,
                              const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set);
  double getAStarStepCost(TGModel& tg_model, const PlanarCoord& start_coord, const PlanarCoord& end_coord,
                          const PlanarCoord& parent_coord, const TGShadowDemandMap* shadow_demand_map);
  double getAStarEstimateCost(TGModel& tg_model, const PlanarCoord& start_coord, const PlanarCoord& end_coord);
  std::vector<Segment<PlanarCoord>> getRoutingSegmentListByCoordList(std::vector<PlanarCoord>& coord_list);
  bool isLongObliqueTopo(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByStraight(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByZPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<int32_t> getMidIndexList(int32_t first_idx, int32_t second_idx);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByUPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByInner3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByLowCostLane3Bends(
      TGModel& tg_model, Segment<PlanarCoord>& planar_topo,
      const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
      const TGShadowDemandMap* shadow_demand_map = nullptr);
  double getPatternSegmentFastScore(TGModel& tg_model, Segment<PlanarCoord>& segment,
                                    const std::set<PlanarCoord, CmpPlanarCoordByXASC>& terminal_coord_set,
                                    const TGShadowDemandMap* shadow_demand_map = nullptr);
  std::vector<std::vector<Segment<PlanarCoord>>> getRoutingSegmentListByOuter3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo);
  void updateTGCandidate(TGModel& tg_model, TGCandidate& tg_candidate,
                         const TGShadowDemandMap* shadow_demand_map = nullptr);
  MTree<PlanarCoord> getCoordTree(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& routing_segment_list);
  void uploadNetResult(TGModel& tg_model, MTree<PlanarCoord>& coord_tree);
  void resetSingleTask(TGModel& tg_model);

#if 1  // update env
  void updateDemandToGraph(TGModel& tg_model, ChangeType change_type, MTree<PlanarCoord>& coord_tree);
  void addCandidateToShadow(TGShadowDemandMap& shadow_map, TGCandidate& tg_candidate);
#endif

#if 1  // exhibit
  void updateSummary(TGModel& tg_model);
  void printSummary(TGModel& tg_model);
  void outputGuide(TGModel& tg_model);
  void outputNetCSV(TGModel& tg_model);
  void outputOverflowCSV(TGModel& tg_model);
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
