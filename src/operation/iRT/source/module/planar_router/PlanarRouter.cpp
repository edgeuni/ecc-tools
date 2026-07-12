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
#include "PlanarRouter.hpp"

#include "GDSPlotter.hpp"
#include "PRCandidate.hpp"
#include "RTInterface.hpp"
#include "TOPOBuilder.hpp"
#include "Utility.hpp"

namespace irt {

namespace {

struct PRRouteMetrics
{
  double overflow = 0;
  double overflow_cost = 0;
  double congestion_risk = 0;
  double high_usage = 0;
  double max_usage_ratio = 0;
  int32_t wire_length = 0;
  int32_t corner_num = 0;
  int32_t segment_num = 0;
};

struct PRLocalRerouteMetrics
{
  double overflow = 0;
  double overflow_cost = 0;
  double high_usage = 0;
  double max_usage_ratio = 0;
};

struct PRNetTask
{
  int32_t net_idx = -1;
  double overflow = 0;
  double high_usage = 0;
  double congestion_risk = 0;
  double max_usage_ratio = 0;
  double wire_length = 0;
  int32_t overflow_segment_num = 0;
};

bool isSamePlanarSegment(const Segment<PlanarCoord>& lhs, const Segment<PlanarCoord>& rhs)
{
  return ((lhs.get_first() == rhs.get_first() && lhs.get_second() == rhs.get_second())
          || (lhs.get_first() == rhs.get_second() && lhs.get_second() == rhs.get_first()));
}

bool isSamePlanarSegment(const Segment<LayerCoord>& lhs, const Segment<PlanarCoord>& rhs)
{
  PlanarCoord lhs_first = lhs.get_first().get_planar_coord();
  PlanarCoord lhs_second = lhs.get_second().get_planar_coord();
  return ((lhs_first == rhs.get_first() && lhs_second == rhs.get_second()) || (lhs_first == rhs.get_second() && lhs_second == rhs.get_first()));
}

Segment<LayerCoord>* getCurrentGlobalSegment(int32_t net_idx, Segment<PlanarCoord>& planar_segment)
{
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  PlanarCoord& first_coord = planar_segment.get_first();
  if (!gcell_map.isInside(first_coord.get_x(), first_coord.get_y())) {
    return nullptr;
  }
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map = gcell_map[first_coord.get_x()][first_coord.get_y()].get_net_global_result_map();
  if (!RTUTIL.exist(net_global_result_map, net_idx)) {
    return nullptr;
  }
  for (Segment<LayerCoord>* segment : net_global_result_map[net_idx]) {
    if (segment != nullptr && isSamePlanarSegment(*segment, planar_segment)) {
      return segment;
    }
  }
  return nullptr;
}

bool isSameRoutingResult(PRSegmentTask& pr_segment_task, std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  return routing_segment_list.size() == 1 && isSamePlanarSegment(routing_segment_list.front(), pr_segment_task.get_planar_segment());
}

void appendSegmentCoord(std::vector<PlanarCoord>& coord_list, Segment<PlanarCoord>& segment)
{
  if (segment.get_first() == segment.get_second()) {
    return;
  }
  if (coord_list.empty()) {
    coord_list.push_back(segment.get_first());
    coord_list.push_back(segment.get_second());
  } else if (coord_list.back() == segment.get_first()) {
    coord_list.push_back(segment.get_second());
  } else if (coord_list.back() == segment.get_second()) {
    coord_list.push_back(segment.get_first());
  } else if (coord_list.front() == segment.get_second()) {
    coord_list.insert(coord_list.begin(), segment.get_first());
  } else if (coord_list.front() == segment.get_first()) {
    coord_list.insert(coord_list.begin(), segment.get_second());
  } else {
    coord_list.push_back(segment.get_first());
    coord_list.push_back(segment.get_second());
  }
}

std::vector<PlanarCoord> getCoordList(std::vector<Segment<PlanarCoord>>& segment_list)
{
  std::vector<PlanarCoord> coord_list;
  for (Segment<PlanarCoord>& segment : segment_list) {
    appendSegmentCoord(coord_list, segment);
  }
  return coord_list;
}

void removeCoordLoop(std::vector<PlanarCoord>& coord_list)
{
  std::map<PlanarCoord, size_t, CmpPlanarCoordByXASC> coord_idx_map;
  std::vector<PlanarCoord> loop_free_coord_list;
  for (PlanarCoord& coord : coord_list) {
    auto iter = coord_idx_map.find(coord);
    if (iter != coord_idx_map.end()) {
      size_t keep_idx = iter->second;
      for (size_t i = keep_idx + 1; i < loop_free_coord_list.size(); i++) {
        coord_idx_map.erase(loop_free_coord_list[i]);
      }
      loop_free_coord_list.resize(keep_idx + 1);
    } else {
      coord_idx_map[coord] = loop_free_coord_list.size();
      loop_free_coord_list.push_back(coord);
    }
  }
  coord_list = loop_free_coord_list;
}

std::vector<Segment<PlanarCoord>> getMergedSegmentList(std::vector<PlanarCoord>& coord_list)
{
  std::vector<Segment<PlanarCoord>> segment_list;
  if (coord_list.size() < 2) {
    return segment_list;
  }

  PlanarCoord segment_start = coord_list.front();
  Direction curr_direction = Direction::kNone;
  for (size_t i = 1; i < coord_list.size(); i++) {
    Direction next_direction = RTUTIL.getDirection(coord_list[i - 1], coord_list[i]);
    if (next_direction == Direction::kProximal) {
      continue;
    }
    if (next_direction == Direction::kOblique) {
      RTLOG.error(Loc::current(), "The direction is error!");
    }
    if (curr_direction == Direction::kNone) {
      curr_direction = next_direction;
      continue;
    }
    if (curr_direction != next_direction) {
      segment_list.emplace_back(segment_start, coord_list[i - 1]);
      segment_start = coord_list[i - 1];
      curr_direction = next_direction;
    }
  }
  if (segment_start != coord_list.back()) {
    segment_list.emplace_back(segment_start, coord_list.back());
  }
  return segment_list;
}

std::vector<Segment<PlanarCoord>> simplifyRoutingSegmentList(std::vector<Segment<PlanarCoord>>& segment_list)
{
  std::vector<PlanarCoord> coord_list = getCoordList(segment_list);
  removeCoordLoop(coord_list);
  return getMergedSegmentList(coord_list);
}

std::vector<Segment<PlanarCoord>> getPlanarSegmentList(std::vector<Segment<LayerCoord>*>& layer_segment_list)
{
  std::vector<Segment<PlanarCoord>> planar_segment_list;
  planar_segment_list.reserve(layer_segment_list.size());
  for (Segment<LayerCoord>* layer_segment : layer_segment_list) {
    if (layer_segment == nullptr) {
      continue;
    }
    planar_segment_list.emplace_back(layer_segment->get_first().get_planar_coord(), layer_segment->get_second().get_planar_coord());
  }
  return planar_segment_list;
}

std::vector<Segment<PlanarCoord>> getPlanarSegmentList(MTree<PlanarCoord>& coord_tree)
{
  std::vector<Segment<PlanarCoord>> planar_segment_list;
  for (Segment<TNode<PlanarCoord>*>& coord_segment : RTUTIL.getSegListByTree(coord_tree)) {
    planar_segment_list.emplace_back(coord_segment.get_first()->value(), coord_segment.get_second()->value());
  }
  return planar_segment_list;
}

std::vector<Segment<LayerCoord>*> getNetGlobalSegmentPtrList(PRModel& pr_model, int32_t net_idx)
{
  std::vector<Segment<LayerCoord>*> segment_ptr_list;
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map = pr_model.get_net_global_result_map();
  if (!RTUTIL.exist(net_global_result_map, net_idx)) {
    return segment_ptr_list;
  }
  segment_ptr_list.reserve(net_global_result_map[net_idx].size());
  for (Segment<LayerCoord>* segment : net_global_result_map[net_idx]) {
    if (segment != nullptr) {
      segment_ptr_list.push_back(segment);
    }
  }
  return segment_ptr_list;
}

PRRouteMetrics getRouteMetrics(PRModel& pr_model, int32_t net_idx, std::vector<Segment<PlanarCoord>>& segment_list)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  GridMap<double>& congestion_risk_map = pr_model.get_congestion_risk_map();
  double overflow_unit = pr_model.get_pr_iter_param().get_overflow_unit();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();

  PRRouteMetrics metrics;
  metrics.segment_num = static_cast<int32_t>(segment_list.size());

  Direction pre_direction = Direction::kNone;
  for (Segment<PlanarCoord>& segment : segment_list) {
    PlanarCoord& first_coord = segment.get_first();
    PlanarCoord& second_coord = segment.get_second();
    Direction direction = RTUTIL.getDirection(first_coord, second_coord);
    if (direction == Direction::kProximal) {
      continue;
    }
    if (direction == Direction::kOblique) {
      RTLOG.error(Loc::current(), "The direction is error!");
    }
    if (pre_direction != Direction::kNone && pre_direction != direction) {
      metrics.corner_num++;
    }
    pre_direction = direction;
    metrics.wire_length += RTUTIL.getManhattanDistance(first_coord, second_coord);

    int32_t first_x = first_coord.get_x();
    int32_t second_x = second_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t second_y = second_coord.get_y();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);
    uint8_t direction_mask = getPRDirectionMask(direction);
    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        if (!pr_node_map.isInside(x, y)) {
          continue;
        }
        PRNodeCost node_cost = pr_node_map[x][y].getFastCost(net_idx, direction_mask, overflow_unit, true);
        metrics.overflow += node_cost.overflow;
        metrics.overflow_cost += node_cost.overflow_cost;
        metrics.high_usage += std::max(0.0, node_cost.max_usage_ratio - high_usage_ratio_threshold);
        metrics.max_usage_ratio = std::max(metrics.max_usage_ratio, node_cost.max_usage_ratio);
        if (!congestion_risk_map.empty() && congestion_risk_map.isInside(x, y)) {
          metrics.congestion_risk += congestion_risk_map[x][y];
        }
      }
    }
  }
  return metrics;
}

void collectSegmentCoordSet(std::vector<Segment<PlanarCoord>>& segment_list, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  for (Segment<PlanarCoord>& segment : segment_list) {
    PlanarCoord& first_coord = segment.get_first();
    PlanarCoord& second_coord = segment.get_second();
    int32_t first_x = first_coord.get_x();
    int32_t second_x = second_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t second_y = second_coord.get_y();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);
    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        coord_set.insert(PlanarCoord(x, y));
      }
    }
  }
}

double getCoordSetOverflow(PRModel& pr_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double overflow = 0;
  for (PlanarCoord coord : coord_set) {
    if (pr_node_map.isInside(coord.get_x(), coord.get_y())) {
      overflow += pr_node_map[coord.get_x()][coord.get_y()].getOverflow();
    }
  }
  return overflow;
}

double getCoordSetHighUsage(PRModel& pr_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();
  double high_usage = 0;
  for (PlanarCoord coord : coord_set) {
    if (pr_node_map.isInside(coord.get_x(), coord.get_y())) {
      high_usage += pr_node_map[coord.get_x()][coord.get_y()].getHighUsage(high_usage_ratio_threshold);
    }
  }
  return high_usage;
}

PRLocalRerouteMetrics getCoordSetLocalMetrics(PRModel& pr_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double overflow_unit = pr_model.get_pr_iter_param().get_overflow_unit();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();

  PRLocalRerouteMetrics metrics;
  for (PlanarCoord coord : coord_set) {
    if (!pr_node_map.isInside(coord.get_x(), coord.get_y())) {
      continue;
    }
    PRNode& pr_node = pr_node_map[coord.get_x()][coord.get_y()];
    PRNodeCost node_cost = pr_node.getFastCost(kPRMaskNone, overflow_unit);
    metrics.overflow += pr_node.getOverflow();
    metrics.overflow_cost += node_cost.overflow_cost;
    metrics.high_usage += pr_node.getHighUsage(high_usage_ratio_threshold);
    metrics.max_usage_ratio = std::max(metrics.max_usage_ratio, pr_node.getMaxUsageRatio());
  }
  return metrics;
}

bool passRerouteShapeGuard(PRRouteMetrics& old_metrics, PRRouteMetrics& new_metrics)
{
  bool overflow_task = old_metrics.overflow > RT_ERROR;
  int32_t max_corner_num = overflow_task ? 8 : 6;
  int32_t max_segment_num = max_corner_num + 1;
  double max_wire_length = old_metrics.wire_length * (overflow_task ? 3.0 : 2.0) + 10;
  return new_metrics.corner_num <= max_corner_num && new_metrics.segment_num <= max_segment_num && new_metrics.wire_length <= max_wire_length;
}

bool acceptReroute(PRRouteMetrics& old_metrics, PRRouteMetrics& new_metrics)
{
  bool overflow_task = old_metrics.overflow > RT_ERROR;
  bool high_usage_task = old_metrics.high_usage > RT_ERROR;
  if (!passRerouteShapeGuard(old_metrics, new_metrics)) {
    return false;
  }
  if (overflow_task) {
    if (new_metrics.overflow < old_metrics.overflow && !RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR) && new_metrics.high_usage < old_metrics.high_usage
        && !RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR) && new_metrics.congestion_risk < old_metrics.congestion_risk
        && !RTUTIL.equalDoubleByError(new_metrics.congestion_risk, old_metrics.congestion_risk, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.congestion_risk, old_metrics.congestion_risk, RT_ERROR) && new_metrics.wire_length <= old_metrics.wire_length
        && new_metrics.corner_num <= old_metrics.corner_num) {
      return true;
    }
    return false;
  }

  if (new_metrics.overflow > old_metrics.overflow && !RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)) {
    return false;
  }
  if (high_usage_task) {
    if (new_metrics.high_usage < old_metrics.high_usage && !RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR) && new_metrics.max_usage_ratio < old_metrics.max_usage_ratio
        && !RTUTIL.equalDoubleByError(new_metrics.max_usage_ratio, old_metrics.max_usage_ratio, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.max_usage_ratio, old_metrics.max_usage_ratio, RT_ERROR)
        && new_metrics.congestion_risk < old_metrics.congestion_risk
        && !RTUTIL.equalDoubleByError(new_metrics.congestion_risk, old_metrics.congestion_risk, RT_ERROR)) {
      return true;
    }
    return false;
  }
  return false;
}

bool acceptTrueLocalReroute(PRRouteMetrics& old_route_metrics, PRRouteMetrics& new_route_metrics, PRLocalRerouteMetrics& old_local_metrics,
                            PRLocalRerouteMetrics& new_local_metrics)
{
  if (!passRerouteShapeGuard(old_route_metrics, new_route_metrics)) {
    return false;
  }
  if (new_local_metrics.overflow > old_local_metrics.overflow && !RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)) {
    return false;
  }

  bool overflow_task = old_route_metrics.overflow > RT_ERROR;
  bool high_usage_task = old_route_metrics.high_usage > RT_ERROR;
  if (overflow_task) {
    if (new_local_metrics.overflow < old_local_metrics.overflow
        && !RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
        && new_local_metrics.overflow_cost < old_local_metrics.overflow_cost
        && !RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
        && new_local_metrics.high_usage < old_local_metrics.high_usage
        && !RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
        && new_local_metrics.max_usage_ratio < old_local_metrics.max_usage_ratio
        && !RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)
        && new_route_metrics.congestion_risk < old_route_metrics.congestion_risk
        && !RTUTIL.equalDoubleByError(new_route_metrics.congestion_risk, old_route_metrics.congestion_risk, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_route_metrics.congestion_risk, old_route_metrics.congestion_risk, RT_ERROR)
        && new_route_metrics.wire_length <= old_route_metrics.wire_length && new_route_metrics.corner_num <= old_route_metrics.corner_num) {
      return true;
    }
    return false;
  }

  if (new_local_metrics.overflow > old_local_metrics.overflow && !RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)) {
    return false;
  }
  if (high_usage_task) {
    if (new_local_metrics.high_usage < old_local_metrics.high_usage
        && !RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
        && new_local_metrics.max_usage_ratio < old_local_metrics.max_usage_ratio
        && !RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)
        && new_route_metrics.congestion_risk < old_route_metrics.congestion_risk
        && !RTUTIL.equalDoubleByError(new_route_metrics.congestion_risk, old_route_metrics.congestion_risk, RT_ERROR)) {
      return true;
    }
  }
  return false;
}

bool passNetRerouteShapeGuard(PRRouteMetrics& old_metrics, PRRouteMetrics& new_metrics)
{
  double max_wire_length = old_metrics.wire_length * 3.0 + 20;
  int32_t max_segment_num = old_metrics.segment_num * 4 + 20;
  return new_metrics.wire_length <= max_wire_length && new_metrics.segment_num <= max_segment_num;
}

bool acceptNetReroute(PRRouteMetrics& old_route_metrics, PRRouteMetrics& new_route_metrics, PRLocalRerouteMetrics& old_local_metrics,
                      PRLocalRerouteMetrics& new_local_metrics)
{
  if (!passNetRerouteShapeGuard(old_route_metrics, new_route_metrics)) {
    return false;
  }
  if (old_local_metrics.overflow <= RT_ERROR && new_local_metrics.overflow > old_local_metrics.overflow
      && !RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)) {
    return false;
  }
  if (new_local_metrics.overflow < old_local_metrics.overflow && !RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)) {
    return true;
  }
  if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
      && new_local_metrics.overflow_cost < old_local_metrics.overflow_cost
      && !RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)) {
    return true;
  }
  if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
      && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
      && new_local_metrics.high_usage < old_local_metrics.high_usage
      && !RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)) {
    return true;
  }
  if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
      && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
      && RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
      && new_local_metrics.max_usage_ratio < old_local_metrics.max_usage_ratio
      && !RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)) {
    return true;
  }
  if (RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)
      && RTUTIL.equalDoubleByError(new_local_metrics.overflow_cost, old_local_metrics.overflow_cost, RT_ERROR)
      && RTUTIL.equalDoubleByError(new_local_metrics.high_usage, old_local_metrics.high_usage, RT_ERROR)
      && RTUTIL.equalDoubleByError(new_local_metrics.max_usage_ratio, old_local_metrics.max_usage_ratio, RT_ERROR)
      && new_route_metrics.congestion_risk < old_route_metrics.congestion_risk
      && !RTUTIL.equalDoubleByError(new_route_metrics.congestion_risk, old_route_metrics.congestion_risk, RT_ERROR)) {
    return true;
  }
  return false;
}

}  // namespace

// public

void PlanarRouter::initInst()
{
  if (_pr_instance == nullptr) {
    _pr_instance = new PlanarRouter();
  }
}

