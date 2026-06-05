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
// INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include <chrono>
#include <sstream>

#include "ChangeType.hpp"
#include "DataManager.hpp"
#include "Database.hpp"
#include "RTHeader.hpp"
#include "GSRModel.hpp"

namespace irt {

#define RTGSR (irt::GlobalSpatialRouter::getInst())

class GlobalSpatialRouter
{
 public:
  static void initInst();
  static GlobalSpatialRouter& getInst();
  static void destroyInst();
  // function
  void route();

 private:
  struct GSRRouteStats
  {
    int32_t total_net_num = 0;
    int32_t task_net_num = 0;
    int32_t routed_net_num = 0;
    int32_t skipped_net_num = 0;
    int32_t invalid_access_point_num = 0;
    int32_t invalid_segment_num = 0;
    int32_t cleared_segment_num = 0;
    int32_t uploaded_segment_num = 0;
    int32_t same_layer_segment_num = 0;
    int32_t via_segment_num = 0;
    int32_t fallback_topology_net_num = 0;
    int32_t net_without_ta_visible_segment_num = 0;
    int32_t reroute_iter_num = 0;
    int32_t reroute_task_num = 0;
    int32_t reroute_accept_num = 0;
    int32_t reroute_reject_num = 0;
    int32_t maze_route_num = 0;
    int32_t maze_fail_num = 0;
    int32_t stage2_detour_route_num = 0;
    int32_t stage2_detour_accept_num = 0;
    int32_t stage3_sparse_maze_route_num = 0;
    int32_t stage3_sparse_maze_success_num = 0;
    int32_t detour_candidate_num = 0;
    int32_t pattern_refine_success_num = 0;
    int32_t stage2_task_num = 0;
    int32_t stage3_task_num = 0;
    int32_t canonical_tree_num = 0;
    int32_t canonical_removed_edge_num = 0;
    int32_t canonical_removed_node_num = 0;
    int32_t canonical_cycle_break_num = 0;
    int32_t canonical_degree2_merge_num = 0;
    int32_t tree_layer_dp_route_num = 0;
    int32_t tree_layer_dp_success_num = 0;
    int32_t tree_layer_dp_fail_num = 0;
    int32_t tree_layer_dp_state_num = 0;
    int32_t tree_layer_dp_edge_candidate_num = 0;
    int32_t total_overflow_accept_num = 0;
    int32_t touched_overflow_accept_num = 0;
    int32_t congestion_risk_accept_num = 0;
    int32_t route_cost_accept_num = 0;
    int32_t sparse_hotspot_line_num = 0;
    int32_t sparse_offset_line_num = 0;
    int32_t ta_visible_net_num = 0;
    int32_t via_only_net_num = 0;
    int32_t same_layer_guide_segment_num = 0;
    int32_t guide_endpoint_on_track_num = 0;
    int32_t guide_endpoint_off_track_num = 0;
    int32_t non_preferred_reject_num = 0;
    int32_t wire_dirty_h_edge_num = 0;
    int32_t wire_dirty_v_edge_num = 0;
    int32_t wire_dirty_h_row_num = 0;
    int32_t wire_dirty_v_col_num = 0;
    int32_t overflow_net_num = 0;
    int32_t selected_overflow_net_num = 0;
    int32_t selected_hotspot_num = 0;
    int32_t skipped_max_routed_times_num = 0;
    int32_t stage2_coverage_cell_num = 0;
    int32_t stage3_coverage_cell_num = 0;
    double stage2_coverage_ratio = 0;
    double stage3_coverage_ratio = 0;
    double congestion_view_h_risk_sum = 0;
    double congestion_view_v_risk_sum = 0;
    double congestion_view_hotspot_sum = 0;
  };
  struct GSRLayerUsage
  {
    int32_t same_layer_segment_num = 0;
    int32_t via_touch_num = 0;
    int64_t wire_grid_length = 0;
  };
  struct GSRCongestionView
  {
    GridMap<double> h_overflow_map;
    GridMap<double> v_overflow_map;
    GridMap<double> h_risk_map;
    GridMap<double> v_risk_map;
    GridMap<double> h_near_full_map;
    GridMap<double> v_near_full_map;
    GridMap<double> internal_overflow_map;
    GridMap<double> via_risk_map;
    GridMap<double> capacity_pressure_map;
    GridMap<double> capacity_block_map;
    GridMap<double> hotspot_map;
    GridMap<double> h_risk_prefix_sum_map;
    GridMap<double> v_risk_prefix_sum_map;
    GridMap<double> h_capacity_prefix_sum_map;
    GridMap<double> v_capacity_prefix_sum_map;
    GridMap<double> h_block_prefix_sum_map;
    GridMap<double> v_block_prefix_sum_map;
    std::vector<PlanarCoord> task_coord_list;
    int32_t overflow_cell_num = 0;
    int32_t hotspot_cell_num = 0;
    double total_h_risk = 0;
    double total_v_risk = 0;
    double total_hotspot = 0;
  };
  struct GSRRerouteTask
  {
    GSRNet* gsr_net = nullptr;
    std::set<PlanarCoord, CmpPlanarCoordByXASC> overflow_coord_set;
    double overflow_score = 0;
    double risk_score = 0;
    double near_full_score = 0;
    double hotspot_score = 0;
    double total_score = 0;
    int32_t overflow_touch_num = 0;
    int32_t hotspot_touch_num = 0;
    int32_t routed_times = 0;
  };
  struct GSRWireCostView
  {
    GridMap<double> h_cost_map;
    GridMap<double> v_cost_map;
    GridMap<double> h_prefix_sum_map;
    GridMap<double> v_prefix_sum_map;
  };
  struct GSRLocalRouteEval
  {
    double new_total_overflow = 0;
    double new_total_congestion_risk = 0;
    double old_touched_overflow = 0;
    double new_touched_overflow = 0;
    double new_route_overflow = 0;
    double new_route_congestion_risk = 0;
    double new_route_cost = 0;
  };
  struct GSRRouteSnapshot
  {
    std::vector<Segment<LayerCoord>> old_segment_list;
    GSRTree old_tree;
    double old_total_overflow = 0;
    double old_total_congestion_risk = 0;
    double old_route_overflow = 0;
    double old_route_congestion_risk = 0;
    double old_route_cost = 0;
  };
  struct GSRUsageCapacityEval
  {
    bool internal_hard_blocked = false;
    int32_t via_side_hard_block_num = 0;
    double internal_overflow = 0;
    double via_side_overflow = 0;
  };
  struct GSRRerouteAttemptRecord
  {
    int32_t iter = -1;
    int32_t stage = -1;
    int32_t net_idx = -1;
    int32_t old_segment_num = 0;
    int32_t new_segment_num = 0;
    double old_total_overflow = 0;
    double new_total_overflow = 0;
    double old_touched_overflow = 0;
    double new_touched_overflow = 0;
    double old_route_overflow = 0;
    double new_route_overflow = 0;
    double old_total_congestion_risk = 0;
    double new_total_congestion_risk = 0;
    double old_route_congestion_risk = 0;
    double new_route_congestion_risk = 0;
    double old_route_cost = 0;
    double new_route_cost = 0;
    double runtime_ms = 0;
    std::string result = "unknown";
    std::string accept_reason = "none";
  };
  struct GSRRerouteTimingRecord
  {
    int32_t iter = -1;
    int32_t stage = -1;
    std::string step;
    double time_ms = 0;
    int32_t count = 0;
  };
  struct GSRPatternCandidate
  {
    std::vector<PlanarCoord> coord_list;
  };
  // self
  static GlobalSpatialRouter* _gsr_instance;
  std::vector<GSRRerouteTimingRecord> _reroute_timing_buffer;
  std::ostringstream _reroute_attempt_buffer;
  std::string _reroute_attempt_buffer_path;