PlanarRouter& PlanarRouter::getInst()
{
  if (_pr_instance == nullptr) {
    RTLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pr_instance;
}

void PlanarRouter::destroyInst()
{
  if (_pr_instance != nullptr) {
    delete _pr_instance;
    _pr_instance = nullptr;
  }
}

// function

void PlanarRouter::generate()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");
  PRModel pr_model = initPRModel();
  setPRComParam(pr_model);
  initPRTaskList(pr_model);
  buildPRNodeMap(pr_model);
  buildPRNodeNeighbor(pr_model);
  buildOrientSupply(pr_model);
  // debugCheckPRModel(pr_model);
  generatePRModel(pr_model);
  setPRIterParam(pr_model);
  outputCongestionSnapshotCSV(pr_model, "_initial_pattern", 0);
  double curr_overflow = getOverflow(pr_model);
  double curr_high_usage = getHighUsage(pr_model);
  if (curr_overflow > RT_ERROR && curr_high_usage > RT_ERROR) {
    reroutePRModel(pr_model);
  } else {
    RTLOG.info(Loc::current(), "Skip reroutePRModel because overflow ", curr_overflow, ", high_usage ", curr_high_usage);
  }
  // debugPlotPRModel(pr_model, "after");
  updateSummary(pr_model);
  printSummary(pr_model);
  outputGuide(pr_model);
  outputNetCSV(pr_model);
  outputOverflowCSV(pr_model);
  outputCongestionCSV(pr_model);
  outputJson(pr_model);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

PlanarRouter* PlanarRouter::_pr_instance = nullptr;

PRModel PlanarRouter::initPRModel()
{
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();

  PRModel pr_model;
  pr_model.set_pr_net_list(convertToPRNetList(net_list));
  return pr_model;
}

std::vector<PRNet> PlanarRouter::convertToPRNetList(std::vector<Net>& net_list)
{
  std::vector<PRNet> pr_net_list;
  pr_net_list.reserve(net_list.size());
  for (size_t i = 0; i < net_list.size(); i++) {
    pr_net_list.emplace_back(convertToPRNet(net_list[i]));
  }
  return pr_net_list;
}

PRNet PlanarRouter::convertToPRNet(Net& net)
{
  PRNet pr_net;
  pr_net.set_origin_net(&net);
  pr_net.set_net_idx(net.get_net_idx());
  pr_net.set_connect_type(net.get_connect_type());
  for (Pin& pin : net.get_pin_list()) {
    pr_net.get_pr_pin_list().push_back(PRPin(pin));
  }
  pr_net.set_bounding_box(net.get_bounding_box());
  return pr_net;
}

void PlanarRouter::setPRComParam(PRModel& pr_model)
{
  int32_t topo_spilt_length = 30;
  int32_t expand_step_num = 30;
  int32_t expand_step_length = 1;
  double prefer_wire_unit = 1;
  double non_prefer_wire_unit = 2.5 * prefer_wire_unit;
  double overflow_unit = 4 * non_prefer_wire_unit;
  /**
   * topo_spilt_length, expand_step_num, expand_step_length, overflow_unit
   */
  double corner_weight = 0.3;

  PRComParam pr_com_param(topo_spilt_length, expand_step_num, expand_step_length, overflow_unit, corner_weight);
  RTLOG.info(Loc::current(), "topo_spilt_length: ", pr_com_param.get_topo_spilt_length());
  RTLOG.info(Loc::current(), "expand_step_num: ", pr_com_param.get_expand_step_num());
  RTLOG.info(Loc::current(), "expand_step_length: ", pr_com_param.get_expand_step_length());
  RTLOG.info(Loc::current(), "overflow_unit: ", pr_com_param.get_overflow_unit());
  RTLOG.info(Loc::current(), "corner_weight: ", pr_com_param.get_corner_weight());
  pr_model.set_pr_com_param(pr_com_param);
}

void PlanarRouter::initPRTaskList(PRModel& pr_model)
{
  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();
  std::vector<PRNet*>& pr_task_list = pr_model.get_pr_task_list();
  pr_task_list.reserve(pr_net_list.size());
  for (PRNet& pr_net : pr_net_list) {
    pr_task_list.push_back(&pr_net);
  }
  std::sort(pr_task_list.begin(), pr_task_list.end(), CmpPRNet());
}

void PlanarRouter::buildPRNodeMap(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  pr_node_map.init(gcell_map.get_x_size(), gcell_map.get_y_size());
#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < gcell_map.get_x_size(); x++) {
    for (int32_t y = 0; y < gcell_map.get_y_size(); y++) {
      PRNode& pr_node = pr_node_map[x][y];
      pr_node.set_coord(x, y);
      pr_node.set_boundary_wire_unit(gcell_map[x][y].get_boundary_wire_unit());
      pr_node.set_internal_wire_unit(gcell_map[x][y].get_internal_wire_unit());
      pr_node.set_internal_via_unit(gcell_map[x][y].get_internal_via_unit());
      for (auto& [routing_layer_idx, ignore_net_orient_map] : gcell_map[x][y].get_routing_ignore_net_orient_map()) {
        for (auto& [net_idx, orient_set] : ignore_net_orient_map) {
          pr_node.get_ignore_net_orient_map()[net_idx].insert(orient_set.begin(), orient_set.end());
        }
      }
    }
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::buildPRNodeNeighbor(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();

  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < gcell_map.get_x_size(); x++) {
    for (int32_t y = 0; y < gcell_map.get_y_size(); y++) {
      std::map<Orientation, PRNode*>& neighbor_node_map = pr_node_map[x][y].get_neighbor_node_map();
      if (x != 0) {
        neighbor_node_map[Orientation::kWest] = &pr_node_map[x - 1][y];
      }
      if (x != (pr_node_map.get_x_size() - 1)) {
        neighbor_node_map[Orientation::kEast] = &pr_node_map[x + 1][y];
      }
      if (y != 0) {
        neighbor_node_map[Orientation::kSouth] = &pr_node_map[x][y - 1];
      }
      if (y != (pr_node_map.get_y_size() - 1)) {
        neighbor_node_map[Orientation::kNorth] = &pr_node_map[x][y + 1];
      }
    }
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::buildOrientSupply(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();

#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < gcell_map.get_x_size(); x++) {
    for (int32_t y = 0; y < gcell_map.get_y_size(); y++) {
      std::map<Orientation, int32_t> planar_orient_supply_map;
      for (auto& [layer_idx, orient_supply_map] : gcell_map[x][y].get_routing_orient_supply_map()) {
        for (auto& [orient, supply] : orient_supply_map) {
          planar_orient_supply_map[orient] += supply;
        }
      }
      pr_node_map[x][y].set_orient_supply_map(planar_orient_supply_map);
    }
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::generatePRModel(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<PRNet*>& pr_task_list = pr_model.get_pr_task_list();

  int32_t batch_size = RTUTIL.getBatchSize(pr_task_list.size());

  Monitor stage_monitor;
  for (size_t i = 0; i < pr_task_list.size(); i++) {
    routePRTask(pr_model, pr_task_list[i]);
    if ((i + 1) % batch_size == 0 || (i + 1) == pr_task_list.size()) {
      RTLOG.info(Loc::current(), "Routed ", (i + 1), "/", pr_task_list.size(), "(", RTUTIL.getPercentage(i + 1, pr_task_list.size()), ") nets",
                 stage_monitor.getStatsInfo());
    }
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::routePRTask(PRModel& pr_model, PRNet* pr_task)
{
  initSingleTask(pr_model, pr_task);
  std::vector<Segment<PlanarCoord>> routing_segment_list = getRoutingSegmentList(pr_model);
  MTree<PlanarCoord> coord_tree = getCoordTree(pr_model, routing_segment_list);
  updateDemandToGraph(pr_model, ChangeType::kAdd, coord_tree);
  uploadNetResult(pr_model, coord_tree);
  resetSingleTask(pr_model);
}

void PlanarRouter::initSingleTask(PRModel& pr_model, PRNet* pr_task)
{
  pr_model.set_curr_pr_task(pr_task);
}

std::vector<Segment<PlanarCoord>> PlanarRouter::getRoutingSegmentList(PRModel& pr_model)
{
  std::vector<Segment<PlanarCoord>> planar_topo_list = getPlanarTopoList(pr_model);
  double corner_weight = pr_model.get_pr_com_param().get_corner_weight();
  PRShadowDemandMap self_shadow = initPRShadowDemandMap(pr_model);
  std::vector<Segment<PlanarCoord>> routing_segment_list;

  for (size_t topo_idx = 0; topo_idx < planar_topo_list.size(); topo_idx++) {
    const PRShadowDemandMap* shadow_ptr = self_shadow.empty() ? nullptr : &self_shadow;
    std::vector<PRCandidate> candidate_list = getPRCandidateListByTopo(pr_model, static_cast<int32_t>(topo_idx), planar_topo_list[topo_idx], shadow_ptr);
    if (candidate_list.empty()) {
      continue;
    }

#pragma omp parallel for
    for (PRCandidate& pr_candidate : candidate_list) {
      updatePRCandidate(pr_model, pr_candidate, shadow_ptr);
    }

    PRCandidate* best_candidate = nullptr;
    for (PRCandidate& pr_candidate : candidate_list) {
      if (best_candidate == nullptr || isBetterCandidate(pr_candidate, *best_candidate, corner_weight)) {
        best_candidate = &pr_candidate;
      }
    }
    if (best_candidate == nullptr) {
      continue;
    }
    for (Segment<PlanarCoord>& routing_segment : best_candidate->get_routing_segment_list()) {
      routing_segment_list.push_back(routing_segment);
    }
    addCandidateToShadow(self_shadow, *best_candidate);
  }
  return routing_segment_list;
}

std::vector<PRCandidate> PlanarRouter::getPRCandidateListByTopo(PRModel& pr_model, int32_t topo_idx, Segment<PlanarCoord>& planar_topo,
                                                                const PRShadowDemandMap* shadow_demand_map)
{
  std::vector<PRCandidate> pr_candidate_list;

  auto appendCandidateList = [&](int32_t corner_num, std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list) {
    for (const std::vector<Segment<PlanarCoord>>& routing_segment_list : routing_segment_list_list) {
      pr_candidate_list.emplace_back(topo_idx, routing_segment_list, corner_num, 0, false, 0);
    }
  };

  bool long_oblique_topo = isLongObliqueTopo(pr_model, planar_topo);
  if (!long_oblique_topo) {
    appendCandidateList(0, getRoutingSegmentListByStraight(pr_model, planar_topo));
  }
  appendCandidateList(1, getRoutingSegmentListByLPattern(pr_model, planar_topo));
  appendCandidateList(2, getRoutingSegmentListByZPattern(pr_model, planar_topo));
  if (long_oblique_topo) {
    appendCandidateList(3, getRoutingSegmentListByLowCostLane3Bends(pr_model, planar_topo, shadow_demand_map));
  } else {
    appendCandidateList(3, getRoutingSegmentListByInner3Bends(pr_model, planar_topo));
  }
  appendCandidateList(4, getRoutingSegmentListByUPattern(pr_model, planar_topo));
  appendCandidateList(5, getRoutingSegmentListByOuter3Bends(pr_model, planar_topo));

  return pr_candidate_list;
}

std::vector<PRCandidate> PlanarRouter::getPRCandidateList(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& planar_topo_list)
{
  std::vector<PRCandidate> pr_candidate_list;
  for (size_t i = 0; i < planar_topo_list.size(); i++) {
    std::vector<PRCandidate> topo_candidate_list = getPRCandidateListByTopo(pr_model, static_cast<int32_t>(i), planar_topo_list[i]);
    pr_candidate_list.insert(pr_candidate_list.end(), topo_candidate_list.begin(), topo_candidate_list.end());
  }
  return pr_candidate_list;
}

PlanarRouter::PRShadowDemandMap PlanarRouter::initPRShadowDemandMap(PRModel& pr_model)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  GridMap<uint8_t>& orient_mask_map = pr_model.get_shadow_orient_mask_map();
  GridMap<int32_t>& stamp_map = pr_model.get_shadow_stamp_map();

  if (orient_mask_map.get_x_size() != pr_node_map.get_x_size() || orient_mask_map.get_y_size() != pr_node_map.get_y_size()
      || stamp_map.get_x_size() != pr_node_map.get_x_size() || stamp_map.get_y_size() != pr_node_map.get_y_size()) {
    orient_mask_map.init(pr_node_map.get_x_size(), pr_node_map.get_y_size(), 0);
    stamp_map.init(pr_node_map.get_x_size(), pr_node_map.get_y_size(), 0);
    pr_model.set_shadow_stamp(0);
  }

  int32_t shadow_stamp = pr_model.get_shadow_stamp() + 1;
  if (shadow_stamp == INT_MAX) {
    stamp_map.init(pr_node_map.get_x_size(), pr_node_map.get_y_size(), 0);
    shadow_stamp = 1;
  }
  pr_model.set_shadow_stamp(shadow_stamp);

  PRShadowDemandMap shadow_demand_map;
  shadow_demand_map.orient_mask_map = &orient_mask_map;
  shadow_demand_map.stamp_map = &stamp_map;
  shadow_demand_map.stamp = shadow_stamp;
  return shadow_demand_map;
}

bool PlanarRouter::isBetterCandidate(PRCandidate& candidate, PRCandidate& current_best, double corner_weight)
{
  auto computeScore = [corner_weight](PRCandidate& c) { return c.get_total_wire_length() + c.get_total_cost() + corner_weight * c.get_total_corner_num(); };

  bool a_blocked = candidate.get_is_path_blocked();
  bool b_blocked = current_best.get_is_path_blocked();
  if (!a_blocked && b_blocked) {
    return true;
  } else if (a_blocked && !b_blocked) {
    return false;
  }
  double score_a = computeScore(candidate);
  double score_b = computeScore(current_best);
  if (std::abs(score_a - score_b) < 1e-9) {
    if (candidate.get_saturation_node_num() != current_best.get_saturation_node_num()) {
      return candidate.get_saturation_node_num() < current_best.get_saturation_node_num();
    }
    if (candidate.get_hotspot_node_num() != current_best.get_hotspot_node_num()) {
      return candidate.get_hotspot_node_num() < current_best.get_hotspot_node_num();
    }
    if (std::abs(candidate.get_max_usage_ratio() - current_best.get_max_usage_ratio()) >= 1e-9) {
      return candidate.get_max_usage_ratio() < current_best.get_max_usage_ratio();
    }
    return candidate.get_total_wire_length() < current_best.get_total_wire_length();
  }
  return score_a < score_b;
}

std::vector<Segment<PlanarCoord>> PlanarRouter::getPlanarTopoList(PRModel& pr_model)
{
  std::vector<PlanarCoord> planar_coord_list;
  {
    for (PRPin& pr_pin : pr_model.get_curr_pr_task()->get_pr_pin_list()) {
      planar_coord_list.push_back(pr_pin.get_access_point().get_grid_coord());
    }
    std::sort(planar_coord_list.begin(), planar_coord_list.end(), CmpPlanarCoordByXASC());
    planar_coord_list.erase(std::unique(planar_coord_list.begin(), planar_coord_list.end()), planar_coord_list.end());
  }
  std::vector<Segment<PlanarCoord>> planar_topo_list;
  for (Segment<PlanarCoord>& planar_topo : RTTB.getPlanarTopoList(planar_coord_list)) {
    planar_topo_list.push_back(planar_topo);
  }
  return planar_topo_list;
}

bool PlanarRouter::isLongObliqueTopo(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  int32_t topo_spilt_length = pr_model.get_pr_com_param().get_topo_spilt_length();
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  int32_t span_x = std::abs(first_coord.get_x() - second_coord.get_x());
  int32_t span_y = std::abs(first_coord.get_y() - second_coord.get_y());
  return (span_x > 1 && span_y > 1 && (span_x > topo_spilt_length || span_y > topo_spilt_length));
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByStraight(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isOblique(first_coord, second_coord)) {
    return {};
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(first_coord, second_coord);
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByLPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  PlanarCoord inflection_coord1(first_coord.get_x(), second_coord.get_y());
  inflection_list_list.push_back({inflection_coord1});
  PlanarCoord inflection_coord2(second_coord.get_x(), first_coord.get_y());
  inflection_list_list.push_back({inflection_coord2});

  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByZPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  std::vector<int32_t> x_mid_index_list = getMidIndexList(first_coord.get_x(), second_coord.get_x());
  std::vector<int32_t> y_mid_index_list = getMidIndexList(first_coord.get_y(), second_coord.get_y());
  if (x_mid_index_list.empty() && y_mid_index_list.empty()) {
    return {};
  }
  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  for (size_t i = 0; i < x_mid_index_list.size(); i++) {
    PlanarCoord inflection_coord1(x_mid_index_list[i], first_coord.get_y());
    PlanarCoord inflection_coord2(x_mid_index_list[i], second_coord.get_y());
    inflection_list_list.push_back({inflection_coord1, inflection_coord2});
  }
  for (size_t i = 0; i < y_mid_index_list.size(); i++) {
    PlanarCoord inflection_coord1(first_coord.get_x(), y_mid_index_list[i]);
    PlanarCoord inflection_coord2(second_coord.get_x(), y_mid_index_list[i]);
    inflection_list_list.push_back({inflection_coord1, inflection_coord2});
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<int32_t> PlanarRouter::getMidIndexList(int32_t first_idx, int32_t second_idx)
{
  std::vector<int32_t> mid_index_list;
  RTUTIL.swapByASC(first_idx, second_idx);
  mid_index_list.reserve(second_idx - first_idx - 1);
  for (int32_t i = (first_idx + 1); i <= (second_idx - 1); i++) {
    mid_index_list.push_back(i);
  }
  return mid_index_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByUPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  Die& die = RTDM.getDatabase().get_die();
  int32_t expand_step_num = pr_model.get_pr_com_param().get_expand_step_num();
  int32_t expand_step_length = pr_model.get_pr_com_param().get_expand_step_length();

  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.getManhattanDistance(first_coord, second_coord) <= 1) {
    return {};
  }
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  if (!RTUTIL.isHorizontal(first_coord, second_coord)) {
    for (int32_t i = 0; i < expand_step_num; i++) {
      first_x -= expand_step_length;
      if (first_x >= die.get_grid_ll_x()) {
        PlanarCoord inflection_coord1(first_x, first_coord.get_y());
        PlanarCoord inflection_coord2(first_x, second_coord.get_y());
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
      second_x += expand_step_length;
      if (second_x <= die.get_grid_ur_x()) {
        PlanarCoord inflection_coord1(second_x, first_coord.get_y());
        PlanarCoord inflection_coord2(second_x, second_coord.get_y());
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
    }
  }
  if (!RTUTIL.isVertical(first_coord, second_coord)) {
    for (int32_t i = 0; i < expand_step_num; i++) {
      first_y -= expand_step_length;
      if (first_y >= die.get_grid_ll_y()) {
        PlanarCoord inflection_coord1(first_coord.get_x(), first_y);
        PlanarCoord inflection_coord2(second_coord.get_x(), first_y);
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
      second_y += expand_step_length;
      if (second_y <= die.get_grid_ur_y()) {
        PlanarCoord inflection_coord1(first_coord.get_x(), second_y);
        PlanarCoord inflection_coord2(second_coord.get_x(), second_y);
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
    }
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByInner3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  std::vector<int32_t> x_mid_index_list = getMidIndexList(first_coord.get_x(), second_coord.get_x());
  std::vector<int32_t> y_mid_index_list = getMidIndexList(first_coord.get_y(), second_coord.get_y());
  if (x_mid_index_list.empty() || y_mid_index_list.empty()) {
    return {};
  }
  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  for (size_t i = 0; i < x_mid_index_list.size(); i++) {
    for (size_t j = 0; j < y_mid_index_list.size(); j++) {
      PlanarCoord inflection_coord1(x_mid_index_list[i], first_coord.get_y());
      PlanarCoord inflection_coord2(x_mid_index_list[i], y_mid_index_list[j]);
      PlanarCoord inflection_coord3(second_coord.get_x(), y_mid_index_list[j]);
      inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
    }
  }

  for (size_t i = 0; i < x_mid_index_list.size(); i++) {
    for (size_t j = 0; j < y_mid_index_list.size(); j++) {
      PlanarCoord inflection_coord1(first_coord.get_x(), y_mid_index_list[j]);
      PlanarCoord inflection_coord2(x_mid_index_list[i], y_mid_index_list[j]);
      PlanarCoord inflection_coord3(x_mid_index_list[i], second_coord.get_y());
      inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
    }
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByLowCostLane3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo,
                                                                                                      const PRShadowDemandMap* shadow_demand_map)
{
  constexpr int32_t kLowCostLaneTopK = 4;
  constexpr int32_t kLowCostLaneMaxLaneNum = 6;
  constexpr int32_t kLowCostLaneMaxScanNum = 128;

  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }

  int32_t min_x = std::min(first_coord.get_x(), second_coord.get_x());
  int32_t max_x = std::max(first_coord.get_x(), second_coord.get_x());
  int32_t min_y = std::min(first_coord.get_y(), second_coord.get_y());
  int32_t max_y = std::max(first_coord.get_y(), second_coord.get_y());
  if (max_x - min_x <= 1 || max_y - min_y <= 1) {
    return {};
  }

  auto makeSegmentScore = [&](const PlanarCoord& start_coord, const PlanarCoord& end_coord) {
    if (start_coord == end_coord) {
      return 0.0;
    }
    Segment<PlanarCoord> segment(start_coord, end_coord);
    return getPatternSegmentFastScore(pr_model, segment, shadow_demand_map);
  };

  auto getSampledLaneList = [kLowCostLaneMaxScanNum](int32_t min_idx, int32_t max_idx) {
    std::vector<int32_t> lane_list;
    int32_t lane_num = max_idx - min_idx - 1;
    if (lane_num <= 0) {
      return lane_list;
    }
    int32_t scan_num = std::min(lane_num, kLowCostLaneMaxScanNum);
    lane_list.reserve(scan_num);
    for (int32_t i = 0; i < scan_num; i++) {
      int32_t lane_idx = min_idx + 1 + static_cast<int32_t>(std::llround((lane_num - 1) * (i / 1.0 / std::max(1, scan_num - 1))));
      if (lane_list.empty() || lane_list.back() != lane_idx) {
        lane_list.push_back(lane_idx);
      }
    }
    return lane_list;
  };

  auto appendQuantileLane = [](std::vector<int32_t>& lane_list, int32_t min_idx, int32_t max_idx, int32_t numerator, int32_t denominator) {
    int32_t lane_idx = min_idx + static_cast<int32_t>(std::llround((max_idx - min_idx) * (numerator / 1.0 / denominator)));
    if (min_idx < lane_idx && lane_idx < max_idx && !RTUTIL.exist(lane_list, lane_idx)) {
      lane_list.push_back(lane_idx);
    }
  };

  std::vector<std::pair<int32_t, double>> x_lane_score_list;
  for (int32_t x : getSampledLaneList(min_x, max_x)) {
    PlanarCoord vertical_start(x, min_y);
    PlanarCoord vertical_end(x, max_y);
    double score = 0;
    score += makeSegmentScore(first_coord, PlanarCoord(x, first_coord.get_y()));
    score += makeSegmentScore(vertical_start, vertical_end);
    score += makeSegmentScore(PlanarCoord(x, second_coord.get_y()), second_coord);
    x_lane_score_list.emplace_back(x, score);
  }
  std::sort(x_lane_score_list.begin(), x_lane_score_list.end(), [](auto& a, auto& b) {
    if (!RTUTIL.equalDoubleByError(a.second, b.second, RT_ERROR)) {
      return a.second < b.second;
    }
    return a.first < b.first;
  });

  std::vector<std::pair<int32_t, double>> y_lane_score_list;
  for (int32_t y : getSampledLaneList(min_y, max_y)) {
    PlanarCoord horizontal_start(min_x, y);
    PlanarCoord horizontal_end(max_x, y);
    double score = 0;
    score += makeSegmentScore(first_coord, PlanarCoord(first_coord.get_x(), y));
    score += makeSegmentScore(horizontal_start, horizontal_end);
    score += makeSegmentScore(PlanarCoord(second_coord.get_x(), y), second_coord);
    y_lane_score_list.emplace_back(y, score);
  }
  std::sort(y_lane_score_list.begin(), y_lane_score_list.end(), [](auto& a, auto& b) {
    if (!RTUTIL.equalDoubleByError(a.second, b.second, RT_ERROR)) {
      return a.second < b.second;
    }
    return a.first < b.first;
  });

  std::vector<int32_t> selected_x_lane_list;
  std::vector<int32_t> selected_y_lane_list;
  for (int32_t i = 0; i < std::min(kLowCostLaneTopK, static_cast<int32_t>(x_lane_score_list.size())); i++) {
    selected_x_lane_list.push_back(x_lane_score_list[i].first);
  }
  for (int32_t i = 0; i < std::min(kLowCostLaneTopK, static_cast<int32_t>(y_lane_score_list.size())); i++) {
    selected_y_lane_list.push_back(y_lane_score_list[i].first);
  }
  for (auto [numerator, denominator] : {std::pair<int32_t, int32_t>(1, 4), std::pair<int32_t, int32_t>(1, 2), std::pair<int32_t, int32_t>(3, 4)}) {
    if (static_cast<int32_t>(selected_x_lane_list.size()) < kLowCostLaneMaxLaneNum) {
      appendQuantileLane(selected_x_lane_list, min_x, max_x, numerator, denominator);
    }
    if (static_cast<int32_t>(selected_y_lane_list.size()) < kLowCostLaneMaxLaneNum) {
      appendQuantileLane(selected_y_lane_list, min_y, max_y, numerator, denominator);
    }
  }

  std::sort(selected_x_lane_list.begin(), selected_x_lane_list.end());
  selected_x_lane_list.erase(std::unique(selected_x_lane_list.begin(), selected_x_lane_list.end()), selected_x_lane_list.end());
  std::sort(selected_y_lane_list.begin(), selected_y_lane_list.end());
  selected_y_lane_list.erase(std::unique(selected_y_lane_list.begin(), selected_y_lane_list.end()), selected_y_lane_list.end());

  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (int32_t x : selected_x_lane_list) {
    for (int32_t y : selected_y_lane_list) {
      PlanarCoord horizontal_mid1(x, first_coord.get_y());
      PlanarCoord horizontal_mid2(x, y);
      PlanarCoord horizontal_mid3(second_coord.get_x(), y);
      routing_segment_list_list.push_back({Segment<PlanarCoord>(first_coord, horizontal_mid1), Segment<PlanarCoord>(horizontal_mid1, horizontal_mid2),
                                           Segment<PlanarCoord>(horizontal_mid2, horizontal_mid3), Segment<PlanarCoord>(horizontal_mid3, second_coord)});

      PlanarCoord vertical_mid1(first_coord.get_x(), y);
      PlanarCoord vertical_mid2(x, y);
      PlanarCoord vertical_mid3(x, second_coord.get_y());
      routing_segment_list_list.push_back({Segment<PlanarCoord>(first_coord, vertical_mid1), Segment<PlanarCoord>(vertical_mid1, vertical_mid2),
                                           Segment<PlanarCoord>(vertical_mid2, vertical_mid3), Segment<PlanarCoord>(vertical_mid3, second_coord)});
    }
  }
  return routing_segment_list_list;
}

double PlanarRouter::getPatternSegmentFastScore(PRModel& pr_model, Segment<PlanarCoord>& segment, const PRShadowDemandMap* shadow_demand_map)
{
  PlanarCoord& first_coord = segment.get_first();
  PlanarCoord& second_coord = segment.get_second();
  if (!RTUTIL.isRightAngled(first_coord, second_coord)) {
    RTLOG.error(Loc::current(), "The direction is error!");
  }
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double overflow_unit = pr_model.get_pr_com_param().get_overflow_unit();
  double wire_unit = 1.0;

  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  double score = RTUTIL.getManhattanDistance(first_coord, second_coord) * wire_unit;
  uint8_t direction_mask = getPRDirectionMask(RTUTIL.getDirection(first_coord, second_coord));
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      uint8_t shadow_mask = shadow_demand_map ? shadow_demand_map->getMask(x, y) : 0;
      PRNodeCost node_cost = pr_node_map[x][y].getFastCost(direction_mask | shadow_mask, overflow_unit);
      score += node_cost.getTotalCost();
    }
  }
  return score;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByOuter3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  Die& die = RTDM.getDatabase().get_die();
  int32_t expand_step_num = pr_model.get_pr_com_param().get_expand_step_num();
  int32_t expand_step_length = pr_model.get_pr_com_param().get_expand_step_length();

  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  int32_t start_x = first_coord.get_x();
  int32_t end_x = second_coord.get_x();
  int32_t start_y = first_coord.get_y();
  int32_t end_y = second_coord.get_y();

  int32_t box_lb_x = std::min(start_x, end_x);
  int32_t box_rt_x = std::max(start_x, end_x);
  int32_t box_lb_y = std::min(start_y, end_y);
  int32_t box_rt_y = std::max(start_y, end_y);

  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  for (int32_t i = 0; i < expand_step_num; i++) {
    box_lb_x -= expand_step_length;
    box_rt_x += expand_step_length;
    box_lb_y -= expand_step_length;
    box_rt_y += expand_step_length;
    if (start_x < end_x) {
      if (start_y < end_y) {
        /**
         *    line style
         *
         *            x(e)
         *          x
         *        x
         *      x(s)
         *
         */
        if (die.get_grid_ll_y() <= box_lb_y && box_rt_x <= die.get_grid_ur_x()) {
          PlanarCoord inflection_coord1(start_x, box_lb_y);
          PlanarCoord inflection_coord2(box_rt_x, box_lb_y);
          PlanarCoord inflection_coord3(box_rt_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(box_lb_x, start_y);
          PlanarCoord inflection_coord2(box_lb_x, box_rt_y);
          PlanarCoord inflection_coord3(end_x, box_rt_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      } else {
        /**
         *    line style
         *
         *   x(s)
         *     x
         *       x
         *         x(e)
         *
         */
        if (box_rt_x <= die.get_grid_ur_x() && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(start_x, box_rt_y);
          PlanarCoord inflection_coord2(box_rt_x, box_rt_y);
          PlanarCoord inflection_coord3(box_rt_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && die.get_grid_ll_y() <= box_lb_y) {
          PlanarCoord inflection_coord1(box_lb_x, start_y);
          PlanarCoord inflection_coord2(box_lb_x, box_lb_y);
          PlanarCoord inflection_coord3(end_x, box_lb_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      }

    } else {
      if (start_y < end_y) {
        /**
         *    line style
         *
         *   x(e)
         *     x
         *       x
         *         x(s)
         *
         */
        if (box_rt_x <= die.get_grid_ur_x() && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(box_rt_x, start_y);
          PlanarCoord inflection_coord2(box_rt_x, box_rt_y);
          PlanarCoord inflection_coord3(end_x, box_rt_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && die.get_grid_ll_y() <= box_lb_y) {
          PlanarCoord inflection_coord1(start_x, box_lb_y);
          PlanarCoord inflection_coord2(box_lb_x, box_lb_y);
          PlanarCoord inflection_coord3(box_lb_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      } else {
        /**
         *    line style
         *
         *            x(s)
         *          x
         *        x
         *      x(e)
         *
         */
        if (die.get_grid_ll_y() <= box_lb_y && box_rt_x <= die.get_grid_ur_x()) {
          PlanarCoord inflection_coord1(box_rt_x, start_y);
          PlanarCoord inflection_coord2(box_rt_x, box_lb_y);
          PlanarCoord inflection_coord3(end_x, box_lb_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(start_x, box_rt_y);
          PlanarCoord inflection_coord2(box_lb_x, box_rt_y);
          PlanarCoord inflection_coord3(box_lb_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      }
    }
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

void PlanarRouter::updatePRCandidate(PRModel& pr_model, PRCandidate& pr_candidate, const PRShadowDemandMap* shadow_demand_map)
{
  double overflow_unit = pr_model.get_pr_com_param().get_overflow_unit();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();

  PRCandidateCost candidate_cost;
  Direction pre_direction = Direction::kNone;
  for (Segment<PlanarCoord>& coord_segment : pr_candidate.get_routing_segment_list()) {
    PlanarCoord& first_coord = coord_segment.get_first();
    PlanarCoord& second_coord = coord_segment.get_second();
    if (!RTUTIL.isRightAngled(first_coord, second_coord)) {
      RTLOG.error(Loc::current(), "The direction is error!");
    }
    candidate_cost.total_wire_length += RTUTIL.getManhattanDistance(first_coord, second_coord);

    int32_t first_x = first_coord.get_x();
    int32_t second_x = second_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t second_y = second_coord.get_y();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);
    Direction direction = RTUTIL.getDirection(first_coord, second_coord);
    if (pre_direction != Direction::kNone && pre_direction != direction) {
      candidate_cost.total_corner_num++;
    }
    pre_direction = direction;
    uint8_t direction_mask = getPRDirectionMask(direction);
    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        uint8_t shadow_mask = shadow_demand_map ? shadow_demand_map->getMask(x, y) : 0;
        PRNodeCost node_cost = pr_node_map[x][y].getFastCost(direction_mask | shadow_mask, overflow_unit);
        if (node_cost.overflow > 0) {
          candidate_cost.is_path_blocked = true;
          candidate_cost.overflow_node_num++;
        }
        candidate_cost.total_usage_cost += node_cost.usage_cost;
        candidate_cost.total_saturation_cost += node_cost.saturation_cost;
        candidate_cost.total_hotspot_cost += node_cost.hotspot_cost;
        candidate_cost.total_overflow_cost += node_cost.overflow_cost;
        candidate_cost.total_overflow += node_cost.overflow;
        candidate_cost.max_usage_ratio = std::max(candidate_cost.max_usage_ratio, node_cost.max_usage_ratio);
        if (node_cost.saturation_orient_num > 0) {
          candidate_cost.saturation_node_num++;
        }
        if (node_cost.hotspot_orient_num > 0) {
          candidate_cost.hotspot_node_num++;
        }
      }
    }
  }

  pr_candidate.set_candidate_cost(candidate_cost);
}

MTree<PlanarCoord> PlanarRouter::getCoordTree(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  std::vector<PlanarCoord> candidate_root_coord_list;
  std::map<PlanarCoord, std::set<int32_t>, CmpPlanarCoordByXASC> key_coord_pin_map;
  std::vector<PRPin>& pr_pin_list = pr_model.get_curr_pr_task()->get_pr_pin_list();
  for (size_t i = 0; i < pr_pin_list.size(); i++) {
    PlanarCoord coord = pr_pin_list[i].get_access_point().get_grid_coord();
    candidate_root_coord_list.push_back(coord);
    key_coord_pin_map[coord].insert(static_cast<int32_t>(i));
  }
  return RTUTIL.getTreeByFullFlow(candidate_root_coord_list, routing_segment_list, key_coord_pin_map);
}

void PlanarRouter::uploadNetResult(PRModel& pr_model, MTree<PlanarCoord>& coord_tree)
{
  for (Segment<TNode<PlanarCoord>*>& coord_segment : RTUTIL.getSegListByTree(coord_tree)) {
    Segment<LayerCoord>* segment = new Segment<LayerCoord>({coord_segment.get_first()->value(), 0}, {coord_segment.get_second()->value(), 0});
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, pr_model.get_curr_pr_task()->get_net_idx(), segment);
  }
}

void PlanarRouter::resetSingleTask(PRModel& pr_model)
{
  pr_model.set_curr_pr_task(nullptr);
}

void PlanarRouter::reroutePRModel(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  setPRIterParam(pr_model);
  rebuildDemandToGraph(pr_model);
  initNetGlobalResultMap(pr_model);
  initPRMetric(pr_model);

  PRIterParam& pr_iter_param = pr_model.get_pr_iter_param();
  for (int32_t iter = 1; iter <= pr_iter_param.get_max_iter_num(); iter++) {
    Monitor iter_monitor;
    RTLOG.info(Loc::current(), "***** Begin iteration ", iter, "/", pr_iter_param.get_max_iter_num(), "(",
               RTUTIL.getPercentage(iter, pr_iter_param.get_max_iter_num()), ") *****");
    updateCongestionRisk(pr_model);
    if (pr_model.get_metric_valid()) {
      pr_model.set_curr_congestion_risk(getCongestionRisk(pr_model));
    }
    updateBestResult(pr_model);

    if (iter < pr_iter_param.get_max_iter_num()) {
      routePRNetTaskListByPattern(pr_model);
      outputCongestionSnapshotCSV(pr_model, RTUTIL.getString("_iter", iter, "_pattern"), iter);
    } else {
      routePRNetTaskListByAStar(pr_model);
      outputCongestionSnapshotCSV(pr_model, RTUTIL.getString("_iter", iter, "_astar"), iter);
    }
    RTLOG.info(Loc::current(), "Completed net-level reroute iteration ", iter, ", overflow ", pr_model.get_curr_overflow(), ", high_usage ",
               pr_model.get_curr_high_usage(), iter_monitor.getStatsInfo());
  }

  updateCongestionRisk(pr_model);
  if (pr_model.get_metric_valid()) {
    pr_model.set_curr_congestion_risk(getCongestionRisk(pr_model));
  }
  if (pr_model.get_curr_overflow() <= RT_ERROR && pr_model.get_curr_high_usage() > RT_ERROR) {
    routePRSegmentTaskListByHighUsage(pr_model);
  }
  updateBestResult(pr_model);
  uploadBestResult(pr_model);
  rebuildDemandToGraph(pr_model);

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::setPRIterParam(PRModel& pr_model)
{
  int32_t max_iter_num = 3;
  int32_t max_routed_times = 1;
  int32_t max_task_num = 10000;
  double wire_unit = 1;
  double corner_unit = 5 * wire_unit;
  double overflow_unit = pr_model.get_pr_com_param().get_overflow_unit();
  double congestion_risk_unit = overflow_unit;
  double high_usage_unit = overflow_unit;
  double high_usage_ratio_threshold = 0.80;
  int32_t congestion_risk_radius = 3;
  int32_t route_window_base_expand = 10;
  int32_t route_window_max_expand_times = 3;
  double route_window_expand_ratio = 2;
  bool enable_full_die_fallback = false;

  PRIterParam pr_iter_param(max_iter_num, max_routed_times, max_task_num, wire_unit, corner_unit, overflow_unit, congestion_risk_unit, high_usage_unit,
                            high_usage_ratio_threshold, congestion_risk_radius, route_window_base_expand, route_window_max_expand_times,
                            route_window_expand_ratio, enable_full_die_fallback);
  pr_model.set_pr_iter_param(pr_iter_param);
}

void PlanarRouter::routePRSegmentTaskListByHighUsage(PRModel& pr_model)
{
  constexpr int32_t kMaxCleanupIter = 2;
  constexpr int32_t kMaxCleanupTaskNum = 10000;
  constexpr double kHighUsageUnitScale = 3.0;

  PRIterParam& pr_iter_param = pr_model.get_pr_iter_param();
  double origin_high_usage_unit = pr_iter_param.get_high_usage_unit();
  int32_t origin_route_window_base_expand = pr_iter_param.get_route_window_base_expand();
  int32_t origin_route_window_max_expand_times = pr_iter_param.get_route_window_max_expand_times();
  double origin_route_window_expand_ratio = pr_iter_param.get_route_window_expand_ratio();
  bool origin_enable_full_die_fallback = pr_iter_param.get_enable_full_die_fallback();

  pr_iter_param.set_high_usage_unit(kHighUsageUnitScale * pr_iter_param.get_overflow_unit());
  pr_iter_param.set_route_window_base_expand(10);
  pr_iter_param.set_route_window_max_expand_times(4);
  pr_iter_param.set_route_window_expand_ratio(2.0);
  pr_iter_param.set_enable_full_die_fallback(false);

  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting high-usage segment cleanup...");
  for (int32_t iter = 1; iter <= kMaxCleanupIter; iter++) {
    Monitor iter_monitor;
    double old_overflow = pr_model.get_curr_overflow();
    double old_high_usage = pr_model.get_curr_high_usage();

    std::vector<PRSegmentTask> pr_segment_task_list = initPRSegmentTaskList(pr_model, false, true, true);
    if (static_cast<int32_t>(pr_segment_task_list.size()) > kMaxCleanupTaskNum) {
      pr_segment_task_list.resize(kMaxCleanupTaskNum);
    }
    if (pr_segment_task_list.empty()) {
      break;
    }

    int32_t routed_task_num = 0;
    int32_t success_task_num = 0;
    for (PRSegmentTask& pr_segment_task : pr_segment_task_list) {
      if (pr_segment_task.get_high_usage() <= RT_ERROR) {
        continue;
      }
      routed_task_num++;
      if (routePRSegmentTask(pr_model, pr_segment_task, true)) {
        success_task_num++;
      }
    }

    updateCongestionRisk(pr_model);
    if (pr_model.get_metric_valid()) {
      pr_model.set_curr_congestion_risk(getCongestionRisk(pr_model));
    }
    updateBestResult(pr_model);

    RTLOG.info(Loc::current(), "Completed high-usage segment cleanup iteration ", iter, "/", kMaxCleanupIter, ", routed ", routed_task_num, ", success ",
               success_task_num, ", overflow ", old_overflow, " -> ", pr_model.get_curr_overflow(), ", high_usage ", old_high_usage, " -> ",
               pr_model.get_curr_high_usage(), iter_monitor.getStatsInfo());

    if (success_task_num == 0 || pr_model.get_curr_high_usage() >= old_high_usage - RT_ERROR) {
      break;
    }
    if (pr_model.get_curr_overflow() > RT_ERROR) {
      RTLOG.warn(Loc::current(), "Stop high-usage cleanup because overflow became ", pr_model.get_curr_overflow());
      break;
    }
  }

  pr_iter_param.set_high_usage_unit(origin_high_usage_unit);
  pr_iter_param.set_route_window_base_expand(origin_route_window_base_expand);
  pr_iter_param.set_route_window_max_expand_times(origin_route_window_max_expand_times);
  pr_iter_param.set_route_window_expand_ratio(origin_route_window_expand_ratio);
  pr_iter_param.set_enable_full_die_fallback(origin_enable_full_die_fallback);
  RTLOG.info(Loc::current(), "Completed high-usage segment cleanup", monitor.getStatsInfo());
}

void PlanarRouter::rebuildDemandToGraph(PRModel& pr_model)
{
  Die& die = RTDM.getDatabase().get_die();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();

#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      pr_node_map[x][y].clearDemand();
    }
  }

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      std::vector<Segment<PlanarCoord>> segment_list;
      segment_list.emplace_back(segment->get_first().get_planar_coord(), segment->get_second().get_planar_coord());
      updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, segment_list);
    }
  }
}

void PlanarRouter::initNetGlobalResultMap(PRModel& pr_model)
{
  Die& die = RTDM.getDatabase().get_die();
  pr_model.set_net_global_result_map(RTDM.getNetGlobalResultMap(die));
}

double PlanarRouter::getOverflow(PRModel& pr_model)
{
  double total_overflow = 0;
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      total_overflow += pr_node_map[x][y].getOverflow();
    }
  }
  return total_overflow;
}

double PlanarRouter::getCongestionRisk(PRModel& pr_model)
{
  double total_congestion_risk = 0;
  for (auto& [net_idx, segment_set] : pr_model.get_net_global_result_map()) {
    for (Segment<LayerCoord>* segment : segment_set) {
      total_congestion_risk += getSegmentCongestionRisk(pr_model, segment);
    }
  }
  return total_congestion_risk;
}

double PlanarRouter::getHighUsage(PRModel& pr_model)
{
  double total_high_usage = 0;
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      total_high_usage += pr_node_map[x][y].getHighUsage(high_usage_ratio_threshold);
    }
  }
  return total_high_usage;
}

double PlanarRouter::getWireLength(PRModel& pr_model)
{
  double total_wire_length = 0;
  for (auto& [net_idx, segment_set] : pr_model.get_net_global_result_map()) {
    for (Segment<LayerCoord>* segment : segment_set) {
      total_wire_length += RTUTIL.getManhattanDistance(segment->get_first(), segment->get_second());
    }
  }
  return total_wire_length;
}

void PlanarRouter::initPRMetric(PRModel& pr_model)
{
  pr_model.set_curr_overflow(getOverflow(pr_model));
  pr_model.set_curr_high_usage(getHighUsage(pr_model));
  pr_model.set_curr_congestion_risk(getCongestionRisk(pr_model));
  pr_model.set_curr_wire_length(getWireLength(pr_model));
  pr_model.set_metric_valid(true);
  pr_model.get_changed_net_set().clear();
}

void PlanarRouter::updateCongestionRisk(PRModel& pr_model)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  GridMap<double>& congestion_risk_map = pr_model.get_congestion_risk_map();

  int32_t risk_radius = std::max(0, pr_model.get_pr_iter_param().get_congestion_risk_radius());
  double history_risk_decay = 0.5;
  GridMap<double> history_congestion_risk_map = congestion_risk_map;
  congestion_risk_map.init(pr_node_map.get_x_size(), pr_node_map.get_y_size(), 0.0);
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      double overflow = pr_node_map[x][y].getOverflow();
      if (overflow <= 0) {
        continue;
      }
      for (int32_t dx = -risk_radius; dx <= risk_radius; dx++) {
        for (int32_t dy = -risk_radius; dy <= risk_radius; dy++) {
          int32_t risk_x = x + dx;
          int32_t risk_y = y + dy;
          if (!congestion_risk_map.isInside(risk_x, risk_y)) {
            continue;
          }
          int32_t distance = std::abs(dx) + std::abs(dy);
          if (distance > risk_radius) {
            continue;
          }
          double decay = 1.0 / (distance + 1);
          congestion_risk_map[risk_x][risk_y] += overflow * decay;
        }
      }
    }
  }
  if (history_congestion_risk_map.get_x_size() == congestion_risk_map.get_x_size()
      && history_congestion_risk_map.get_y_size() == congestion_risk_map.get_y_size()) {
    for (int32_t x = 0; x < congestion_risk_map.get_x_size(); x++) {
      for (int32_t y = 0; y < congestion_risk_map.get_y_size(); y++) {
        congestion_risk_map[x][y] = std::max(congestion_risk_map[x][y], history_congestion_risk_map[x][y] * history_risk_decay);
      }
    }
  }
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      pr_node_map[x][y].set_congestion_risk(congestion_risk_map[x][y]);
    }
  }
}

void PlanarRouter::collectPRHotspotInfo(PRModel& pr_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set, std::set<int32_t>& overflow_net_set,
                                        std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set, std::set<int32_t>& high_usage_net_set)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();

  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      PRNode& pr_node = pr_node_map[x][y];
      if (pr_node.getOverflow() > 0) {
        overflow_coord_set.insert(PlanarCoord(x, y));
        std::set<int32_t> node_overflow_net_set = pr_node.getOverflowNetSet();
        overflow_net_set.insert(node_overflow_net_set.begin(), node_overflow_net_set.end());
      }
      if (pr_node.getMaxUsageRatio() > high_usage_ratio_threshold + RT_ERROR) {
        high_usage_coord_set.insert(PlanarCoord(x, y));
        std::set<int32_t> node_high_usage_net_set = pr_node.getHighUsageNetSet(high_usage_ratio_threshold);
        high_usage_net_set.insert(node_high_usage_net_set.begin(), node_high_usage_net_set.end());
      }
    }
  }
}

std::vector<PRSegmentTask> PlanarRouter::initPRSegmentTaskList(PRModel& pr_model, bool include_overflow, bool include_high_usage, bool high_usage_first)
{
  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();
  int32_t max_task_num = pr_model.get_pr_iter_param().get_max_task_num();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();

  std::set<PlanarCoord, CmpPlanarCoordByXASC> overflow_coord_set;
  std::set<int32_t> overflow_net_set;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> high_usage_coord_set;
  std::set<int32_t> high_usage_net_set;
  collectPRHotspotInfo(pr_model, overflow_coord_set, overflow_net_set, high_usage_coord_set, high_usage_net_set);

  std::set<int32_t> candidate_net_set;
  if (include_overflow) {
    candidate_net_set.insert(overflow_net_set.begin(), overflow_net_set.end());
  }
  if (include_high_usage) {
    candidate_net_set.insert(high_usage_net_set.begin(), high_usage_net_set.end());
  }
  std::set<int32_t> changed_candidate_net_set = pr_model.get_changed_net_set();
  candidate_net_set.insert(changed_candidate_net_set.begin(), changed_candidate_net_set.end());
  pr_model.get_changed_net_set().clear();

  std::set<Segment<LayerCoord>*> visited_segment_set;
  std::vector<PRSegmentTask> pr_segment_task_list;
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map = pr_model.get_net_global_result_map();
  for (int32_t net_idx : candidate_net_set) {
    auto iter = net_global_result_map.find(net_idx);
    if (iter == net_global_result_map.end()) {
      continue;
    }
    for (Segment<LayerCoord>* segment : iter->second) {
      if (RTUTIL.exist(visited_segment_set, segment)) {
        continue;
      }
      visited_segment_set.insert(segment);
      double segment_overflow = getSegmentOverflow(pr_model, segment);
      double segment_congestion_risk = getSegmentCongestionRisk(pr_model, segment);
      double segment_high_usage = getSegmentHighUsage(pr_model, segment);
      double segment_max_usage_ratio = getSegmentMaxUsageRatio(pr_model, segment);
      bool changed_net = RTUTIL.exist(changed_candidate_net_set, net_idx);
      bool cross_overflow
          = include_overflow && (RTUTIL.exist(overflow_net_set, net_idx) || changed_net) && isSegmentCrossOverflow(pr_model, segment, overflow_coord_set);
      bool cross_high_usage = include_high_usage && (RTUTIL.exist(high_usage_net_set, net_idx) || changed_net)
                              && isSegmentCrossHighUsage(pr_model, segment, high_usage_coord_set);
      if (!cross_overflow && !cross_high_usage) {
        continue;
      }
      std::map<PlanarCoord, double, CmpPlanarCoordByXASC> origin_overflow_penalty_map;
      if (cross_overflow) {
        PlanarCoord first_coord = segment->get_first().get_planar_coord();
        PlanarCoord second_coord = segment->get_second().get_planar_coord();
        int32_t first_x = first_coord.get_x();
        int32_t second_x = second_coord.get_x();
        int32_t first_y = first_coord.get_y();
        int32_t second_y = second_coord.get_y();
        RTUTIL.swapByASC(first_x, second_x);
        RTUTIL.swapByASC(first_y, second_y);
        for (int32_t x = first_x; x <= second_x; x++) {
          for (int32_t y = first_y; y <= second_y; y++) {
            PlanarCoord coord(x, y);
            if (coord == first_coord || coord == second_coord || !RTUTIL.exist(overflow_coord_set, coord)) {
              continue;
            }
            origin_overflow_penalty_map[coord] = pr_model.get_pr_node_map()[x][y].getOverflow();
          }
        }
      }
      std::map<PlanarCoord, double, CmpPlanarCoordByXASC> origin_high_usage_penalty_map;
      if (cross_high_usage) {
        PlanarCoord first_coord = segment->get_first().get_planar_coord();
        PlanarCoord second_coord = segment->get_second().get_planar_coord();
        int32_t first_x = first_coord.get_x();
        int32_t second_x = second_coord.get_x();
        int32_t first_y = first_coord.get_y();
        int32_t second_y = second_coord.get_y();
        RTUTIL.swapByASC(first_x, second_x);
        RTUTIL.swapByASC(first_y, second_y);
        for (int32_t x = first_x; x <= second_x; x++) {
          for (int32_t y = first_y; y <= second_y; y++) {
            PlanarCoord coord(x, y);
            if (coord == first_coord || coord == second_coord || !RTUTIL.exist(high_usage_coord_set, coord)) {
              continue;
            }
            origin_high_usage_penalty_map[coord] = pr_model.get_pr_node_map()[x][y].getHighUsage(high_usage_ratio_threshold);
          }
        }
      }
      PRSegmentTask pr_segment_task;
      pr_segment_task.set_net_idx(net_idx);
      pr_segment_task.set_connect_type(pr_net_list[net_idx].get_connect_type());
      pr_segment_task.set_origin_segment(segment);
      pr_segment_task.set_planar_segment(Segment<PlanarCoord>(segment->get_first().get_planar_coord(), segment->get_second().get_planar_coord()));
      pr_segment_task.set_wire_length(RTUTIL.getManhattanDistance(segment->get_first(), segment->get_second()));
      pr_segment_task.set_overflow(segment_overflow);
      pr_segment_task.set_congestion_risk(segment_congestion_risk);
      pr_segment_task.set_high_usage(segment_high_usage);
      pr_segment_task.set_max_usage_ratio(segment_max_usage_ratio);
      pr_segment_task.set_origin_overflow_penalty_map(origin_overflow_penalty_map);
      pr_segment_task.set_origin_high_usage_penalty_map(origin_high_usage_penalty_map);
      pr_segment_task_list.push_back(pr_segment_task);
    }
  }

  std::sort(pr_segment_task_list.begin(), pr_segment_task_list.end(), [high_usage_first](PRSegmentTask& a, PRSegmentTask& b) {
    if (high_usage_first) {
      if (!RTUTIL.equalDoubleByError(a.get_high_usage(), b.get_high_usage(), RT_ERROR)) {
        return a.get_high_usage() > b.get_high_usage();
      }
      if (!RTUTIL.equalDoubleByError(a.get_max_usage_ratio(), b.get_max_usage_ratio(), RT_ERROR)) {
        return a.get_max_usage_ratio() > b.get_max_usage_ratio();
      }
      if (!RTUTIL.equalDoubleByError(a.get_congestion_risk(), b.get_congestion_risk(), RT_ERROR)) {
        return a.get_congestion_risk() > b.get_congestion_risk();
      }
      if (!RTUTIL.equalDoubleByError(a.get_wire_length(), b.get_wire_length(), RT_ERROR)) {
        return a.get_wire_length() > b.get_wire_length();
      }
      return a.get_net_idx() < b.get_net_idx();
    }
    if (!RTUTIL.equalDoubleByError(a.get_overflow(), b.get_overflow(), RT_ERROR)) {
      return a.get_overflow() > b.get_overflow();
    }
    if (!RTUTIL.equalDoubleByError(a.get_high_usage(), b.get_high_usage(), RT_ERROR)) {
      return a.get_high_usage() > b.get_high_usage();
    }
    if (!RTUTIL.equalDoubleByError(a.get_congestion_risk(), b.get_congestion_risk(), RT_ERROR)) {
      return a.get_congestion_risk() > b.get_congestion_risk();
    }
    if (!RTUTIL.equalDoubleByError(a.get_max_usage_ratio(), b.get_max_usage_ratio(), RT_ERROR)) {
      return a.get_max_usage_ratio() > b.get_max_usage_ratio();
    }
    if (a.get_connect_type() != b.get_connect_type()) {
      return a.get_connect_type() == ConnectType::kClock;
    }
    if (!RTUTIL.equalDoubleByError(a.get_wire_length(), b.get_wire_length(), RT_ERROR)) {
      return a.get_wire_length() > b.get_wire_length();
    }
    return a.get_net_idx() < b.get_net_idx();
  });
  if (0 < max_task_num && max_task_num < static_cast<int32_t>(pr_segment_task_list.size())) {
    pr_segment_task_list.resize(max_task_num);
  }
  RTLOG.info(Loc::current(), "Generated ", pr_segment_task_list.size(), " PR segment tasks");
  return pr_segment_task_list;
}

bool PlanarRouter::routePRSegmentTask(PRModel& pr_model, PRSegmentTask& pr_segment_task, bool enable_true_local_accept)
{
  Segment<LayerCoord>* curr_segment = getCurrentGlobalSegment(pr_segment_task.get_net_idx(), pr_segment_task.get_planar_segment());
  if (curr_segment == nullptr) {
    return false;
  }
  double segment_overflow = getSegmentOverflow(pr_model, curr_segment);
  double segment_high_usage = getSegmentHighUsage(pr_model, curr_segment);
  if (segment_overflow <= RT_ERROR && segment_high_usage <= RT_ERROR) {
    return false;
  }

  int32_t net_idx = pr_segment_task.get_net_idx();
  std::vector<Segment<PlanarCoord>> origin_segment_list = {pr_segment_task.get_planar_segment()};
  PRRouteMetrics old_metrics = getRouteMetrics(pr_model, net_idx, origin_segment_list);
  PRRouteMetrics accepted_metrics;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> accepted_affected_coord_set;
  PRLocalRerouteMetrics accepted_old_local_metrics;
  PRLocalRerouteMetrics accepted_new_local_metrics;
  std::vector<Segment<PlanarCoord>> routing_segment_list;

  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, origin_segment_list);
  for (PlanarRect& route_window : getRouteWindowList(pr_model, pr_segment_task)) {
    routing_segment_list.clear();
    if (!searchSegmentByAStar(pr_model, pr_segment_task, route_window, routing_segment_list) || routing_segment_list.empty()) {
      continue;
    }
    routing_segment_list = simplifyRoutingSegmentList(routing_segment_list);
    if (routing_segment_list.empty() || isSameRoutingResult(pr_segment_task, routing_segment_list)) {
      continue;
    }
    PRRouteMetrics new_metrics = getRouteMetrics(pr_model, net_idx, routing_segment_list);
    bool accepted = false;
    std::set<PlanarCoord, CmpPlanarCoordByXASC> affected_coord_set;
    PRLocalRerouteMetrics old_local_metrics;
    PRLocalRerouteMetrics new_local_metrics;
    if (enable_true_local_accept) {
      collectSegmentCoordSet(origin_segment_list, affected_coord_set);
      collectSegmentCoordSet(routing_segment_list, affected_coord_set);

      updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, origin_segment_list);
      old_local_metrics = getCoordSetLocalMetrics(pr_model, affected_coord_set);
      updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, origin_segment_list);

      updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, routing_segment_list);
      new_local_metrics = getCoordSetLocalMetrics(pr_model, affected_coord_set);
      updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, routing_segment_list);

      accepted = acceptTrueLocalReroute(old_metrics, new_metrics, old_local_metrics, new_local_metrics);
    } else {
      accepted = acceptReroute(old_metrics, new_metrics);
    }
    if (accepted) {
      accepted_metrics = new_metrics;
      if (enable_true_local_accept) {
        accepted_affected_coord_set = affected_coord_set;
        accepted_old_local_metrics = old_local_metrics;
        accepted_new_local_metrics = new_local_metrics;
      }
      break;
    }
    routing_segment_list.clear();
  }
  if (routing_segment_list.empty()) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, origin_segment_list);
    return false;
  }

  std::set<PlanarCoord, CmpPlanarCoordByXASC> affected_coord_set;
  double old_affected_overflow = 0;
  double old_affected_high_usage = 0;
  if (enable_true_local_accept) {
    affected_coord_set = accepted_affected_coord_set;
    old_affected_overflow = accepted_old_local_metrics.overflow;
    old_affected_high_usage = accepted_old_local_metrics.high_usage;
  } else {
    collectSegmentCoordSet(origin_segment_list, affected_coord_set);
    collectSegmentCoordSet(routing_segment_list, affected_coord_set);
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, origin_segment_list);
    old_affected_overflow = getCoordSetOverflow(pr_model, affected_coord_set);
    old_affected_high_usage = getCoordSetHighUsage(pr_model, affected_coord_set);
    updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, origin_segment_list);
  }

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, routing_segment_list);

  double new_affected_overflow = enable_true_local_accept ? accepted_new_local_metrics.overflow : getCoordSetOverflow(pr_model, affected_coord_set);
  double new_affected_high_usage = enable_true_local_accept ? accepted_new_local_metrics.high_usage : getCoordSetHighUsage(pr_model, affected_coord_set);
  if (pr_model.get_metric_valid()) {
    pr_model.set_curr_overflow(pr_model.get_curr_overflow() + new_affected_overflow - old_affected_overflow);
    pr_model.set_curr_high_usage(pr_model.get_curr_high_usage() + new_affected_high_usage - old_affected_high_usage);
    pr_model.set_curr_congestion_risk(pr_model.get_curr_congestion_risk() + accepted_metrics.congestion_risk - old_metrics.congestion_risk);
    pr_model.set_curr_wire_length(pr_model.get_curr_wire_length() + accepted_metrics.wire_length - old_metrics.wire_length);
  }
  pr_model.get_changed_net_set().insert(net_idx);

  pr_model.get_net_global_result_map()[net_idx].erase(curr_segment);
  RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, curr_segment);
  if (pr_model.get_net_global_result_map()[net_idx].empty()) {
    pr_model.get_net_global_result_map().erase(net_idx);
  }
  for (Segment<PlanarCoord>& routing_segment : routing_segment_list) {
    Segment<LayerCoord>* new_segment = new Segment<LayerCoord>(LayerCoord(routing_segment.get_first(), 0), LayerCoord(routing_segment.get_second(), 0));
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, net_idx, new_segment);
    pr_model.get_net_global_result_map()[net_idx].insert(new_segment);
  }
  pr_segment_task.set_origin_segment(nullptr);
  return true;
}