  GlobalSpatialRouter() = default;
  GlobalSpatialRouter(const GlobalSpatialRouter& other) = delete;
  GlobalSpatialRouter(GlobalSpatialRouter&& other) = delete;
  ~GlobalSpatialRouter() = default;
  GlobalSpatialRouter& operator=(const GlobalSpatialRouter& other) = delete;
  GlobalSpatialRouter& operator=(GlobalSpatialRouter&& other) = delete;
  // function
  GSRModel initGSRModel(GSRRouteStats& route_stats);
  GSRNet convertToGSRNet(GSRModel& gsr_model, Net& net, GSRRouteStats& route_stats);
  void setGSRComParam(GSRModel& gsr_model);
  GSRComParam buildGSRComParam();
  int32_t getLowestPreferLayerIdx(const int32_t bottom_routing_layer_idx, const int32_t top_routing_layer_idx, const bool prefer_h);
  void buildGSRGridGraph(GSRModel& gsr_model);
  void clearGlobalResult(GSRRouteStats& route_stats);
  void routeGSRModel(GSRModel& gsr_model, GSRRouteStats& route_stats);
  void routeGSRNet(GSRModel& gsr_model, GSRNet& gsr_net, GSRRouteStats& route_stats);
  void rerouteGSRModel(GSRModel& gsr_model, GSRRouteStats& route_stats);
  std::vector<GSRNet*> getRerouteTaskList(GSRModel& gsr_model, GSRCongestionView& congestion_view, const int32_t iter,
                                         const bool prefer_uncovered, GSRRouteStats& route_stats, const bool stage3);
  bool tryDetourNet(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView& congestion_view, GSRRouteStats& route_stats,
                    GSRRerouteAttemptRecord* attempt_record = nullptr);
  bool trySparseMazeNet(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView& congestion_view, GSRWireCostView& wire_cost_view,
                        GSRRouteStats& route_stats, GSRRerouteAttemptRecord* attempt_record = nullptr);
  GSRCongestionView extractCongestionView(GSRModel& gsr_model);
  void addCongestionRisk(GridMap<double>& risk_map, const int32_t x, const int32_t y, const double value, const int32_t risk_radius);
  void rebuildCongestionRiskPrefix(GSRCongestionView& congestion_view);
  void rebuildCapacityPrefix(GSRCongestionView& congestion_view);
  double queryLinePrefixSum(const GridMap<double>& h_prefix_sum_map, const GridMap<double>& v_prefix_sum_map, const Direction direction,
                            const PlanarCoord& first_coord, const PlanarCoord& second_coord, const bool include_end,
                            const double invalid_value);
  double getCongestionRisk(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& coord);
  double getCongestionRiskSum(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& first_coord,
                              const PlanarCoord& second_coord);
  double getCapacityPressure(GSRCongestionView& congestion_view, const PlanarCoord& coord);
  double getCapacityBlock(GSRCongestionView& congestion_view, const PlanarCoord& coord);
  double getCapacityPressureSum(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& first_coord,
                                const PlanarCoord& second_coord);
  double getCapacityBlockSum(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& first_coord,
                             const PlanarCoord& second_coord);
  GSRWireCostView extractWireCostView(GSRModel& gsr_model);
  void rebuildWireCostPrefix(GSRWireCostView& wire_cost_view);
  void updateWireCostView(GSRModel& gsr_model, GSRWireCostView& wire_cost_view, const std::vector<Segment<LayerCoord>>& segment_list,
                          GSRRouteStats* route_stats = nullptr);
  double getWireCost(GSRWireCostView& wire_cost_view, const Direction direction, const PlanarCoord& first_coord, const PlanarCoord& second_coord);
  std::vector<Segment<LayerCoord>> routeByPattern(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView* congestion_view, const bool enable_detour,
                                                  GSRRouteStats& route_stats);
  std::vector<Segment<LayerCoord>> routeByMaze(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView& congestion_view,
                                               GSRWireCostView& wire_cost_view, GSRRouteStats& route_stats);
  std::vector<Segment<PlanarCoord>> getPlanarTopoList(GSRNet& gsr_net, GSRRouteStats& route_stats);
  GSRTree buildGSRTree(GSRModel& gsr_model, GSRNet& gsr_net, std::vector<Segment<PlanarCoord>>& planar_topo_list, GSRRouteStats& route_stats);
  GSRTree buildGSRTreeFromSegmentList(GSRModel& gsr_model, GSRNet& gsr_net, std::vector<Segment<LayerCoord>>& segment_list, GSRRouteStats& route_stats);
  GSRTree canonicalizeGSRTree(GSRModel& gsr_model, GSRNet& gsr_net, GSRTree& gsr_tree, GSRRouteStats& route_stats);
  std::vector<Segment<PlanarCoord>> getPlanarSegmentList(GSRTree& gsr_tree);
  std::vector<GSRPatternCandidate> buildPatternCandidateList(GSRModel& gsr_model, const PlanarCoord& first_coord, const PlanarCoord& second_coord,
                                                            GSRCongestionView* congestion_view, const bool enable_detour,
                                                            GSRRouteStats& route_stats);
  std::vector<Segment<LayerCoord>> refineTreeByPatternDAG(GSRModel& gsr_model, GSRNet& gsr_net, GSRTree& gsr_tree, GSRCongestionView* congestion_view,
                                                          const bool enable_detour, GSRRouteStats& route_stats);
  std::vector<Segment<LayerCoord>> buildCandidateSegmentList(const std::vector<PlanarCoord>& coord_list, const int32_t h_layer_idx,
                                                             const int32_t v_layer_idx);
  bool hasCongestion(GSRCongestionView& congestion_view, const std::vector<PlanarCoord>& coord_list, const double threshold);
  std::vector<Orientation> getViaSideOrientList(const GSRNode& gsr_node);
  GSRUsageCapacityEval evalUsageCapacity(const GSRModel& gsr_model, const GSRNode& gsr_node, const int32_t net_idx,
                                         const std::set<Orientation>& orient_set);
  bool isCapacitySafeRoute(GSRModel& gsr_model, const int32_t net_idx, const std::vector<Segment<LayerCoord>>& segment_list,
                           const bool allow_soft_overflow);
  double getRouteCapacityPenalty(GSRModel& gsr_model, const int32_t net_idx, const std::vector<Segment<LayerCoord>>& segment_list);
  bool isRouteConnected(GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& segment_list);
  std::vector<Segment<LayerCoord>> getRoutingSegmentList(GSRModel& gsr_model, GSRNet& gsr_net, std::vector<Segment<PlanarCoord>>& planar_topo_list,
                                                         GSRRouteStats& route_stats);
  std::vector<Segment<LayerCoord>> getValidUniqueSegmentList(GSRModel& gsr_model, std::vector<Segment<LayerCoord>>& routing_segment_list,
                                                             GSRRouteStats& route_stats);
  void uploadNetResult(GSRNet& gsr_net, std::vector<Segment<LayerCoord>>& routing_segment_list, GSRRouteStats& route_stats);
  void uploadGSRModelResult(GSRModel& gsr_model, GSRRouteStats& route_stats);
  void addRouteDemand(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& routing_segment_list);
  void removeRouteDemand(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& routing_segment_list);
  void updateGSRModelCost(GSRModel& gsr_model);
  void updateGSRNetCost(GSRModel& gsr_model, GSRNet& gsr_net);
  void updateBestResult(GSRModel& gsr_model);
  void selectBestResult(GSRModel& gsr_model);
  GSRRouteSnapshot snapshotRoute(GSRModel& gsr_model, GSRNet& gsr_net);
  void initRerouteAttemptRecord(const GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot, GSRRerouteAttemptRecord* attempt_record);
  void removeSnapshotRoute(GSRModel& gsr_model, GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot, GSRWireCostView* wire_cost_view,
                           GSRRouteStats* route_stats);
  void restoreSnapshotRoute(GSRModel& gsr_model, GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot, GSRWireCostView* wire_cost_view,
                            GSRRouteStats* route_stats);
  bool tryCommitCandidateRoute(GSRModel& gsr_model, GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot,
                               std::vector<Segment<LayerCoord>>& candidate_segment_list, GSRRouteStats& route_stats,
                               GSRRerouteAttemptRecord* attempt_record, const int32_t stage, const std::string& empty_result,
                               const std::chrono::steady_clock::time_point& attempt_start_time,
                               GSRWireCostView* wire_cost_view = nullptr);
  GSRLocalRouteEval evalCandidateRouteByLocalDelta(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& old_segment_list,
                                                  const std::vector<Segment<LayerCoord>>& new_segment_list, const double old_total_overflow,
                                                  const double old_total_congestion_risk, const double old_route_congestion_risk);
  bool acceptNewRoute(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& old_segment_list,
                      const std::vector<Segment<LayerCoord>>& new_segment_list, const double old_total_overflow, const double new_total_overflow,
                      const double old_total_congestion_risk, const double new_total_congestion_risk, const double old_touched_overflow,
                      const double new_touched_overflow,
                      const double old_route_cost, const double new_route_cost, GSRRouteStats& route_stats, std::string* accept_reason = nullptr);
  std::vector<int32_t> getCandidateLayerList(const GSRComParam& gsr_com_param, const bool prefer_h);
  std::vector<Segment<LayerCoord>> buildPatternRoute(const PlanarCoord& first_coord, const PlanarCoord& second_coord, const bool h_first,
                                                     const int32_t h_layer_idx, const int32_t v_layer_idx);
  bool isRoutePreferredOnly(GSRModel& gsr_model, const std::vector<Segment<LayerCoord>>& routing_segment_list);
  int64_t getGSRNetHPWL(const GSRNet& gsr_net);
  std::map<int32_t, GSRLayerUsage> getLayerUsageMap(std::vector<RoutingLayer>& routing_layer_list, GSRGridGraph& gsr_grid_graph,
                                                   GSRRouteStats& route_stats);
  void updateHandoffStats(GSRModel& gsr_model, GSRRouteStats& route_stats);
  void initRerouteDiagnostics();
  void initRerouteAttemptCSV(const int32_t iter, const int32_t stage);
  void flushRerouteTimingCSV();
  void appendRerouteTiming(const int32_t iter, const int32_t stage, const std::string& step, const double time_ms, const int32_t count);
  void appendRerouteIterSummary(GSRModel& gsr_model, GSRCongestionView* congestion_view, const int32_t iter, const std::string& phase,
                                const double prev_total_overflow, const double prev_total_congestion_risk, const int32_t stage2_task_num,
                                const int32_t stage2_accept_num, const int32_t stage3_task_num, const int32_t stage3_accept_num);
  void outputOverflowHotspotCSV(GSRModel& gsr_model, GSRCongestionView& congestion_view, const int32_t iter, const std::string& phase);
  void outputRerouteTaskCSV(const std::vector<GSRRerouteTask>& task_list, const std::set<int32_t>& selected_net_idx_set, const int32_t iter,
                            const int32_t stage);
  void flushRerouteAttemptCSV();
  void outputRerouteAttemptCSV(const GSRRerouteAttemptRecord& attempt_record);
  void outputSummaryCSV(GSRModel& gsr_model, GSRRouteStats& route_stats);
  bool isValidAccessCoord(GSRModel& gsr_model, const LayerCoord& access_coord);
  bool isValidSegment(GSRModel& gsr_model, Segment<LayerCoord>& segment);
};

}  // namespace irt