void PlanarRouter::routePRNetTaskListByPattern(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<int32_t> pr_net_task_list = initPRNetTaskList(pr_model, true, true);
  size_t routed_task_num = 0;
  size_t success_task_num = 0;
  for (int32_t net_idx : pr_net_task_list) {
    if (routePRNetTaskByPattern(pr_model, net_idx)) {
      success_task_num++;
    }
    routed_task_num++;
  }
  RTLOG.info(Loc::current(), "Routed ", routed_task_num, " net tasks by pattern, success ", success_task_num, ", overflow ", pr_model.get_curr_overflow(),
             ", high_usage ", pr_model.get_curr_high_usage(), monitor.getStatsInfo());
}

bool PlanarRouter::routePRNetTaskByPattern(PRModel& pr_model, int32_t net_idx)
{
  if (net_idx < 0 || net_idx >= static_cast<int32_t>(pr_model.get_pr_net_list().size())) {
    return false;
  }
  std::vector<Segment<LayerCoord>*> old_segment_ptr_list = getNetGlobalSegmentPtrList(pr_model, net_idx);
  if (old_segment_ptr_list.empty()) {
    return false;
  }
  std::vector<Segment<PlanarCoord>> old_planar_segment_list = getPlanarSegmentList(old_segment_ptr_list);
  if (old_planar_segment_list.empty()) {
    return false;
  }
  PRRouteMetrics old_route_metrics = getRouteMetrics(pr_model, net_idx, old_planar_segment_list);
  if (old_route_metrics.overflow <= RT_ERROR && old_route_metrics.high_usage <= RT_ERROR) {
    return false;
  }

  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, old_planar_segment_list);

  PRNet* origin_curr_pr_task = pr_model.get_curr_pr_task();
  pr_model.set_curr_pr_task(&pr_model.get_pr_net_list()[net_idx]);
  std::vector<Segment<PlanarCoord>> topo_edge_list = getPlanarTopoList(pr_model);
  PRShadowDemandMap self_shadow = initPRShadowDemandMap(pr_model);
  std::vector<Segment<PlanarCoord>> new_planar_segment_list;
  bool success = true;
  for (size_t topo_idx = 0; topo_idx < topo_edge_list.size(); topo_idx++) {
    std::vector<Segment<PlanarCoord>> edge_segment_list;
    if (!routePRTopoEdgeByPattern(pr_model, static_cast<int32_t>(topo_idx), topo_edge_list[topo_idx], self_shadow, edge_segment_list)
        || edge_segment_list.empty()) {
      success = false;
      break;
    }
    new_planar_segment_list.insert(new_planar_segment_list.end(), edge_segment_list.begin(), edge_segment_list.end());
  }
  pr_model.set_curr_pr_task(origin_curr_pr_task);
  if (!success || new_planar_segment_list.empty()) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
    return false;
  }

  MTree<PlanarCoord> coord_tree;
  {
    PRNet* saved_curr_pr_task = pr_model.get_curr_pr_task();
    pr_model.set_curr_pr_task(&pr_model.get_pr_net_list()[net_idx]);
    coord_tree = getCoordTree(pr_model, new_planar_segment_list);
    pr_model.set_curr_pr_task(saved_curr_pr_task);
  }
  std::vector<Segment<PlanarCoord>> new_tree_segment_list = getPlanarSegmentList(coord_tree);
  if (new_tree_segment_list.empty()) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
    return false;
  }

  std::set<PlanarCoord, CmpPlanarCoordByXASC> affected_coord_set;
  collectSegmentCoordSet(old_planar_segment_list, affected_coord_set);
  collectSegmentCoordSet(new_tree_segment_list, affected_coord_set);

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
  PRLocalRerouteMetrics old_local_metrics = getCoordSetLocalMetrics(pr_model, affected_coord_set);
  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, old_planar_segment_list);

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, new_tree_segment_list);
  PRLocalRerouteMetrics new_local_metrics = getCoordSetLocalMetrics(pr_model, affected_coord_set);
  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, new_tree_segment_list);

  PRRouteMetrics new_route_metrics = getRouteMetrics(pr_model, net_idx, new_tree_segment_list);
  if (!acceptNetReroute(old_route_metrics, new_route_metrics, old_local_metrics, new_local_metrics)) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
    return false;
  }

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, new_tree_segment_list);
  if (pr_model.get_metric_valid()) {
    pr_model.set_curr_overflow(pr_model.get_curr_overflow() + new_local_metrics.overflow - old_local_metrics.overflow);
    pr_model.set_curr_high_usage(pr_model.get_curr_high_usage() + new_local_metrics.high_usage - old_local_metrics.high_usage);
    pr_model.set_curr_congestion_risk(pr_model.get_curr_congestion_risk() + new_route_metrics.congestion_risk - old_route_metrics.congestion_risk);
    pr_model.set_curr_wire_length(pr_model.get_curr_wire_length() + new_route_metrics.wire_length - old_route_metrics.wire_length);
  }

  pr_model.get_net_global_result_map().erase(net_idx);
  for (Segment<LayerCoord>* old_segment : old_segment_ptr_list) {
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, old_segment);
  }
  for (Segment<PlanarCoord>& new_planar_segment : new_tree_segment_list) {
    Segment<LayerCoord>* new_segment = new Segment<LayerCoord>(LayerCoord(new_planar_segment.get_first(), 0), LayerCoord(new_planar_segment.get_second(), 0));
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, net_idx, new_segment);
    pr_model.get_net_global_result_map()[net_idx].insert(new_segment);
  }
  pr_model.get_changed_net_set().insert(net_idx);
  return true;
}

bool PlanarRouter::routePRTopoEdgeByPattern(PRModel& pr_model, int32_t topo_idx, Segment<PlanarCoord>& topo_edge, PRShadowDemandMap& shadow_demand_map,
                                            std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  const PRShadowDemandMap* shadow_ptr = shadow_demand_map.empty() ? nullptr : &shadow_demand_map;
  std::vector<PRCandidate> candidate_list = getPRCandidateListByTopo(pr_model, topo_idx, topo_edge, shadow_ptr);
  if (candidate_list.empty()) {
    return false;
  }

#pragma omp parallel for
  for (PRCandidate& pr_candidate : candidate_list) {
    updatePRCandidate(pr_model, pr_candidate, shadow_ptr);
  }

  double corner_weight = pr_model.get_pr_com_param().get_corner_weight();
  PRCandidate* best_candidate = nullptr;
  for (PRCandidate& pr_candidate : candidate_list) {
    if (best_candidate == nullptr || isBetterCandidate(pr_candidate, *best_candidate, corner_weight)) {
      best_candidate = &pr_candidate;
    }
  }
  if (best_candidate == nullptr || best_candidate->get_routing_segment_list().empty()) {
    return false;
  }
  routing_segment_list = best_candidate->get_routing_segment_list();
  addCandidateToShadow(shadow_demand_map, *best_candidate);
  return true;
}

void PlanarRouter::routePRNetTaskListByAStar(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<int32_t> pr_net_task_list = initPRNetTaskList(pr_model, true, false);
  size_t routed_task_num = 0;
  size_t success_task_num = 0;
  for (int32_t net_idx : pr_net_task_list) {
    if (routePRNetTaskByAStar(pr_model, net_idx)) {
      success_task_num++;
    }
    routed_task_num++;
  }
  RTLOG.info(Loc::current(), "Routed ", routed_task_num, " net tasks by A*, success ", success_task_num, ", overflow ", pr_model.get_curr_overflow(),
             ", high_usage ", pr_model.get_curr_high_usage(), monitor.getStatsInfo());
}

std::vector<int32_t> PlanarRouter::initPRNetTaskList(PRModel& pr_model, bool include_high_usage, bool include_changed_net)
{
  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();
  std::set<PlanarCoord, CmpPlanarCoordByXASC> overflow_coord_set;
  std::set<int32_t> overflow_net_set;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> high_usage_coord_set;
  std::set<int32_t> high_usage_net_set;
  collectPRHotspotInfo(pr_model, overflow_coord_set, overflow_net_set, high_usage_coord_set, high_usage_net_set);

  std::set<int32_t> candidate_net_set;
  candidate_net_set.insert(overflow_net_set.begin(), overflow_net_set.end());
  if (include_high_usage) {
    candidate_net_set.insert(high_usage_net_set.begin(), high_usage_net_set.end());
  }
  if (include_changed_net) {
    std::set<int32_t> changed_candidate_net_set = pr_model.get_changed_net_set();
    candidate_net_set.insert(changed_candidate_net_set.begin(), changed_candidate_net_set.end());
    pr_model.get_changed_net_set().clear();
  }

  std::vector<PRNetTask> pr_net_task_list;
  pr_net_task_list.reserve(candidate_net_set.size());
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map = pr_model.get_net_global_result_map();
  for (int32_t net_idx : candidate_net_set) {
    if (!RTUTIL.exist(net_global_result_map, net_idx)) {
      continue;
    }
    if (net_idx < 0 || net_idx >= static_cast<int32_t>(pr_net_list.size())) {
      continue;
    }
    if (pr_net_list[net_idx].get_connect_type() == ConnectType::kClock) {
      continue;
    }
    PRNetTask pr_net_task;
    pr_net_task.net_idx = net_idx;
    for (Segment<LayerCoord>* segment : net_global_result_map[net_idx]) {
      if (segment == nullptr) {
        continue;
      }
      double segment_overflow = getSegmentOverflow(pr_model, segment);
      pr_net_task.overflow += segment_overflow;
      pr_net_task.high_usage += getSegmentHighUsage(pr_model, segment);
      pr_net_task.congestion_risk += getSegmentCongestionRisk(pr_model, segment);
      pr_net_task.max_usage_ratio = std::max(pr_net_task.max_usage_ratio, getSegmentMaxUsageRatio(pr_model, segment));
      pr_net_task.wire_length += RTUTIL.getManhattanDistance(segment->get_first(), segment->get_second());
      if (segment_overflow > RT_ERROR) {
        pr_net_task.overflow_segment_num++;
      }
    }
    if (pr_net_task.overflow <= RT_ERROR && pr_net_task.high_usage <= RT_ERROR) {
      continue;
    }
    pr_net_task_list.push_back(pr_net_task);
  }

  std::sort(pr_net_task_list.begin(), pr_net_task_list.end(), [](PRNetTask& a, PRNetTask& b) {
    if (!RTUTIL.equalDoubleByError(a.overflow, b.overflow, RT_ERROR)) {
      return a.overflow > b.overflow;
    }
    if (!RTUTIL.equalDoubleByError(a.high_usage, b.high_usage, RT_ERROR)) {
      return a.high_usage > b.high_usage;
    }
    if (!RTUTIL.equalDoubleByError(a.congestion_risk, b.congestion_risk, RT_ERROR)) {
      return a.congestion_risk > b.congestion_risk;
    }
    if (!RTUTIL.equalDoubleByError(a.max_usage_ratio, b.max_usage_ratio, RT_ERROR)) {
      return a.max_usage_ratio > b.max_usage_ratio;
    }
    if (a.overflow_segment_num != b.overflow_segment_num) {
      return a.overflow_segment_num > b.overflow_segment_num;
    }
    if (!RTUTIL.equalDoubleByError(a.wire_length, b.wire_length, RT_ERROR)) {
      return a.wire_length > b.wire_length;
    }
    return a.net_idx < b.net_idx;
  });

  std::vector<int32_t> net_idx_list;
  net_idx_list.reserve(pr_net_task_list.size());
  for (PRNetTask& pr_net_task : pr_net_task_list) {
    net_idx_list.push_back(pr_net_task.net_idx);
  }
  RTLOG.info(Loc::current(), "Generated ", net_idx_list.size(), " PR net tasks");
  return net_idx_list;
}

bool PlanarRouter::routePRNetTaskByAStar(PRModel& pr_model, int32_t net_idx)
{
  if (net_idx < 0 || net_idx >= static_cast<int32_t>(pr_model.get_pr_net_list().size())) {
    return false;
  }
  std::vector<Segment<LayerCoord>*> old_segment_ptr_list = getNetGlobalSegmentPtrList(pr_model, net_idx);
  if (old_segment_ptr_list.empty()) {
    return false;
  }
  std::vector<Segment<PlanarCoord>> old_planar_segment_list = getPlanarSegmentList(old_segment_ptr_list);
  if (old_planar_segment_list.empty()) {
    return false;
  }
  PRRouteMetrics old_route_metrics = getRouteMetrics(pr_model, net_idx, old_planar_segment_list);
  if (old_route_metrics.overflow <= RT_ERROR && old_route_metrics.high_usage <= RT_ERROR) {
    return false;
  }

  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, old_planar_segment_list);

  PRNet* origin_curr_pr_task = pr_model.get_curr_pr_task();
  pr_model.set_curr_pr_task(&pr_model.get_pr_net_list()[net_idx]);
  std::vector<Segment<PlanarCoord>> topo_edge_list = getPlanarTopoList(pr_model);
  std::vector<Segment<PlanarCoord>> new_planar_segment_list;
  bool success = true;
  for (Segment<PlanarCoord>& topo_edge : topo_edge_list) {
    std::vector<Segment<PlanarCoord>> edge_segment_list;
    if (!routePRTopoEdgeByAStar(pr_model, net_idx, topo_edge, edge_segment_list) || edge_segment_list.empty()) {
      success = false;
      break;
    }
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, edge_segment_list);
    new_planar_segment_list.insert(new_planar_segment_list.end(), edge_segment_list.begin(), edge_segment_list.end());
  }
  pr_model.set_curr_pr_task(origin_curr_pr_task);
  if (!new_planar_segment_list.empty()) {
    updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, new_planar_segment_list);
  }
  if (!success || new_planar_segment_list.empty()) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
    return false;
  }

  MTree<PlanarCoord> coord_tree;
  {
    PRNet* saved_curr_pr_task = pr_model.get_curr_pr_task();
    pr_model.set_curr_pr_task(&pr_model.get_pr_net_list()[net_idx]);
    coord_tree = getCoordTree(pr_model, new_planar_segment_list);
    pr_model.set_curr_pr_task(saved_curr_pr_task);
  }
  std::vector<Segment<PlanarCoord>> new_tree_segment_list = getPlanarSegmentList(coord_tree);
  if (new_tree_segment_list.empty()) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
    return false;
  }

  std::set<PlanarCoord, CmpPlanarCoordByXASC> affected_coord_set;
  collectSegmentCoordSet(old_planar_segment_list, affected_coord_set);
  collectSegmentCoordSet(new_tree_segment_list, affected_coord_set);

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
  PRLocalRerouteMetrics old_local_metrics = getCoordSetLocalMetrics(pr_model, affected_coord_set);
  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, old_planar_segment_list);

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, new_tree_segment_list);
  PRLocalRerouteMetrics new_local_metrics = getCoordSetLocalMetrics(pr_model, affected_coord_set);
  updateDemandToGraph(pr_model, ChangeType::kDel, net_idx, new_tree_segment_list);

  PRRouteMetrics new_route_metrics = getRouteMetrics(pr_model, net_idx, new_tree_segment_list);
  if (!acceptNetReroute(old_route_metrics, new_route_metrics, old_local_metrics, new_local_metrics)) {
    updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, old_planar_segment_list);
    return false;
  }

  updateDemandToGraph(pr_model, ChangeType::kAdd, net_idx, new_tree_segment_list);
  if (pr_model.get_metric_valid()) {
    pr_model.set_curr_overflow(pr_model.get_curr_overflow() + new_local_metrics.overflow - old_local_metrics.overflow);
    pr_model.set_curr_high_usage(pr_model.get_curr_high_usage() + new_local_metrics.high_usage - old_local_metrics.high_usage);
    pr_model.set_curr_congestion_risk(pr_model.get_curr_congestion_risk() + new_route_metrics.congestion_risk - old_route_metrics.congestion_risk);
    pr_model.set_curr_wire_length(pr_model.get_curr_wire_length() + new_route_metrics.wire_length - old_route_metrics.wire_length);
  }

  pr_model.get_net_global_result_map().erase(net_idx);
  for (Segment<LayerCoord>* old_segment : old_segment_ptr_list) {
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, old_segment);
  }
  for (Segment<PlanarCoord>& new_planar_segment : new_tree_segment_list) {
    Segment<LayerCoord>* new_segment = new Segment<LayerCoord>(LayerCoord(new_planar_segment.get_first(), 0), LayerCoord(new_planar_segment.get_second(), 0));
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, net_idx, new_segment);
    pr_model.get_net_global_result_map()[net_idx].insert(new_segment);
  }
  pr_model.get_changed_net_set().insert(net_idx);
  return true;
}

bool PlanarRouter::routePRTopoEdgeByAStar(PRModel& pr_model, int32_t net_idx, Segment<PlanarCoord>& topo_edge,
                                          std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  PRSegmentTask pr_segment_task;
  pr_segment_task.set_net_idx(net_idx);
  pr_segment_task.set_connect_type(pr_model.get_pr_net_list()[net_idx].get_connect_type());
  pr_segment_task.set_planar_segment(topo_edge);
  pr_segment_task.set_wire_length(RTUTIL.getManhattanDistance(topo_edge.get_first(), topo_edge.get_second()));
  pr_segment_task.set_overflow(1);

  for (PlanarRect& route_window : getRouteWindowList(pr_model, pr_segment_task)) {
    routing_segment_list.clear();
    if (!searchSegmentByAStar(pr_model, pr_segment_task, route_window, routing_segment_list) || routing_segment_list.empty()) {
      continue;
    }
    routing_segment_list = simplifyRoutingSegmentList(routing_segment_list);
    if (!routing_segment_list.empty()) {
      return true;
    }
  }
  return false;
}

std::vector<PlanarRect> PlanarRouter::getRouteWindowList(PRModel& pr_model, PRSegmentTask& pr_segment_task)
{
  PRIterParam& pr_iter_param = pr_model.get_pr_iter_param();
  std::vector<PlanarRect> route_window_list;

  int32_t expand_size = std::max(0, pr_iter_param.get_route_window_base_expand());
  int32_t max_expand_times = std::max(1, pr_iter_param.get_route_window_max_expand_times());
  double expand_ratio = std::max(1.0, pr_iter_param.get_route_window_expand_ratio());
  for (int32_t i = 0; i < max_expand_times; i++) {
    PlanarRect route_window = getRouteWindow(pr_model, pr_segment_task, expand_size);
    if (route_window_list.empty() || route_window_list.back() != route_window) {
      route_window_list.push_back(route_window);
    }
    expand_size = static_cast<int32_t>(std::ceil(expand_size * expand_ratio));
  }
  if (pr_iter_param.get_enable_full_die_fallback()) {
    PlanarRect die_window = getDieWindow(pr_model);
    if (route_window_list.empty() || route_window_list.back() != die_window) {
      route_window_list.push_back(die_window);
    }
  }
  return route_window_list;
}

PlanarRect PlanarRouter::getRouteWindow(PRModel& pr_model, PRSegmentTask& pr_segment_task, int32_t expand_size)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  PlanarCoord& first_coord = pr_segment_task.get_planar_segment().get_first();
  PlanarCoord& second_coord = pr_segment_task.get_planar_segment().get_second();

  int32_t ll_x = std::min(first_coord.get_x(), second_coord.get_x()) - expand_size;
  int32_t ll_y = std::min(first_coord.get_y(), second_coord.get_y()) - expand_size;
  int32_t ur_x = std::max(first_coord.get_x(), second_coord.get_x()) + expand_size;
  int32_t ur_y = std::max(first_coord.get_y(), second_coord.get_y()) + expand_size;

  ll_x = std::max(0, ll_x);
  ll_y = std::max(0, ll_y);
  ur_x = std::min(pr_node_map.get_x_size() - 1, ur_x);
  ur_y = std::min(pr_node_map.get_y_size() - 1, ur_y);

  return PlanarRect(ll_x, ll_y, ur_x, ur_y);
}

PlanarRect PlanarRouter::getDieWindow(PRModel& pr_model)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  return PlanarRect(0, 0, pr_node_map.get_x_size() - 1, pr_node_map.get_y_size() - 1);
}

bool PlanarRouter::searchSegmentByAStar(PRModel& pr_model, PRSegmentTask& pr_segment_task, PlanarRect& route_window,
                                        std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  PlanarCoord start_coord = pr_segment_task.get_planar_segment().get_first();
  PlanarCoord end_coord = pr_segment_task.get_planar_segment().get_second();
  if (!pr_node_map.isInside(start_coord.get_x(), start_coord.get_y()) || !pr_node_map.isInside(end_coord.get_x(), end_coord.get_y())) {
    return false;
  }
  if (!RTUTIL.isInside(route_window, start_coord) || !RTUTIL.isInside(route_window, end_coord)) {
    return false;
  }
  if (start_coord == end_coord) {
    return false;
  }

  PRNode* start_node = &pr_node_map[start_coord.get_x()][start_coord.get_y()];
  PRNode* end_node = &pr_node_map[end_coord.get_x()][end_coord.get_y()];
  std::vector<PRNode*> visited_node_list;
  OpenQueue<PRNode> open_queue;
  PRAStarNodeCostCache node_cost_cache;
  node_cost_cache.route_window = route_window;
  int32_t window_x_size = route_window.get_ur_x() - route_window.get_ll_x() + 1;
  int32_t window_y_size = route_window.get_ur_y() - route_window.get_ll_y() + 1;
  node_cost_cache.cost_map.init(window_x_size, window_y_size, std::array<double, 2>{0.0, 0.0});
  node_cost_cache.valid_map.init(window_x_size, window_y_size, std::array<bool, 2>{false, false});

  initPathHead(pr_model, start_node, end_node, visited_node_list, open_queue);
  PRNode* path_head_node = popFromOpenList(open_queue);
  while (!searchEnded(path_head_node, end_node)) {
    expandSearching(pr_model, pr_segment_task, route_window, path_head_node, end_node, visited_node_list, open_queue, node_cost_cache);
    path_head_node = popFromOpenList(open_queue);
  }
  if (path_head_node == end_node) {
    routing_segment_list = getRoutingSegmentListByNode(path_head_node);
  }
  resetPathState(visited_node_list, open_queue);
  return !routing_segment_list.empty();
}

void PlanarRouter::initPathHead(PRModel& pr_model, PRNode* start_node, PRNode* end_node, std::vector<PRNode*>& visited_node_list, OpenQueue<PRNode>& open_queue)
{
  start_node->set_known_cost(0);
  start_node->set_estimated_cost(getEstimateCost(pr_model, start_node, end_node));
  open_queue.push(start_node);
  start_node->set_state(PRNodeState::kOpen);
  visited_node_list.push_back(start_node);
}

bool PlanarRouter::searchEnded(PRNode* path_head_node, PRNode* end_node)
{
  if (path_head_node == nullptr) {
    return true;
  }
  return path_head_node == end_node;
}

void PlanarRouter::expandSearching(PRModel& pr_model, PRSegmentTask& pr_segment_task, PlanarRect& route_window, PRNode* path_head_node, PRNode* end_node,
                                   std::vector<PRNode*>& visited_node_list, OpenQueue<PRNode>& open_queue, PRAStarNodeCostCache& node_cost_cache)
{
  for (auto& [orientation, neighbor_node] : path_head_node->get_neighbor_node_map()) {
    if (neighbor_node == nullptr || neighbor_node->isClose()) {
      continue;
    }
    if (!RTUTIL.isInside(route_window, *neighbor_node)) {
      continue;
    }
    double known_cost = getKnownCost(pr_model, pr_segment_task, path_head_node, neighbor_node, node_cost_cache);
    if (neighbor_node->isOpen() && known_cost < neighbor_node->get_known_cost()) {
      neighbor_node->set_known_cost(known_cost);
      neighbor_node->set_parent_node(path_head_node);
      open_queue.push(neighbor_node);
    } else if (neighbor_node->isNone()) {
      neighbor_node->set_known_cost(known_cost);
      neighbor_node->set_parent_node(path_head_node);
      neighbor_node->set_estimated_cost(getEstimateCost(pr_model, neighbor_node, end_node));
      open_queue.push(neighbor_node);
      neighbor_node->set_state(PRNodeState::kOpen);
      visited_node_list.push_back(neighbor_node);
    }
  }
}

PRNode* PlanarRouter::popFromOpenList(OpenQueue<PRNode>& open_queue)
{
  PRNode* node = open_queue.pop();
  if (node != nullptr) {
    node->set_state(PRNodeState::kClose);
  }
  return node;
}

void PlanarRouter::resetPathState(std::vector<PRNode*>& visited_node_list, OpenQueue<PRNode>& open_queue)
{
  open_queue.clear();
  for (PRNode* visited_node : visited_node_list) {
    visited_node->set_state(PRNodeState::kNone);
    visited_node->set_parent_node(nullptr);
    visited_node->set_known_cost(0);
    visited_node->set_estimated_cost(0);
  }
  visited_node_list.clear();
}

std::vector<Segment<PlanarCoord>> PlanarRouter::getRoutingSegmentListByNode(PRNode* node)
{
  std::vector<Segment<PlanarCoord>> routing_segment_list;

  PRNode* curr_node = node;
  PRNode* pre_node = curr_node->get_parent_node();
  if (pre_node == nullptr) {
    return routing_segment_list;
  }
  Orientation curr_orientation = RTUTIL.getOrientation(*curr_node, *pre_node);
  while (pre_node->get_parent_node() != nullptr) {
    Orientation pre_orientation = RTUTIL.getOrientation(*pre_node, *pre_node->get_parent_node());
    if (curr_orientation != pre_orientation) {
      routing_segment_list.emplace_back(*curr_node, *pre_node);
      curr_orientation = pre_orientation;
      curr_node = pre_node;
    }
    pre_node = pre_node->get_parent_node();
  }
  routing_segment_list.emplace_back(*curr_node, *pre_node);
  return routing_segment_list;
}

double PlanarRouter::getKnownCost(PRModel& pr_model, PRSegmentTask& pr_segment_task, PRNode* start_node, PRNode* end_node,
                                  PRAStarNodeCostCache& node_cost_cache)
{
  bool exist_neighbor = false;
  for (auto& [orientation, neighbor_ptr] : start_node->get_neighbor_node_map()) {
    if (neighbor_ptr == end_node) {
      exist_neighbor = true;
      break;
    }
  }
  if (!exist_neighbor) {
    RTLOG.error(Loc::current(), "The neighbor not exist!");
  }

  Direction direction = RTUTIL.getDirection(*start_node, *end_node);
  double cost = 0;
  cost += start_node->get_known_cost();
  cost += getNodeCost(pr_model, pr_segment_task, start_node, direction, node_cost_cache);
  cost += getNodeCost(pr_model, pr_segment_task, end_node, direction, node_cost_cache);
  cost += pr_model.get_pr_iter_param().get_wire_unit();
  if (start_node->get_parent_node() != nullptr) {
    Direction pre_direction = RTUTIL.getDirection(*start_node->get_parent_node(), *start_node);
    if (pre_direction != direction) {
      cost += pr_model.get_pr_iter_param().get_corner_unit();
    }
  }
  return cost;
}

double PlanarRouter::getNodeCost(PRModel& pr_model, PRSegmentTask& pr_segment_task, PRNode* curr_node, Direction direction,
                                 PRAStarNodeCostCache& node_cost_cache)
{
  int32_t direction_idx = -1;
  if (direction == Direction::kHorizontal) {
    direction_idx = 0;
  } else if (direction == Direction::kVertical) {
    direction_idx = 1;
  } else {
    RTLOG.error(Loc::current(), "The direction is error!");
  }
  int32_t x = curr_node->get_x();
  int32_t y = curr_node->get_y();
  int32_t cache_x = x - node_cost_cache.route_window.get_ll_x();
  int32_t cache_y = y - node_cost_cache.route_window.get_ll_y();
  if (node_cost_cache.valid_map[cache_x][cache_y][direction_idx]) {
    return node_cost_cache.cost_map[cache_x][cache_y][direction_idx];
  }

  double overflow_unit = pr_model.get_pr_iter_param().get_overflow_unit();
  double congestion_risk_unit = pr_model.get_pr_iter_param().get_congestion_risk_unit();
  double high_usage_unit = pr_model.get_pr_iter_param().get_high_usage_unit();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();
  double node_cost = 0;
  PRNodeCost pr_node_cost = curr_node->getFastCost(pr_segment_task.get_net_idx(), getPRDirectionMask(direction), overflow_unit, false);
  node_cost += pr_node_cost.getTotalCost();
  if (pr_node_cost.max_usage_ratio >= high_usage_ratio_threshold) {
    double high_usage_ratio = (pr_node_cost.max_usage_ratio - high_usage_ratio_threshold) / std::max(RT_ERROR, 1.0 - high_usage_ratio_threshold);
    node_cost += high_usage_unit * std::pow(high_usage_ratio, 2);
  }
  if (pr_segment_task.get_overflow() > RT_ERROR) {
    auto iter = pr_segment_task.get_origin_overflow_penalty_map().find(PlanarCoord(x, y));
    if (iter != pr_segment_task.get_origin_overflow_penalty_map().end()) {
      constexpr double kEscapePenaltyScale = 0.5;
      node_cost += overflow_unit * kEscapePenaltyScale * std::pow(iter->second + 1, 4);
    }
  }
  auto high_usage_iter = pr_segment_task.get_origin_high_usage_penalty_map().find(PlanarCoord(x, y));
  if (high_usage_iter != pr_segment_task.get_origin_high_usage_penalty_map().end()) {
    double normalized_high_usage = high_usage_iter->second / std::max(RT_ERROR, 1.0 - high_usage_ratio_threshold);
    node_cost += high_usage_unit * std::pow(normalized_high_usage, 2);
  }
  node_cost += congestion_risk_unit * curr_node->get_congestion_risk();
  node_cost_cache.cost_map[cache_x][cache_y][direction_idx] = node_cost;
  node_cost_cache.valid_map[cache_x][cache_y][direction_idx] = true;
  return node_cost;
}

double PlanarRouter::getEstimateCost(PRModel& pr_model, PRNode* start_node, PRNode* end_node)
{
  return RTUTIL.getManhattanDistance(*start_node, *end_node) * pr_model.get_pr_iter_param().get_wire_unit();
}

bool PlanarRouter::isSegmentCrossOverflow(PRModel& pr_model, Segment<LayerCoord>* segment, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set)
{
  PlanarCoord first_coord = segment->get_first().get_planar_coord();
  PlanarCoord second_coord = segment->get_second().get_planar_coord();
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      if (RTUTIL.exist(overflow_coord_set, PlanarCoord(x, y))) {
        return true;
      }
    }
  }
  return false;
}

bool PlanarRouter::isSegmentCrossHighUsage(PRModel& pr_model, Segment<LayerCoord>* segment, std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set)
{
  PlanarCoord first_coord = segment->get_first().get_planar_coord();
  PlanarCoord second_coord = segment->get_second().get_planar_coord();
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      if (RTUTIL.exist(high_usage_coord_set, PlanarCoord(x, y))) {
        return true;
      }
    }
  }
  return false;
}

double PlanarRouter::getSegmentOverflow(PRModel& pr_model, Segment<LayerCoord>* segment)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  PlanarCoord first_coord = segment->get_first().get_planar_coord();
  PlanarCoord second_coord = segment->get_second().get_planar_coord();
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  double segment_overflow = 0;
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      if (pr_node_map.isInside(x, y)) {
        segment_overflow += pr_node_map[x][y].getOverflow();
      }
    }
  }
  return segment_overflow;
}

double PlanarRouter::getSegmentCongestionRisk(PRModel& pr_model, Segment<LayerCoord>* segment)
{
  GridMap<double>& congestion_risk_map = pr_model.get_congestion_risk_map();
  if (congestion_risk_map.get_x_size() == 0 || congestion_risk_map.get_y_size() == 0) {
    return 0;
  }
  PlanarCoord first_coord = segment->get_first().get_planar_coord();
  PlanarCoord second_coord = segment->get_second().get_planar_coord();
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  double segment_congestion_risk = 0;
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      if (congestion_risk_map.isInside(x, y)) {
        segment_congestion_risk += congestion_risk_map[x][y];
      }
    }
  }
  return segment_congestion_risk;
}

double PlanarRouter::getSegmentHighUsage(PRModel& pr_model, Segment<LayerCoord>* segment)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  double high_usage_ratio_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();
  PlanarCoord first_coord = segment->get_first().get_planar_coord();
  PlanarCoord second_coord = segment->get_second().get_planar_coord();
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  double segment_high_usage = 0;
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      if (pr_node_map.isInside(x, y)) {
        segment_high_usage += pr_node_map[x][y].getHighUsage(high_usage_ratio_threshold);
      }
    }
  }
  return segment_high_usage;
}

double PlanarRouter::getSegmentMaxUsageRatio(PRModel& pr_model, Segment<LayerCoord>* segment)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  PlanarCoord first_coord = segment->get_first().get_planar_coord();
  PlanarCoord second_coord = segment->get_second().get_planar_coord();
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  double segment_max_usage_ratio = 0;
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      if (pr_node_map.isInside(x, y)) {
        segment_max_usage_ratio = std::max(segment_max_usage_ratio, pr_node_map[x][y].getMaxUsageRatio());
      }
    }
  }
  return segment_max_usage_ratio;
}

void PlanarRouter::updateBestResult(PRModel& pr_model)
{
  std::map<int32_t, std::vector<Segment<LayerCoord>>>& best_net_global_result_map = pr_model.get_best_net_global_result_map();

  double curr_overflow = pr_model.get_metric_valid() ? pr_model.get_curr_overflow() : getOverflow(pr_model);
  double curr_high_usage = pr_model.get_metric_valid() ? pr_model.get_curr_high_usage() : getHighUsage(pr_model);
  double curr_congestion_risk = pr_model.get_metric_valid() ? pr_model.get_curr_congestion_risk() : getCongestionRisk(pr_model);
  double curr_wire_length = pr_model.get_metric_valid() ? pr_model.get_curr_wire_length() : getWireLength(pr_model);
  if (!best_net_global_result_map.empty()) {
    if (pr_model.get_best_overflow() < curr_overflow) {
      return;
    }
    if (RTUTIL.equalDoubleByError(pr_model.get_best_overflow(), curr_overflow, RT_ERROR) && pr_model.get_best_high_usage() < curr_high_usage) {
      return;
    }
    if (RTUTIL.equalDoubleByError(pr_model.get_best_overflow(), curr_overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(pr_model.get_best_high_usage(), curr_high_usage, RT_ERROR) && pr_model.get_best_congestion_risk() < curr_congestion_risk) {
      return;
    }
    if (RTUTIL.equalDoubleByError(pr_model.get_best_overflow(), curr_overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(pr_model.get_best_high_usage(), curr_high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(pr_model.get_best_congestion_risk(), curr_congestion_risk, RT_ERROR)
        && pr_model.get_best_wire_length() < curr_wire_length) {
      return;
    }
  }

  best_net_global_result_map.clear();
  for (auto& [net_idx, segment_set] : pr_model.get_net_global_result_map()) {
    for (Segment<LayerCoord>* segment : segment_set) {
      best_net_global_result_map[net_idx].push_back(*segment);
    }
  }
  pr_model.set_best_overflow(curr_overflow);
  pr_model.set_best_high_usage(curr_high_usage);
  pr_model.set_best_congestion_risk(curr_congestion_risk);
  pr_model.set_best_wire_length(curr_wire_length);
}

void PlanarRouter::uploadBestResult(PRModel& pr_model)
{
  Die& die = RTDM.getDatabase().get_die();
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, segment);
    }
  }
  for (auto& [net_idx, segment_list] : pr_model.get_best_net_global_result_map()) {
    for (Segment<LayerCoord>& segment : segment_list) {
      RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, net_idx, new Segment<LayerCoord>(segment));
    }
  }
  initNetGlobalResultMap(pr_model);
}

#if 1  // update env

void PlanarRouter::updateDemandToGraph(PRModel& pr_model, ChangeType change_type, MTree<PlanarCoord>& coord_tree)
{
  int32_t curr_net_idx = pr_model.get_curr_pr_task()->get_net_idx();

  std::vector<Segment<PlanarCoord>> routing_segment_list;
  for (Segment<TNode<PlanarCoord>*>& coord_segment : RTUTIL.getSegListByTree(coord_tree)) {
    routing_segment_list.emplace_back(coord_segment.get_first()->value(), coord_segment.get_second()->value());
  }
  std::map<PlanarCoord, std::set<Orientation>, CmpPlanarCoordByXASC> usage_map;
  for (Segment<PlanarCoord>& coord_segment : routing_segment_list) {
    PlanarCoord& first_coord = coord_segment.get_first();
    PlanarCoord& second_coord = coord_segment.get_second();

    Orientation orientation = RTUTIL.getOrientation(first_coord, second_coord);
    if (orientation == Orientation::kNone || orientation == Orientation::kOblique) {
      RTLOG.error(Loc::current(), "The orientation is error!");
    }
    Orientation opposite_orientation = RTUTIL.getOppositeOrientation(orientation);

    int32_t first_x = first_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t second_x = second_coord.get_x();
    int32_t second_y = second_coord.get_y();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);

    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        PlanarCoord coord(x, y);
        if (coord != first_coord) {
          usage_map[coord].insert(opposite_orientation);
        }
        if (coord != second_coord) {
          usage_map[coord].insert(orientation);
        }
      }
    }
  }
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  for (auto& [usage_coord, orientation_list] : usage_map) {
    PRNode& pr_node = pr_node_map[usage_coord.get_x()][usage_coord.get_y()];
    pr_node.updateDemand(curr_net_idx, orientation_list, change_type);
  }
}

void PlanarRouter::updateDemandToGraph(PRModel& pr_model, ChangeType change_type, int32_t net_idx, std::vector<Segment<PlanarCoord>>& segment_list)
{
  std::map<PlanarCoord, std::set<Orientation>, CmpPlanarCoordByXASC> usage_map;
  for (Segment<PlanarCoord>& coord_segment : segment_list) {
    PlanarCoord& first_coord = coord_segment.get_first();
    PlanarCoord& second_coord = coord_segment.get_second();

    Orientation orientation = RTUTIL.getOrientation(first_coord, second_coord);
    if (orientation == Orientation::kNone || orientation == Orientation::kOblique) {
      RTLOG.error(Loc::current(), "The orientation is error!");
    }
    Orientation opposite_orientation = RTUTIL.getOppositeOrientation(orientation);

    int32_t first_x = first_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t second_x = second_coord.get_x();
    int32_t second_y = second_coord.get_y();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);

    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        PlanarCoord coord(x, y);
        if (coord != first_coord) {
          usage_map[coord].insert(opposite_orientation);
        }
        if (coord != second_coord) {
          usage_map[coord].insert(orientation);
        }
      }
    }
  }
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  for (auto& [usage_coord, orientation_list] : usage_map) {
    PRNode& pr_node = pr_node_map[usage_coord.get_x()][usage_coord.get_y()];
    pr_node.updateDemand(net_idx, orientation_list, change_type);
  }
}

void PlanarRouter::addCandidateToShadow(PRShadowDemandMap& shadow_map, PRCandidate& pr_candidate)
{
  for (Segment<PlanarCoord>& coord_segment : pr_candidate.get_routing_segment_list()) {
    PlanarCoord& first_coord = coord_segment.get_first();
    PlanarCoord& second_coord = coord_segment.get_second();

    Orientation orientation = RTUTIL.getOrientation(first_coord, second_coord);
    if (orientation == Orientation::kNone || orientation == Orientation::kOblique) {
      RTLOG.error(Loc::current(), "The orientation is error!");
    }
    Orientation opposite_orientation = RTUTIL.getOppositeOrientation(orientation);
    uint8_t orientation_mask = getPROrientMask(orientation);
    uint8_t opposite_orientation_mask = getPROrientMask(opposite_orientation);

    int32_t first_x = first_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t second_x = second_coord.get_x();
    int32_t second_y = second_coord.get_y();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);

    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        PlanarCoord coord(x, y);
        uint8_t add_mask = 0;
        if (coord != first_coord) {
          add_mask |= opposite_orientation_mask;
        }
        if (coord != second_coord) {
          add_mask |= orientation_mask;
        }
        shadow_map.addMask(x, y, add_mask);
      }
    }
  }
}

#endif

#if 1  // exhibit

void PlanarRouter::updateSummary(PRModel& pr_model)
{
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  Summary& summary = RTDM.getDatabase().get_summary();
  int32_t enable_timing = RTDM.getConfig().enable_timing;

  double& total_demand = summary.pr_summary.total_demand;
  double& total_overflow = summary.pr_summary.total_overflow;
  double& total_wire_length = summary.pr_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.pr_summary.clock_timing_map;

  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();

  total_demand = 0;
  total_overflow = 0;
  total_wire_length = 0;
  clock_timing_map.clear();

  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      double node_demand = pr_node_map[x][y].getDemand();
      double node_overflow = pr_node_map[x][y].getOverflow();
      total_demand += node_demand;
      total_overflow += node_overflow;
    }
  }
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      LayerCoord& first_coord = segment->get_first();
      int32_t first_layer_idx = first_coord.get_layer_idx();
      LayerCoord& second_coord = segment->get_second();
      int32_t second_layer_idx = second_coord.get_layer_idx();

      if (first_layer_idx == second_layer_idx) {
        GCell& first_gcell = gcell_map[first_coord.get_x()][first_coord.get_y()];
        GCell& second_gcell = gcell_map[second_coord.get_x()][second_coord.get_y()];
        double wire_length = RTUTIL.getManhattanDistance(first_gcell.getMidPoint(), second_gcell.getMidPoint()) / 1.0 / micron_dbu;
        total_wire_length += wire_length;
      } else {
        RTLOG.error(Loc::current(), "first_layer_idx != second_layer_idx!");
      }
    }
  }
  if (enable_timing) {
    std::vector<std::map<std::string, std::vector<LayerCoord>>> real_pin_coord_map_list;
    real_pin_coord_map_list.resize(pr_net_list.size());
    std::vector<std::vector<Segment<LayerCoord>>> routing_segment_list_list;
    routing_segment_list_list.resize(pr_net_list.size());
    for (PRNet& pr_net : pr_net_list) {
      for (PRPin& pr_pin : pr_net.get_pr_pin_list()) {
        LayerCoord layer_coord = pr_pin.get_access_point().getGridLayerCoord();
        real_pin_coord_map_list[pr_net.get_net_idx()][pr_pin.get_pin_name()].emplace_back(RTUTIL.getRealRectByGCell(layer_coord, gcell_axis).getMidPoint(), 0);
      }
    }
    for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
      for (Segment<LayerCoord>* segment : segment_set) {
        LayerCoord first_layer_coord = segment->get_first();
        LayerCoord first_real_coord(RTUTIL.getRealRectByGCell(first_layer_coord, gcell_axis).getMidPoint(), first_layer_coord.get_layer_idx());
        LayerCoord second_layer_coord = segment->get_second();
        LayerCoord second_real_coord(RTUTIL.getRealRectByGCell(second_layer_coord, gcell_axis).getMidPoint(), second_layer_coord.get_layer_idx());

        routing_segment_list_list[net_idx].emplace_back(first_real_coord, second_real_coord);
      }
    }
    RTI.updateTiming(real_pin_coord_map_list, routing_segment_list_list, clock_timing_map);
  }
}

void PlanarRouter::printSummary(PRModel& pr_model)
{
  Summary& summary = RTDM.getDatabase().get_summary();
  int32_t enable_timing = RTDM.getConfig().enable_timing;

  double& total_demand = summary.pr_summary.total_demand;
  double& total_overflow = summary.pr_summary.total_overflow;
  double& total_wire_length = summary.pr_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.pr_summary.clock_timing_map;

  fort::char_table summary_table;
  {
    summary_table.set_cell_text_align(fort::text_align::right);
    summary_table << fort::header << "total_demand" << total_demand << fort::endr;
    summary_table << fort::header << "total_overflow" << total_overflow << fort::endr;
    summary_table << fort::header << "total_wire_length" << total_wire_length << fort::endr;
  }
  fort::char_table timing_table;
  timing_table.set_cell_text_align(fort::text_align::right);
  if (enable_timing) {
    timing_table << fort::header << "clock_name"
                 << "tns"
                 << "wns"
                 << "freq" << fort::endr;
    for (auto& [clock_name, timing_map] : clock_timing_map) {
      timing_table << clock_name << timing_map["TNS"] << timing_map["WNS"] << timing_map["Freq(MHz)"] << fort::endr;
    }
  }
  RTUTIL.printTableList({summary_table});
  RTUTIL.printTableList({timing_table});
}

void PlanarRouter::outputGuide(PRModel& pr_model)
{
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();

  std::ofstream* guide_file_stream = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, "route.guide"));
  if (guide_file_stream == nullptr) {
    return;
  }
  RTUTIL.pushStream(guide_file_stream, "guide net_name\n");
  RTUTIL.pushStream(guide_file_stream, "pin grid_x grid_y real_x real_y layer energy name\n");
  RTUTIL.pushStream(guide_file_stream, "wire grid1_x grid1_y grid2_x grid2_y real1_x real1_y real2_x real2_y layer\n");
  RTUTIL.pushStream(guide_file_stream, "via grid_x grid_y real_x real_y layer1 layer2\n");

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    PRNet& pr_net = pr_net_list[net_idx];
    RTUTIL.pushStream(guide_file_stream, "guide ", pr_net.get_origin_net()->get_net_name(), "\n");

    for (PRPin& pr_pin : pr_net.get_pr_pin_list()) {
      AccessPoint& access_point = pr_pin.get_access_point();
      double grid_x = access_point.get_grid_x();
      double grid_y = access_point.get_grid_y();
      double real_x = access_point.get_real_x() / 1.0 / micron_dbu;
      double real_y = access_point.get_real_y() / 1.0 / micron_dbu;
      std::string layer = routing_layer_list[access_point.get_layer_idx()].get_layer_name();
      std::string connnect;
      if (pr_pin.get_is_driven()) {
        connnect = "driven";
      } else {
        connnect = "load";
      }
      RTUTIL.pushStream(guide_file_stream, "pin ", grid_x, " ", grid_y, " ", real_x, " ", real_y, " ", layer, " ", connnect, " ", pr_pin.get_pin_name(), "\n");
    }
    for (Segment<LayerCoord>* segment : segment_set) {
      LayerCoord first_layer_coord = segment->get_first();
      double grid1_x = first_layer_coord.get_x();
      double grid1_y = first_layer_coord.get_y();
      int32_t first_layer_idx = first_layer_coord.get_layer_idx();

      PlanarCoord first_mid_coord = RTUTIL.getRealRectByGCell(first_layer_coord, gcell_axis).getMidPoint();
      double real1_x = first_mid_coord.get_x() / 1.0 / micron_dbu;
      double real1_y = first_mid_coord.get_y() / 1.0 / micron_dbu;

      LayerCoord second_layer_coord = segment->get_second();
      double grid2_x = second_layer_coord.get_x();
      double grid2_y = second_layer_coord.get_y();
      int32_t second_layer_idx = second_layer_coord.get_layer_idx();

      PlanarCoord second_mid_coord = RTUTIL.getRealRectByGCell(second_layer_coord, gcell_axis).getMidPoint();
      double real2_x = second_mid_coord.get_x() / 1.0 / micron_dbu;
      double real2_y = second_mid_coord.get_y() / 1.0 / micron_dbu;

      if (first_layer_idx != second_layer_idx) {
        RTUTIL.swapByASC(first_layer_idx, second_layer_idx);
        std::string layer1 = routing_layer_list[first_layer_idx].get_layer_name();
        std::string layer2 = routing_layer_list[second_layer_idx].get_layer_name();
        RTUTIL.pushStream(guide_file_stream, "via ", grid1_x, " ", grid1_y, " ", real1_x, " ", real1_y, " ", layer1, " ", layer2, "\n");
      } else {
        std::string layer = routing_layer_list[first_layer_idx].get_layer_name();
        RTUTIL.pushStream(guide_file_stream, "wire ", grid1_x, " ", grid1_y, " ", grid2_x, " ", grid2_y, " ", real1_x, " ", real1_y, " ", real2_x, " ", real2_y,
                          " ", layer, "\n");
      }
    }
  }
  RTUTIL.closeFileStream(guide_file_stream);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputNetCSV(PRModel& pr_model)
{
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::ofstream* net_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, "net_map.csv"));
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  for (int32_t y = pr_node_map.get_y_size() - 1; y >= 0; y--) {
    for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
      RTUTIL.pushStream(net_csv_file, pr_node_map[x][y].getDemand(), ",");
    }
    RTUTIL.pushStream(net_csv_file, "\n");
  }
  RTUTIL.closeFileStream(net_csv_file);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputOverflowCSV(PRModel& pr_model)
{
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::ofstream* overflow_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, "overflow_map.csv"));
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  for (int32_t y = pr_node_map.get_y_size() - 1; y >= 0; y--) {
    for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
      RTUTIL.pushStream(overflow_csv_file, pr_node_map[x][y].getOverflow(), ",");
    }
    RTUTIL.pushStream(overflow_csv_file, "\n");
  }
  RTUTIL.closeFileStream(overflow_csv_file);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputCongestionSnapshotCSV(PRModel& pr_model, const std::string& suffix, int32_t iter)
{
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }

  GridMap<double> saved_congestion_risk_map = pr_model.get_congestion_risk_map();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  GridMap<double> saved_node_congestion_risk_map;
  saved_node_congestion_risk_map.init(pr_node_map.get_x_size(), pr_node_map.get_y_size(), 0.0);
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      saved_node_congestion_risk_map[x][y] = pr_node_map[x][y].get_congestion_risk();
    }
  }
  double saved_curr_congestion_risk = pr_model.get_curr_congestion_risk();

  updateCongestionRisk(pr_model);
  if (pr_model.get_metric_valid()) {
    pr_model.set_curr_congestion_risk(getCongestionRisk(pr_model));
  }
  outputCongestionCSV(pr_model, suffix, iter);

  pr_model.set_congestion_risk_map(saved_congestion_risk_map);
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      pr_node_map[x][y].set_congestion_risk(saved_node_congestion_risk_map[x][y]);
    }
  }
  pr_model.set_curr_congestion_risk(saved_curr_congestion_risk);
}

void PlanarRouter::outputCongestionCSV(PRModel& pr_model, const std::string& suffix, int32_t iter)
{
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  constexpr int32_t kMaxNetListSize = 64;
  bool output_full = (output_inter_result >= 2);
  double high_usage_threshold = pr_model.get_pr_iter_param().get_high_usage_ratio_threshold();
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();

  auto calcUsageRatio = [](double demand, double supply) {
    if (supply <= 0) {
      return demand <= 0 ? 0.0 : demand + 1.0;
    }
    return demand / supply;
  };
  auto joinNetSet = [&](const std::set<int32_t>& net_set) {
    std::string net_list_str;
    int32_t output_num = 0;
    for (int32_t net_idx : net_set) {
      if (output_num >= kMaxNetListSize) {
        net_list_str += "|...";
        break;
      }
      if (!net_list_str.empty()) {
        net_list_str += "|";
      }
      net_list_str += std::to_string(net_idx);
      output_num++;
    }
    return net_list_str;
  };
  auto getDemandNetSet = [](PRNode& pr_node, Orientation orient) {
    std::set<int32_t> net_set;
    if (RTUTIL.exist(pr_node.get_orient_net_map(), orient)) {
      for (int32_t net_idx : pr_node.get_orient_net_map()[orient]) {
        if (RTUTIL.exist(pr_node.get_ignore_net_orient_map(), net_idx) && RTUTIL.exist(pr_node.get_ignore_net_orient_map()[net_idx], orient)) {
          continue;
        }
        net_set.insert(net_idx);
      }
    }
    return net_set;
  };
  auto getInternalDemand = [](PRNode& pr_node, std::set<int32_t>& internal_net_set) {
    double demand = 0;
    std::set<int32_t> net_set;
    for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
      if (RTUTIL.exist(pr_node.get_orient_net_map(), orient)) {
        for (int32_t net_idx : pr_node.get_orient_net_map()[orient]) {
          if (RTUTIL.exist(pr_node.get_ignore_net_orient_map(), net_idx) && RTUTIL.exist(pr_node.get_ignore_net_orient_map()[net_idx], orient)) {
            continue;
          }
          demand += pr_node.get_internal_wire_unit();
          internal_net_set.insert(net_idx);
        }
      }
    }
    return demand;
  };
  auto getSupply = [](PRNode& pr_node, Orientation orient) {
    if (RTUTIL.exist(pr_node.get_orient_supply_map(), orient)) {
      return static_cast<double>(pr_node.get_orient_supply_map()[orient]);
    }
    return 0.0;
  };
  auto getInternalSupply = [](PRNode& pr_node) {
    double supply = 0;
    for (auto& [orient, orient_supply] : pr_node.get_orient_supply_map()) {
      if (orient == Orientation::kEast || orient == Orientation::kWest || orient == Orientation::kSouth || orient == Orientation::kNorth) {
        supply += orient_supply;
      }
    }
    return supply;
  };
  auto pushHeader = [](std::ofstream* csv_file) {
    RTUTIL.pushStream(csv_file,
                      "stage,iter,layer_idx,layer_name,x,y,real_llx,real_lly,real_urx,real_ury,resource,orient,demand,supply,overflow,"
                      "usage_ratio,node_total_demand,node_total_overflow,node_max_usage_ratio,high_usage,congestion_risk,net_count,"
                      "overflow_net_count,high_usage_net_count,net_list,overflow_net_list,high_usage_net_list\n");
  };
  auto pushRow = [&](std::ofstream* csv_file, bool include_all, PRNode& pr_node, const std::string& resource, const std::string& orient_name, double demand,
                     double supply, const std::set<int32_t>& net_set, const std::set<int32_t>& overflow_net_set, const std::set<int32_t>& high_usage_net_set) {
    double usage_ratio = calcUsageRatio(demand, supply);
    double overflow = std::max(0.0, demand - supply);
    if (!include_all && overflow <= 0 && usage_ratio < high_usage_threshold && pr_node.get_congestion_risk() <= 0) {
      return;
    }
    PlanarRect real_rect = RTUTIL.getRealRectByGCell(pr_node, gcell_axis);
    RTUTIL.pushStream(csv_file, "PR,", iter, ",-1,all,", pr_node.get_x(), ",", pr_node.get_y(), ",", real_rect.get_ll_x() / 1.0 / micron_dbu, ",",
                      real_rect.get_ll_y() / 1.0 / micron_dbu, ",", real_rect.get_ur_x() / 1.0 / micron_dbu, ",", real_rect.get_ur_y() / 1.0 / micron_dbu, ",",
                      resource, ",", orient_name, ",", demand, ",", supply, ",", overflow, ",", usage_ratio, ",", pr_node.getDemand(), ",",
                      pr_node.getOverflow(), ",", pr_node.getMaxUsageRatio(), ",", pr_node.getHighUsage(high_usage_threshold), ",",
                      pr_node.get_congestion_risk(), ",", net_set.size(), ",", overflow_net_set.size(), ",", high_usage_net_set.size(), ",",
                      joinNetSet(net_set), ",", joinNetSet(overflow_net_set), ",", joinNetSet(high_usage_net_set), "\n");
  };

  std::ofstream* hotspot_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, "congestion_hotspot_PR", suffix, ".csv"));
  pushHeader(hotspot_csv_file);
  std::ofstream* full_csv_file = nullptr;
  if (output_full) {
    full_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, "congestion_full_PR", suffix, ".csv"));
    pushHeader(full_csv_file);
  }
  auto pushToFiles = [&](PRNode& pr_node, const std::string& resource, const std::string& orient_name, double demand, double supply,
                         const std::set<int32_t>& net_set, const std::set<int32_t>& overflow_net_set, const std::set<int32_t>& high_usage_net_set) {
    pushRow(hotspot_csv_file, false, pr_node, resource, orient_name, demand, supply, net_set, overflow_net_set, high_usage_net_set);
    if (full_csv_file != nullptr) {
      pushRow(full_csv_file, true, pr_node, resource, orient_name, demand, supply, net_set, overflow_net_set, high_usage_net_set);
    }
  };

  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      PRNode& pr_node = pr_node_map[x][y];
      std::set<int32_t> overflow_net_set = pr_node.getOverflowNetSet();
      std::set<int32_t> high_usage_net_set = pr_node.getHighUsageNetSet(high_usage_threshold);
      for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
        std::set<int32_t> net_set = getDemandNetSet(pr_node, orient);
        double demand = net_set.size() * pr_node.get_boundary_wire_unit();
        pushToFiles(pr_node, "boundary", GetOrientationName()(orient), demand, getSupply(pr_node, orient), net_set, overflow_net_set, high_usage_net_set);
      }
      std::set<int32_t> internal_net_set;
      double internal_demand = getInternalDemand(pr_node, internal_net_set);
      pushToFiles(pr_node, "internal", "internal", internal_demand, getInternalSupply(pr_node), internal_net_set, overflow_net_set, high_usage_net_set);
    }
  }

  RTUTIL.closeFileStream(hotspot_csv_file);
  if (full_csv_file != nullptr) {
    RTUTIL.closeFileStream(full_csv_file);
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputJson(PRModel& pr_model)
{
  int32_t enable_notification = RTDM.getConfig().enable_notification;
  if (!enable_notification) {
    return;
  }
  std::map<std::string, std::string> json_path_map;
  json_path_map["net_map"] = outputNetJson(pr_model);
  json_path_map["overflow_map"] = outputOverflowJson(pr_model);
  json_path_map["summary"] = outputSummaryJson(pr_model);
  RTI.sendNotification("PR", 1, json_path_map);
}

std::string PlanarRouter::outputNetJson(PRModel& pr_model)
{
  Die& die = RTDM.getDatabase().get_die();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  std::vector<nlohmann::json> net_json_list;
  {
    nlohmann::json result_shape_json;
    for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
      std::string net_name = net_list[net_idx].get_net_name();
      for (Segment<LayerCoord>* segment : segment_set) {
        PlanarRect first_gcell = RTUTIL.getRealRectByGCell(segment->get_first(), gcell_axis);
        PlanarRect second_gcell = RTUTIL.getRealRectByGCell(segment->get_second(), gcell_axis);
        if (segment->get_first().get_layer_idx() != segment->get_second().get_layer_idx()) {
          result_shape_json["result_shape"][net_name]["path"].push_back({first_gcell.get_ll_x(), first_gcell.get_ll_y(), first_gcell.get_ur_x(),
                                                                         first_gcell.get_ur_y(),
                                                                         routing_layer_list[segment->get_first().get_layer_idx()].get_layer_name()});
          result_shape_json["result_shape"][net_name]["path"].push_back({second_gcell.get_ll_x(), second_gcell.get_ll_y(), second_gcell.get_ur_x(),
                                                                         second_gcell.get_ur_y(),
                                                                         routing_layer_list[segment->get_second().get_layer_idx()].get_layer_name()});
        } else {
          PlanarRect gcell = RTUTIL.getBoundingBox({first_gcell, second_gcell});
          result_shape_json["result_shape"][net_name]["path"].push_back({gcell.get_ll_x(), gcell.get_ll_y(), gcell.get_ur_x(), gcell.get_ur_y(),
                                                                         routing_layer_list[segment->get_first().get_layer_idx()].get_layer_name()});
        }
      }
    }
    net_json_list.push_back(result_shape_json);
  }
  std::string net_json_file_path = RTUTIL.getString(pr_temp_directory_path, "net_map.json");
  std::ofstream* net_json_file = RTUTIL.getOutputFileStream(net_json_file_path);
  (*net_json_file) << net_json_list;
  RTUTIL.closeFileStream(net_json_file);
  return net_json_file_path;
}

std::string PlanarRouter::outputOverflowJson(PRModel& pr_model)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  std::vector<nlohmann::json> overflow_json_list;
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      PlanarRect gcell = RTUTIL.getRealRectByGCell(PlanarCoord(x, y), gcell_axis);
      overflow_json_list.push_back(
          {gcell.get_ll_x(), gcell.get_ll_y(), gcell.get_ur_x(), gcell.get_ur_y(), routing_layer_list[0].get_layer_name(), pr_node_map[x][y].getOverflow()});
    }
  }
  std::string overflow_json_file_path = RTUTIL.getString(pr_temp_directory_path, "overflow_map.json");
  std::ofstream* overflow_json_file = RTUTIL.getOutputFileStream(overflow_json_file_path);
  (*overflow_json_file) << overflow_json_list;
  RTUTIL.closeFileStream(overflow_json_file);
  return overflow_json_file_path;
}

std::string PlanarRouter::outputSummaryJson(PRModel& pr_model)
{
  Summary& summary = RTDM.getDatabase().get_summary();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  double& total_demand = summary.pr_summary.total_demand;
  double& total_overflow = summary.pr_summary.total_overflow;
  double& total_wire_length = summary.pr_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.pr_summary.clock_timing_map;

  nlohmann::json summary_json;
  summary_json["total_demand"] = total_demand;
  summary_json["total_overflow"] = total_overflow;
  summary_json["total_wire_length"] = total_wire_length;
  for (auto& [clock_name, timing] : clock_timing_map) {
    summary_json["clock_timing_map"]["clock_name"] = clock_name;
    summary_json["clock_timing_map"]["timing"] = timing;
  }
  std::string summary_json_file_path = RTUTIL.getString(pr_temp_directory_path, "summary.json");
  std::ofstream* summary_json_file = RTUTIL.getOutputFileStream(summary_json_file_path);
  (*summary_json_file) << summary_json;
  RTUTIL.closeFileStream(summary_json_file);
  return summary_json_file_path;
}

#endif

#if 1  // debug

void PlanarRouter::debugPlotPRModel(PRModel& pr_model, std::string flag)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  int32_t point_size = 5;

  GPGDS gp_gds;

  // base_region
  {
    GPStruct base_region_struct("base_region");
    GPBoundary gp_boundary;
    gp_boundary.set_layer_idx(0);
    gp_boundary.set_data_type(0);
    gp_boundary.set_rect(die.get_real_rect());
    base_region_struct.push(gp_boundary);
    gp_gds.addStruct(base_region_struct);
  }

  // gcell_axis
  {
    GPStruct gcell_axis_struct("gcell_axis");
    std::vector<int32_t> gcell_x_list = RTUTIL.getScaleList(die.get_real_ll_x(), die.get_real_ur_x(), gcell_axis.get_x_grid_list());
    std::vector<int32_t> gcell_y_list = RTUTIL.getScaleList(die.get_real_ll_y(), die.get_real_ur_y(), gcell_axis.get_y_grid_list());
    for (int32_t x : gcell_x_list) {
      GPPath gp_path;
      gp_path.set_layer_idx(0);
      gp_path.set_data_type(1);
      gp_path.set_segment(x, die.get_real_ll_y(), x, die.get_real_ur_y());
      gcell_axis_struct.push(gp_path);
    }
    for (int32_t y : gcell_y_list) {
      GPPath gp_path;
      gp_path.set_layer_idx(0);
      gp_path.set_data_type(1);
      gp_path.set_segment(die.get_real_ll_x(), y, die.get_real_ur_x(), y);
      gcell_axis_struct.push(gp_path);
    }
    gp_gds.addStruct(gcell_axis_struct);
  }

  // track_axis_struct
  {
    GPStruct track_axis_struct("track_axis_struct");
    for (RoutingLayer& routing_layer : routing_layer_list) {
      std::vector<int32_t> x_list = RTUTIL.getScaleList(die.get_real_ll_x(), die.get_real_ur_x(), routing_layer.getXTrackGridList());
      std::vector<int32_t> y_list = RTUTIL.getScaleList(die.get_real_ll_y(), die.get_real_ur_y(), routing_layer.getYTrackGridList());
      for (int32_t x : x_list) {
        GPPath gp_path;
        gp_path.set_data_type(static_cast<int32_t>(GPDataType::kAxis));
        gp_path.set_segment(x, die.get_real_ll_y(), x, die.get_real_ur_y());
        gp_path.set_layer_idx(RTGP.getGDSIdxByRouting(routing_layer.get_layer_idx()));
        track_axis_struct.push(gp_path);
      }
      for (int32_t y : y_list) {
        GPPath gp_path;
        gp_path.set_data_type(static_cast<int32_t>(GPDataType::kAxis));
        gp_path.set_segment(die.get_real_ll_x(), y, die.get_real_ur_x(), y);
        gp_path.set_layer_idx(RTGP.getGDSIdxByRouting(routing_layer.get_layer_idx()));
        track_axis_struct.push(gp_path);
      }
    }
    gp_gds.addStruct(track_axis_struct);
  }

  // fixed_rect
  for (auto& [is_routing, layer_net_rect_map] : RTDM.getTypeLayerNetFixedRectMap(die)) {
    for (auto& [layer_idx, net_rect_map] : layer_net_rect_map) {
      for (auto& [net_idx, rect_set] : net_rect_map) {
        GPStruct fixed_rect_struct(RTUTIL.getString("fixed_rect(net_", net_idx, ")"));
        for (EXTLayerRect* rect : rect_set) {
          GPBoundary gp_boundary;
          gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kShape));
          gp_boundary.set_rect(rect->get_real_rect());
          if (is_routing) {
            gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(layer_idx));
          } else {
            gp_boundary.set_layer_idx(RTGP.getGDSIdxByCut(layer_idx));
          }
          fixed_rect_struct.push(gp_boundary);
        }
        gp_gds.addStruct(fixed_rect_struct);
      }
    }
  }

  // access_point
  for (auto& [net_idx, access_point_set] : RTDM.getNetAccessPointMap(die)) {
    GPStruct access_point_struct(RTUTIL.getString("access_point(net_", net_idx, ")"));
    for (AccessPoint* access_point : access_point_set) {
      int32_t x = access_point->get_real_x();
      int32_t y = access_point->get_real_y();

      GPBoundary access_point_boundary;
      access_point_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(access_point->get_layer_idx()));
      access_point_boundary.set_data_type(static_cast<int32_t>(GPDataType::kAccessPoint));
      access_point_boundary.set_rect(x - point_size, y - point_size, x + point_size, y + point_size);
      access_point_struct.push(access_point_boundary);
    }
    gp_gds.addStruct(access_point_struct);
  }

  // routing result
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    GPStruct global_result_struct(RTUTIL.getString("global_result(net_", net_idx, ")"));
    for (Segment<LayerCoord>* segment : segment_set) {
      for (NetShape& net_shape : RTDM.getNetGlobalShapeList(net_idx, *segment)) {
        GPBoundary gp_boundary;
        gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kGlobalPath));
        gp_boundary.set_rect(net_shape.get_rect());
        if (net_shape.get_is_routing()) {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(net_shape.get_layer_idx()));
        } else {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByCut(net_shape.get_layer_idx()));
        }
        global_result_struct.push(gp_boundary);
      }
    }
    gp_gds.addStruct(global_result_struct);
  }

  // routing result
  for (auto& [net_idx, segment_set] : RTDM.getNetDetailedResultMap(die)) {
    GPStruct detailed_result_struct(RTUTIL.getString("detailed_result(net_", net_idx, ")"));
    for (Segment<LayerCoord>* segment : segment_set) {
      for (NetShape& net_shape : RTDM.getNetDetailedShapeList(net_idx, *segment)) {
        GPBoundary gp_boundary;
        gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kShape));
        gp_boundary.set_rect(net_shape.get_rect());
        if (net_shape.get_is_routing()) {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(net_shape.get_layer_idx()));
        } else {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByCut(net_shape.get_layer_idx()));
        }
        detailed_result_struct.push(gp_boundary);
      }
    }
    gp_gds.addStruct(detailed_result_struct);
  }

  // routing patch
  for (auto& [net_idx, patch_set] : RTDM.getNetDetailedPatchMap(die)) {
    GPStruct detailed_patch_struct(RTUTIL.getString("detailed_patch(net_", net_idx, ")"));
    for (EXTLayerRect* patch : patch_set) {
      GPBoundary gp_boundary;
      gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kShape));
      gp_boundary.set_rect(patch->get_real_rect());
      gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(patch->get_layer_idx()));
      detailed_patch_struct.push(gp_boundary);
    }
    gp_gds.addStruct(detailed_patch_struct);
  }

  {
    GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
    // pr_node_map
    {
      GPStruct pr_node_map_struct("pr_node_map");
      for (int32_t grid_x = 0; grid_x < pr_node_map.get_x_size(); grid_x++) {
        for (int32_t grid_y = 0; grid_y < pr_node_map.get_y_size(); grid_y++) {
          PRNode& pr_node = pr_node_map[grid_x][grid_y];
          PlanarRect real_rect = RTUTIL.getRealRectByGCell(pr_node, gcell_axis);
          int32_t y_reduced_span = std::max(1, real_rect.getYSpan() / 12);
          int32_t y = real_rect.get_ur_y();

          y -= y_reduced_span;
          GPText gp_text_node_real_coord;
          gp_text_node_real_coord.set_coord(real_rect.get_ll_x(), y);
          gp_text_node_real_coord.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_node_real_coord.set_message(RTUTIL.getString("(", pr_node.get_x(), " , ", pr_node.get_y(), " , ", 0, ")"));
          gp_text_node_real_coord.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_node_real_coord.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_node_real_coord);

          y -= y_reduced_span;
          GPText gp_text_node_grid_coord;
          gp_text_node_grid_coord.set_coord(real_rect.get_ll_x(), y);
          gp_text_node_grid_coord.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_node_grid_coord.set_message(RTUTIL.getString("(", grid_x, " , ", grid_y, " , ", 0, ")"));
          gp_text_node_grid_coord.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_node_grid_coord.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_node_grid_coord);

          y -= y_reduced_span;
          GPText gp_text_orient_supply_map;
          gp_text_orient_supply_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_orient_supply_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_orient_supply_map.set_message("orient_supply_map: ");
          gp_text_orient_supply_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_orient_supply_map.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_orient_supply_map);

          if (!pr_node.get_orient_supply_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_orient_supply_map_info;
            gp_text_orient_supply_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_orient_supply_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string orient_supply_map_info_message = "--";
            for (auto& [orient, supply] : pr_node.get_orient_supply_map()) {
              orient_supply_map_info_message += RTUTIL.getString("(", GetOrientationName()(orient), ",", supply, ")");
            }
            gp_text_orient_supply_map_info.set_message(orient_supply_map_info_message);
            gp_text_orient_supply_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_orient_supply_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            pr_node_map_struct.push(gp_text_orient_supply_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_ignore_net_orient_map;
          gp_text_ignore_net_orient_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_ignore_net_orient_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_ignore_net_orient_map.set_message("ignore_net_orient_map: ");
          gp_text_ignore_net_orient_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_ignore_net_orient_map.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_ignore_net_orient_map);

          if (!pr_node.get_ignore_net_orient_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_ignore_net_orient_map_info;
            gp_text_ignore_net_orient_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_ignore_net_orient_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string ignore_net_orient_map_info_message = "--";
            for (auto& [net_idx, orient_set] : pr_node.get_ignore_net_orient_map()) {
              ignore_net_orient_map_info_message += RTUTIL.getString("(", net_idx);
              for (Orientation orient : orient_set) {
                ignore_net_orient_map_info_message += RTUTIL.getString(",", GetOrientationName()(orient));
              }
              ignore_net_orient_map_info_message += RTUTIL.getString(")");
            }
            gp_text_ignore_net_orient_map_info.set_message(ignore_net_orient_map_info_message);
            gp_text_ignore_net_orient_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_ignore_net_orient_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            pr_node_map_struct.push(gp_text_ignore_net_orient_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_orient_net_map;
          gp_text_orient_net_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_orient_net_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_orient_net_map.set_message("orient_net_map: ");
          gp_text_orient_net_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_orient_net_map.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_orient_net_map);

          if (!pr_node.get_orient_net_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_orient_net_map_info;
            gp_text_orient_net_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_orient_net_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string orient_net_map_info_message = "--";
            for (auto& [orient, net_set] : pr_node.get_orient_net_map()) {
              orient_net_map_info_message += RTUTIL.getString("(", GetOrientationName()(orient));
              for (int32_t net_idx : net_set) {
                orient_net_map_info_message += RTUTIL.getString(",", net_idx);
              }
              orient_net_map_info_message += RTUTIL.getString(")");
            }
            gp_text_orient_net_map_info.set_message(orient_net_map_info_message);
            gp_text_orient_net_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_orient_net_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            pr_node_map_struct.push(gp_text_orient_net_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_net_orient_map;
          gp_text_net_orient_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_net_orient_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_net_orient_map.set_message("net_orient_map: ");
          gp_text_net_orient_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_net_orient_map.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_net_orient_map);

          if (!pr_node.get_net_orient_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_net_orient_map_info;
            gp_text_net_orient_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_net_orient_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string net_orient_map_info_message = "--";
            for (auto& [net_idx, orient_set] : pr_node.get_net_orient_map()) {
              net_orient_map_info_message += RTUTIL.getString("(", net_idx);
              for (Orientation orient : orient_set) {
                net_orient_map_info_message += RTUTIL.getString(",", GetOrientationName()(orient));
              }
              net_orient_map_info_message += RTUTIL.getString(")");
            }
            gp_text_net_orient_map_info.set_message(net_orient_map_info_message);
            gp_text_net_orient_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_net_orient_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            pr_node_map_struct.push(gp_text_net_orient_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_overflow;
          gp_text_overflow.set_coord(real_rect.get_ll_x(), y);
          gp_text_overflow.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_overflow.set_message(RTUTIL.getString("overflow: ", pr_node.getOverflow()));
          gp_text_overflow.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_overflow.set_presentation(GPTextPresentation::kLeftMiddle);
          pr_node_map_struct.push(gp_text_overflow);
        }
      }
      gp_gds.addStruct(pr_node_map_struct);
    }
    // overflow
    {
      GPStruct overflow_struct("overflow");
      for (int32_t grid_x = 0; grid_x < pr_node_map.get_x_size(); grid_x++) {
        for (int32_t grid_y = 0; grid_y < pr_node_map.get_y_size(); grid_y++) {
          PRNode& pr_node = pr_node_map[grid_x][grid_y];
          if (pr_node.getOverflow() <= 0) {
            continue;
          }
          PlanarRect real_rect = RTUTIL.getRealRectByGCell(pr_node, gcell_axis);

          GPBoundary gp_boundary;
          gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kOverflow));
          gp_boundary.set_rect(real_rect);
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          overflow_struct.push(gp_boundary);
        }
      }
      gp_gds.addStruct(overflow_struct);
    }
  }

  std::string gds_file_path = RTUTIL.getString(pr_temp_directory_path, flag, "_pr_model.gds");
  RTGP.plot(gp_gds, gds_file_path);
}

void PlanarRouter::debugCheckPRModel(PRModel& pr_model)
{
  GridMap<PRNode>& pr_node_map = pr_model.get_pr_node_map();
  for (int32_t x = 0; x < pr_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < pr_node_map.get_y_size(); y++) {
      PRNode& pr_node = pr_node_map[x][y];
      for (auto& [orient, neighbor] : pr_node.get_neighbor_node_map()) {
        Orientation opposite_orient = RTUTIL.getOppositeOrientation(orient);
        if (!RTUTIL.exist(neighbor->get_neighbor_node_map(), opposite_orient)) {
          RTLOG.error(Loc::current(), "The pr_node neighbor is not bidirectional!");
        }
        if (neighbor->get_neighbor_node_map()[opposite_orient] != &pr_node) {
          RTLOG.error(Loc::current(), "The pr_node neighbor is not bidirectional!");
        }
        if (RTUTIL.getOrientation(PlanarCoord(pr_node), PlanarCoord(*neighbor)) == orient) {
          continue;
        }
        RTLOG.error(Loc::current(), "The neighbor orient is different with real region!");
      }
    }
  }
}

#endif

}  // namespace irt
