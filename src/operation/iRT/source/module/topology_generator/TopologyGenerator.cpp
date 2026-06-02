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
#include "TopologyGenerator.hpp"

#include "GDSPlotter.hpp"
#include "RTInterface.hpp"
#include "TGCandidate.hpp"
#include "Utility.hpp"

namespace irt {

namespace {

struct TGRouteMetrics
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

struct TGLocalRerouteMetrics
{
  double overflow = 0;
  double overflow_cost = 0;
  double high_usage = 0;
  double max_usage_ratio = 0;
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

bool isSameRoutingResult(TGSegmentTask& tg_segment_task, std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  return routing_segment_list.size() == 1 && isSamePlanarSegment(routing_segment_list.front(), tg_segment_task.get_planar_segment());
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

TGRouteMetrics getRouteMetrics(TGModel& tg_model, int32_t net_idx, std::vector<Segment<PlanarCoord>>& segment_list)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  GridMap<double>& congestion_risk_map = tg_model.get_congestion_risk_map();
  double overflow_unit = tg_model.get_tg_iter_param().get_overflow_unit();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();

  TGRouteMetrics metrics;
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
    uint8_t direction_mask = getTGDirectionMask(direction);
    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        if (!tg_node_map.isInside(x, y)) {
          continue;
        }
        TGNodeCost node_cost = tg_node_map[x][y].getFastCost(net_idx, direction_mask, overflow_unit, true);
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

double getCoordSetOverflow(TGModel& tg_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double overflow = 0;
  for (PlanarCoord coord : coord_set) {
    if (tg_node_map.isInside(coord.get_x(), coord.get_y())) {
      overflow += tg_node_map[coord.get_x()][coord.get_y()].getOverflow();
    }
  }
  return overflow;
}

double getCoordSetHighUsage(TGModel& tg_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();
  double high_usage = 0;
  for (PlanarCoord coord : coord_set) {
    if (tg_node_map.isInside(coord.get_x(), coord.get_y())) {
      high_usage += tg_node_map[coord.get_x()][coord.get_y()].getHighUsage(high_usage_ratio_threshold);
    }
  }
  return high_usage;
}

TGLocalRerouteMetrics getCoordSetLocalMetrics(TGModel& tg_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& coord_set)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double overflow_unit = tg_model.get_tg_iter_param().get_overflow_unit();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();

  TGLocalRerouteMetrics metrics;
  for (PlanarCoord coord : coord_set) {
    if (!tg_node_map.isInside(coord.get_x(), coord.get_y())) {
      continue;
    }
    TGNode& tg_node = tg_node_map[coord.get_x()][coord.get_y()];
    TGNodeCost node_cost = tg_node.getFastCost(kTGMaskNone, overflow_unit);
    metrics.overflow += tg_node.getOverflow();
    metrics.overflow_cost += node_cost.overflow_cost;
    metrics.high_usage += tg_node.getHighUsage(high_usage_ratio_threshold);
    metrics.max_usage_ratio = std::max(metrics.max_usage_ratio, tg_node.getMaxUsageRatio());
  }
  return metrics;
}

bool passRerouteShapeGuard(TGRouteMetrics& old_metrics, TGRouteMetrics& new_metrics)
{
  bool overflow_task = old_metrics.overflow > RT_ERROR;
  int32_t max_corner_num = overflow_task ? 8 : 6;
  int32_t max_segment_num = max_corner_num + 1;
  double max_wire_length = old_metrics.wire_length * (overflow_task ? 3.0 : 2.0) + 10;
  return new_metrics.corner_num <= max_corner_num && new_metrics.segment_num <= max_segment_num && new_metrics.wire_length <= max_wire_length;
}

bool acceptReroute(TGRouteMetrics& old_metrics, TGRouteMetrics& new_metrics)
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
    if (RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)
        && new_metrics.high_usage < old_metrics.high_usage
        && !RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)
        && new_metrics.congestion_risk < old_metrics.congestion_risk
        && !RTUTIL.equalDoubleByError(new_metrics.congestion_risk, old_metrics.congestion_risk, RT_ERROR)) {
      return true;
    }
    if (RTUTIL.equalDoubleByError(new_metrics.overflow, old_metrics.overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(new_metrics.congestion_risk, old_metrics.congestion_risk, RT_ERROR)
        && new_metrics.wire_length <= old_metrics.wire_length && new_metrics.corner_num <= old_metrics.corner_num) {
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
    if (RTUTIL.equalDoubleByError(new_metrics.high_usage, old_metrics.high_usage, RT_ERROR)
        && new_metrics.max_usage_ratio < old_metrics.max_usage_ratio
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

bool acceptTrueLocalReroute(TGRouteMetrics& old_route_metrics, TGRouteMetrics& new_route_metrics, TGLocalRerouteMetrics& old_local_metrics,
                            TGLocalRerouteMetrics& new_local_metrics)
{
  if (!passRerouteShapeGuard(old_route_metrics, new_route_metrics)) {
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

  if (new_local_metrics.overflow > old_local_metrics.overflow
      && !RTUTIL.equalDoubleByError(new_local_metrics.overflow, old_local_metrics.overflow, RT_ERROR)) {
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

}  // namespace

// public

void TopologyGenerator::initInst()
{
  if (_tg_instance == nullptr) {
    _tg_instance = new TopologyGenerator();
  }
}

TopologyGenerator& TopologyGenerator::getInst()
{
  if (_tg_instance == nullptr) {
    RTLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tg_instance;
}

void TopologyGenerator::destroyInst()
{
  if (_tg_instance != nullptr) {
    delete _tg_instance;
    _tg_instance = nullptr;
  }
}

// function

void TopologyGenerator::generate()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");
  TGModel tg_model = initTGModel();
  setTGComParam(tg_model);
  initTGTaskList(tg_model);
  buildTGNodeMap(tg_model);
  buildTGNodeNeighbor(tg_model);
  buildOrientSupply(tg_model);
  // debugCheckTGModel(tg_model);
  generateTGModel(tg_model);
  rerouteTGModel(tg_model);
  // debugPlotTGModel(tg_model, "after");
  updateSummary(tg_model);
  printSummary(tg_model);
  outputGuide(tg_model);
  outputNetCSV(tg_model);
  outputOverflowCSV(tg_model);
  outputJson(tg_model);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TopologyGenerator* TopologyGenerator::_tg_instance = nullptr;

TGModel TopologyGenerator::initTGModel()
{
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();

  TGModel tg_model;
  tg_model.set_tg_net_list(convertToTGNetList(net_list));
  return tg_model;
}

std::vector<TGNet> TopologyGenerator::convertToTGNetList(std::vector<Net>& net_list)
{
  std::vector<TGNet> tg_net_list;
  tg_net_list.reserve(net_list.size());
  for (size_t i = 0; i < net_list.size(); i++) {
    tg_net_list.emplace_back(convertToTGNet(net_list[i]));
  }
  return tg_net_list;
}

TGNet TopologyGenerator::convertToTGNet(Net& net)
{
  TGNet tg_net;
  tg_net.set_origin_net(&net);
  tg_net.set_net_idx(net.get_net_idx());
  tg_net.set_connect_type(net.get_connect_type());
  for (Pin& pin : net.get_pin_list()) {
    tg_net.get_tg_pin_list().push_back(TGPin(pin));
  }
  tg_net.set_bounding_box(net.get_bounding_box());
  return tg_net;
}

void TopologyGenerator::setTGComParam(TGModel& tg_model)
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

  TGComParam tg_com_param(topo_spilt_length, expand_step_num, expand_step_length, overflow_unit, corner_weight);
  RTLOG.info(Loc::current(), "topo_spilt_length: ", tg_com_param.get_topo_spilt_length());
  RTLOG.info(Loc::current(), "expand_step_num: ", tg_com_param.get_expand_step_num());
  RTLOG.info(Loc::current(), "expand_step_length: ", tg_com_param.get_expand_step_length());
  RTLOG.info(Loc::current(), "overflow_unit: ", tg_com_param.get_overflow_unit());
  RTLOG.info(Loc::current(), "corner_weight: ", tg_com_param.get_corner_weight());
  tg_model.set_tg_com_param(tg_com_param);
}

void TopologyGenerator::initTGTaskList(TGModel& tg_model)
{
  std::vector<TGNet>& tg_net_list = tg_model.get_tg_net_list();
  std::vector<TGNet*>& tg_task_list = tg_model.get_tg_task_list();
  tg_task_list.reserve(tg_net_list.size());
  for (TGNet& tg_net : tg_net_list) {
    tg_task_list.push_back(&tg_net);
  }
  std::sort(tg_task_list.begin(), tg_task_list.end(), CmpTGNet());
}

void TopologyGenerator::buildTGNodeMap(TGModel& tg_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  tg_node_map.init(gcell_map.get_x_size(), gcell_map.get_y_size());
#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < gcell_map.get_x_size(); x++) {
    for (int32_t y = 0; y < gcell_map.get_y_size(); y++) {
      TGNode& tg_node = tg_node_map[x][y];
      tg_node.set_coord(x, y);
      tg_node.set_boundary_wire_unit(gcell_map[x][y].get_boundary_wire_unit());
      tg_node.set_internal_wire_unit(gcell_map[x][y].get_internal_wire_unit());
      tg_node.set_internal_via_unit(gcell_map[x][y].get_internal_via_unit());
      for (auto& [routing_layer_idx, ignore_net_orient_map] : gcell_map[x][y].get_routing_ignore_net_orient_map()) {
        for (auto& [net_idx, orient_set] : ignore_net_orient_map) {
          tg_node.get_ignore_net_orient_map()[net_idx].insert(orient_set.begin(), orient_set.end());
        }
      }
    }
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::buildTGNodeNeighbor(TGModel& tg_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();

  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < gcell_map.get_x_size(); x++) {
    for (int32_t y = 0; y < gcell_map.get_y_size(); y++) {
      std::map<Orientation, TGNode*>& neighbor_node_map = tg_node_map[x][y].get_neighbor_node_map();
      if (x != 0) {
        neighbor_node_map[Orientation::kWest] = &tg_node_map[x - 1][y];
      }
      if (x != (tg_node_map.get_x_size() - 1)) {
        neighbor_node_map[Orientation::kEast] = &tg_node_map[x + 1][y];
      }
      if (y != 0) {
        neighbor_node_map[Orientation::kSouth] = &tg_node_map[x][y - 1];
      }
      if (y != (tg_node_map.get_y_size() - 1)) {
        neighbor_node_map[Orientation::kNorth] = &tg_node_map[x][y + 1];
      }
    }
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::buildOrientSupply(TGModel& tg_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();

#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < gcell_map.get_x_size(); x++) {
    for (int32_t y = 0; y < gcell_map.get_y_size(); y++) {
      std::map<Orientation, int32_t> planar_orient_supply_map;
      for (auto& [layer_idx, orient_supply_map] : gcell_map[x][y].get_routing_orient_supply_map()) {
        for (auto& [orient, supply] : orient_supply_map) {
          planar_orient_supply_map[orient] += supply;
        }
      }
      tg_node_map[x][y].set_orient_supply_map(planar_orient_supply_map);
    }
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::generateTGModel(TGModel& tg_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<TGNet*>& tg_task_list = tg_model.get_tg_task_list();

  int32_t batch_size = RTUTIL.getBatchSize(tg_task_list.size());

  Monitor stage_monitor;
  for (size_t i = 0; i < tg_task_list.size(); i++) {
    routeTGTask(tg_model, tg_task_list[i]);
    if ((i + 1) % batch_size == 0 || (i + 1) == tg_task_list.size()) {
      RTLOG.info(Loc::current(), "Routed ", (i + 1), "/", tg_task_list.size(), "(", RTUTIL.getPercentage(i + 1, tg_task_list.size()), ") nets",
                 stage_monitor.getStatsInfo());
    }
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::routeTGTask(TGModel& tg_model, TGNet* tg_task)
{
  initSingleTask(tg_model, tg_task);
  std::vector<Segment<PlanarCoord>> routing_segment_list = getRoutingSegmentList(tg_model);
  MTree<PlanarCoord> coord_tree = getCoordTree(tg_model, routing_segment_list);
  updateDemandToGraph(tg_model, ChangeType::kAdd, coord_tree);
  uploadNetResult(tg_model, coord_tree);
  resetSingleTask(tg_model);
}

void TopologyGenerator::initSingleTask(TGModel& tg_model, TGNet* tg_task)
{
  tg_model.set_curr_tg_task(tg_task);
}

std::vector<Segment<PlanarCoord>> TopologyGenerator::getRoutingSegmentList(TGModel& tg_model)
{
  std::vector<Segment<PlanarCoord>> planar_topo_list = getPlanarTopoList(tg_model);
  double corner_weight = tg_model.get_tg_com_param().get_corner_weight();
  TGShadowDemandMap self_shadow = initTGShadowDemandMap(tg_model);
  std::vector<Segment<PlanarCoord>> routing_segment_list;

  for (size_t topo_idx = 0; topo_idx < planar_topo_list.size(); topo_idx++) {
    const TGShadowDemandMap* shadow_ptr = self_shadow.empty() ? nullptr : &self_shadow;
    std::vector<TGCandidate> candidate_list
        = getTGCandidateListByTopo(tg_model, static_cast<int32_t>(topo_idx), planar_topo_list[topo_idx], shadow_ptr);
    if (candidate_list.empty()) {
      continue;
    }

#pragma omp parallel for
    for (TGCandidate& tg_candidate : candidate_list) {
      updateTGCandidate(tg_model, tg_candidate, shadow_ptr);
    }

    TGCandidate* best_candidate = nullptr;
    for (TGCandidate& tg_candidate : candidate_list) {
      if (best_candidate == nullptr || isBetterCandidate(tg_candidate, *best_candidate, corner_weight)) {
        best_candidate = &tg_candidate;
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

std::vector<TGCandidate> TopologyGenerator::getTGCandidateListByTopo(TGModel& tg_model, int32_t topo_idx, Segment<PlanarCoord>& planar_topo,
                                                                     const TGShadowDemandMap* shadow_demand_map)
{
  std::vector<TGCandidate> tg_candidate_list;

  auto appendCandidateList = [&](int32_t corner_num, std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list) {
    for (const std::vector<Segment<PlanarCoord>>& routing_segment_list : routing_segment_list_list) {
      tg_candidate_list.emplace_back(topo_idx, routing_segment_list, corner_num, 0, false, 0);
    }
  };

  bool long_oblique_topo = isLongObliqueTopo(tg_model, planar_topo);
  if (!long_oblique_topo) {
    appendCandidateList(0, getRoutingSegmentListByStraight(tg_model, planar_topo));
  }
  appendCandidateList(1, getRoutingSegmentListByLPattern(tg_model, planar_topo));
  appendCandidateList(2, getRoutingSegmentListByZPattern(tg_model, planar_topo));
  if (long_oblique_topo) {
    appendCandidateList(3, getRoutingSegmentListByLowCostLane3Bends(tg_model, planar_topo, shadow_demand_map));
  } else {
    appendCandidateList(3, getRoutingSegmentListByInner3Bends(tg_model, planar_topo));
  }
  appendCandidateList(4, getRoutingSegmentListByUPattern(tg_model, planar_topo));
  appendCandidateList(5, getRoutingSegmentListByOuter3Bends(tg_model, planar_topo));

  return tg_candidate_list;
}

std::vector<TGCandidate> TopologyGenerator::getTGCandidateList(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& planar_topo_list)
{
  std::vector<TGCandidate> tg_candidate_list;
  for (size_t i = 0; i < planar_topo_list.size(); i++) {
    std::vector<TGCandidate> topo_candidate_list = getTGCandidateListByTopo(tg_model, static_cast<int32_t>(i), planar_topo_list[i]);
    tg_candidate_list.insert(tg_candidate_list.end(), topo_candidate_list.begin(), topo_candidate_list.end());
  }
  return tg_candidate_list;
}

TopologyGenerator::TGShadowDemandMap TopologyGenerator::initTGShadowDemandMap(TGModel& tg_model)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  GridMap<uint8_t>& orient_mask_map = tg_model.get_shadow_orient_mask_map();
  GridMap<int32_t>& stamp_map = tg_model.get_shadow_stamp_map();

  if (orient_mask_map.get_x_size() != tg_node_map.get_x_size() || orient_mask_map.get_y_size() != tg_node_map.get_y_size()
      || stamp_map.get_x_size() != tg_node_map.get_x_size() || stamp_map.get_y_size() != tg_node_map.get_y_size()) {
    orient_mask_map.init(tg_node_map.get_x_size(), tg_node_map.get_y_size(), 0);
    stamp_map.init(tg_node_map.get_x_size(), tg_node_map.get_y_size(), 0);
    tg_model.set_shadow_stamp(0);
  }

  int32_t shadow_stamp = tg_model.get_shadow_stamp() + 1;
  if (shadow_stamp == INT_MAX) {
    stamp_map.init(tg_node_map.get_x_size(), tg_node_map.get_y_size(), 0);
    shadow_stamp = 1;
  }
  tg_model.set_shadow_stamp(shadow_stamp);

  TGShadowDemandMap shadow_demand_map;
  shadow_demand_map.orient_mask_map = &orient_mask_map;
  shadow_demand_map.stamp_map = &stamp_map;
  shadow_demand_map.stamp = shadow_stamp;
  return shadow_demand_map;
}

bool TopologyGenerator::isBetterCandidate(TGCandidate& candidate, TGCandidate& current_best, double corner_weight)
{
  auto computeScore = [corner_weight](TGCandidate& c) {
    return c.get_total_wire_length() + c.get_total_cost() + corner_weight * c.get_total_corner_num();
  };

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

std::vector<Segment<PlanarCoord>> TopologyGenerator::getPlanarTopoList(TGModel& tg_model)
{
  std::vector<PlanarCoord> planar_coord_list;
  {
    for (TGPin& tg_pin : tg_model.get_curr_tg_task()->get_tg_pin_list()) {
      planar_coord_list.push_back(tg_pin.get_access_point().get_grid_coord());
    }
    std::sort(planar_coord_list.begin(), planar_coord_list.end(), CmpPlanarCoordByXASC());
    planar_coord_list.erase(std::unique(planar_coord_list.begin(), planar_coord_list.end()), planar_coord_list.end());
  }
  std::vector<Segment<PlanarCoord>> planar_topo_list;
  for (Segment<PlanarCoord>& planar_topo : RTI.getPlanarTopoList(planar_coord_list)) {
    planar_topo_list.push_back(planar_topo);
  }
  return planar_topo_list;
}

bool TopologyGenerator::isLongObliqueTopo(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
{
  int32_t topo_spilt_length = tg_model.get_tg_com_param().get_topo_spilt_length();
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  int32_t span_x = std::abs(first_coord.get_x() - second_coord.get_x());
  int32_t span_y = std::abs(first_coord.get_y() - second_coord.get_y());
  return (span_x > 1 && span_y > 1 && (span_x > topo_spilt_length || span_y > topo_spilt_length));
}

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByStraight(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
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

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByLPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
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

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByZPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
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

std::vector<int32_t> TopologyGenerator::getMidIndexList(int32_t first_idx, int32_t second_idx)
{
  std::vector<int32_t> mid_index_list;
  RTUTIL.swapByASC(first_idx, second_idx);
  mid_index_list.reserve(second_idx - first_idx - 1);
  for (int32_t i = (first_idx + 1); i <= (second_idx - 1); i++) {
    mid_index_list.push_back(i);
  }
  return mid_index_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByUPattern(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
{
  Die& die = RTDM.getDatabase().get_die();
  int32_t expand_step_num = tg_model.get_tg_com_param().get_expand_step_num();
  int32_t expand_step_length = tg_model.get_tg_com_param().get_expand_step_length();

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

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByInner3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
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

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByLowCostLane3Bends(
    TGModel& tg_model, Segment<PlanarCoord>& planar_topo, const TGShadowDemandMap* shadow_demand_map)
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
    return getPatternSegmentFastScore(tg_model, segment, shadow_demand_map);
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
  for (auto [numerator, denominator] : {std::pair<int32_t, int32_t>(1, 4), std::pair<int32_t, int32_t>(1, 2),
                                        std::pair<int32_t, int32_t>(3, 4)}) {
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
      routing_segment_list_list.push_back({Segment<PlanarCoord>(first_coord, horizontal_mid1),
                                           Segment<PlanarCoord>(horizontal_mid1, horizontal_mid2),
                                           Segment<PlanarCoord>(horizontal_mid2, horizontal_mid3),
                                           Segment<PlanarCoord>(horizontal_mid3, second_coord)});

      PlanarCoord vertical_mid1(first_coord.get_x(), y);
      PlanarCoord vertical_mid2(x, y);
      PlanarCoord vertical_mid3(x, second_coord.get_y());
      routing_segment_list_list.push_back({Segment<PlanarCoord>(first_coord, vertical_mid1),
                                           Segment<PlanarCoord>(vertical_mid1, vertical_mid2),
                                           Segment<PlanarCoord>(vertical_mid2, vertical_mid3),
                                           Segment<PlanarCoord>(vertical_mid3, second_coord)});
    }
  }
  return routing_segment_list_list;
}

double TopologyGenerator::getPatternSegmentFastScore(TGModel& tg_model, Segment<PlanarCoord>& segment, const TGShadowDemandMap* shadow_demand_map)
{
  PlanarCoord& first_coord = segment.get_first();
  PlanarCoord& second_coord = segment.get_second();
  if (!RTUTIL.isRightAngled(first_coord, second_coord)) {
    RTLOG.error(Loc::current(), "The direction is error!");
  }
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double overflow_unit = tg_model.get_tg_com_param().get_overflow_unit();
  double wire_unit = 1.0;

  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  double score = RTUTIL.getManhattanDistance(first_coord, second_coord) * wire_unit;
  uint8_t direction_mask = getTGDirectionMask(RTUTIL.getDirection(first_coord, second_coord));
  for (int32_t x = first_x; x <= second_x; x++) {
    for (int32_t y = first_y; y <= second_y; y++) {
      uint8_t shadow_mask = shadow_demand_map ? shadow_demand_map->getMask(x, y) : 0;
      TGNodeCost node_cost = tg_node_map[x][y].getFastCost(direction_mask | shadow_mask, overflow_unit);
      score += node_cost.getTotalCost();
    }
  }
  return score;
}

std::vector<std::vector<Segment<PlanarCoord>>> TopologyGenerator::getRoutingSegmentListByOuter3Bends(TGModel& tg_model, Segment<PlanarCoord>& planar_topo)
{
  Die& die = RTDM.getDatabase().get_die();
  int32_t expand_step_num = tg_model.get_tg_com_param().get_expand_step_num();
  int32_t expand_step_length = tg_model.get_tg_com_param().get_expand_step_length();

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

void TopologyGenerator::updateTGCandidate(TGModel& tg_model, TGCandidate& tg_candidate,
                                         const TGShadowDemandMap* shadow_demand_map)
{
  double overflow_unit = tg_model.get_tg_com_param().get_overflow_unit();
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();

  TGCandidateCost candidate_cost;
  Direction pre_direction = Direction::kNone;
  for (Segment<PlanarCoord>& coord_segment : tg_candidate.get_routing_segment_list()) {
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
    uint8_t direction_mask = getTGDirectionMask(direction);
    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        uint8_t shadow_mask = shadow_demand_map ? shadow_demand_map->getMask(x, y) : 0;
        TGNodeCost node_cost = tg_node_map[x][y].getFastCost(direction_mask | shadow_mask, overflow_unit);
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

  tg_candidate.set_candidate_cost(candidate_cost);
}

MTree<PlanarCoord> TopologyGenerator::getCoordTree(TGModel& tg_model, std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  std::vector<PlanarCoord> candidate_root_coord_list;
  std::map<PlanarCoord, std::set<int32_t>, CmpPlanarCoordByXASC> key_coord_pin_map;
  std::vector<TGPin>& tg_pin_list = tg_model.get_curr_tg_task()->get_tg_pin_list();
  for (size_t i = 0; i < tg_pin_list.size(); i++) {
    PlanarCoord coord = tg_pin_list[i].get_access_point().get_grid_coord();
    candidate_root_coord_list.push_back(coord);
    key_coord_pin_map[coord].insert(static_cast<int32_t>(i));
  }
  return RTUTIL.getTreeByFullFlow(candidate_root_coord_list, routing_segment_list, key_coord_pin_map);
}

void TopologyGenerator::uploadNetResult(TGModel& tg_model, MTree<PlanarCoord>& coord_tree)
{
  for (Segment<TNode<PlanarCoord>*>& coord_segment : RTUTIL.getSegListByTree(coord_tree)) {
    Segment<LayerCoord>* segment = new Segment<LayerCoord>({coord_segment.get_first()->value(), 0}, {coord_segment.get_second()->value(), 0});
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, tg_model.get_curr_tg_task()->get_net_idx(), segment);
  }
}

void TopologyGenerator::resetSingleTask(TGModel& tg_model)
{
  tg_model.set_curr_tg_task(nullptr);
}

void TopologyGenerator::rerouteTGModel(TGModel& tg_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  setTGIterParam(tg_model);
  rebuildDemandToGraph(tg_model);
  initNetGlobalResultMap(tg_model);
  initTGMetric(tg_model);

  TGIterParam& tg_iter_param = tg_model.get_tg_iter_param();
  for (int32_t iter = 1; iter <= tg_iter_param.get_max_iter_num(); iter++) {
    Monitor iter_monitor;
    RTLOG.info(Loc::current(), "***** Begin iteration ", iter, "/", tg_iter_param.get_max_iter_num(), "(",
               RTUTIL.getPercentage(iter, tg_iter_param.get_max_iter_num()), ") *****");
    updateCongestionRisk(tg_model);
    if (tg_model.get_metric_valid()) {
      tg_model.set_curr_congestion_risk(getCongestionRisk(tg_model));
    }
    updateBestResult(tg_model);

    std::vector<TGSegmentTask> tg_segment_task_list = initTGSegmentTaskList(tg_model);
    if (tg_segment_task_list.empty()) {
      RTLOG.info(Loc::current(), "No TG segment task found!");
      break;
    }

    size_t routed_task_num = 0;
    size_t success_task_num = 0;
    for (TGSegmentTask& tg_segment_task : tg_segment_task_list) {
      if (tg_segment_task.get_routed_times() >= tg_iter_param.get_max_routed_times()) {
        continue;
      }
      bool enable_true_local_accept = (iter == tg_iter_param.get_max_iter_num());
      if (routeTGSegmentTask(tg_model, tg_segment_task, enable_true_local_accept)) {
        success_task_num++;
      }
      tg_segment_task.addRoutedTimes();
      routed_task_num++;
    }
    RTLOG.info(Loc::current(), "Routed ", routed_task_num, " segment tasks, success ", success_task_num, ", overflow ",
               tg_model.get_curr_overflow(), ", high_usage ", tg_model.get_curr_high_usage(), iter_monitor.getStatsInfo());
  }

  updateCongestionRisk(tg_model);
  if (tg_model.get_metric_valid()) {
    tg_model.set_curr_congestion_risk(getCongestionRisk(tg_model));
  }
  updateBestResult(tg_model);
  uploadBestResult(tg_model);
  rebuildDemandToGraph(tg_model);

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::setTGIterParam(TGModel& tg_model)
{
  int32_t max_iter_num = 3;
  int32_t max_routed_times = 1;
  int32_t max_task_num = 8000;
  double wire_unit = 1;
  double corner_unit = 5 * wire_unit;
  double overflow_unit = tg_model.get_tg_com_param().get_overflow_unit();
  double congestion_risk_unit = overflow_unit;
  double high_usage_unit = overflow_unit;
  double high_usage_ratio_threshold = 0.90;
  int32_t congestion_risk_radius = 3;
  int32_t route_window_base_expand = 10;
  int32_t route_window_max_expand_times = 3;
  double route_window_expand_ratio = 2;
  bool enable_full_die_fallback = false;

  TGIterParam tg_iter_param(max_iter_num, max_routed_times, max_task_num, wire_unit, corner_unit, overflow_unit, congestion_risk_unit,
                            high_usage_unit, high_usage_ratio_threshold, congestion_risk_radius, route_window_base_expand,
                            route_window_max_expand_times, route_window_expand_ratio, enable_full_die_fallback);
  RTLOG.info(Loc::current(), "max_iter_num: ", tg_iter_param.get_max_iter_num());
  RTLOG.info(Loc::current(), "max_routed_times: ", tg_iter_param.get_max_routed_times());
  RTLOG.info(Loc::current(), "max_task_num: ", tg_iter_param.get_max_task_num());
  RTLOG.info(Loc::current(), "wire_unit: ", tg_iter_param.get_wire_unit());
  RTLOG.info(Loc::current(), "corner_unit: ", tg_iter_param.get_corner_unit());
  RTLOG.info(Loc::current(), "overflow_unit: ", tg_iter_param.get_overflow_unit());
  RTLOG.info(Loc::current(), "congestion_risk_unit: ", tg_iter_param.get_congestion_risk_unit());
  RTLOG.info(Loc::current(), "high_usage_unit: ", tg_iter_param.get_high_usage_unit());
  RTLOG.info(Loc::current(), "high_usage_ratio_threshold: ", tg_iter_param.get_high_usage_ratio_threshold());
  RTLOG.info(Loc::current(), "congestion_risk_radius: ", tg_iter_param.get_congestion_risk_radius());
  RTLOG.info(Loc::current(), "route_window_base_expand: ", tg_iter_param.get_route_window_base_expand());
  RTLOG.info(Loc::current(), "route_window_max_expand_times: ", tg_iter_param.get_route_window_max_expand_times());
  RTLOG.info(Loc::current(), "route_window_expand_ratio: ", tg_iter_param.get_route_window_expand_ratio());
  RTLOG.info(Loc::current(), "enable_full_die_fallback: ", tg_iter_param.get_enable_full_die_fallback());
  tg_model.set_tg_iter_param(tg_iter_param);
}

void TopologyGenerator::rebuildDemandToGraph(TGModel& tg_model)
{
  Die& die = RTDM.getDatabase().get_die();
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();

#pragma omp parallel for collapse(2)
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      tg_node_map[x][y].clearDemand();
    }
  }

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      std::vector<Segment<PlanarCoord>> segment_list;
      segment_list.emplace_back(segment->get_first().get_planar_coord(), segment->get_second().get_planar_coord());
      updateDemandToGraph(tg_model, ChangeType::kAdd, net_idx, segment_list);
    }
  }
}

void TopologyGenerator::initNetGlobalResultMap(TGModel& tg_model)
{
  Die& die = RTDM.getDatabase().get_die();
  tg_model.set_net_global_result_map(RTDM.getNetGlobalResultMap(die));
}

double TopologyGenerator::getOverflow(TGModel& tg_model)
{
  double total_overflow = 0;
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      total_overflow += tg_node_map[x][y].getOverflow();
    }
  }
  return total_overflow;
}

double TopologyGenerator::getCongestionRisk(TGModel& tg_model)
{
  double total_congestion_risk = 0;
  for (auto& [net_idx, segment_set] : tg_model.get_net_global_result_map()) {
    for (Segment<LayerCoord>* segment : segment_set) {
      total_congestion_risk += getSegmentCongestionRisk(tg_model, segment);
    }
  }
  return total_congestion_risk;
}

double TopologyGenerator::getHighUsage(TGModel& tg_model)
{
  double total_high_usage = 0;
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      total_high_usage += tg_node_map[x][y].getHighUsage(high_usage_ratio_threshold);
    }
  }
  return total_high_usage;
}

double TopologyGenerator::getWireLength(TGModel& tg_model)
{
  double total_wire_length = 0;
  for (auto& [net_idx, segment_set] : tg_model.get_net_global_result_map()) {
    for (Segment<LayerCoord>* segment : segment_set) {
      total_wire_length += RTUTIL.getManhattanDistance(segment->get_first(), segment->get_second());
    }
  }
  return total_wire_length;
}

void TopologyGenerator::initTGMetric(TGModel& tg_model)
{
  tg_model.set_curr_overflow(getOverflow(tg_model));
  tg_model.set_curr_high_usage(getHighUsage(tg_model));
  tg_model.set_curr_congestion_risk(getCongestionRisk(tg_model));
  tg_model.set_curr_wire_length(getWireLength(tg_model));
  tg_model.set_metric_valid(true);
  tg_model.get_changed_net_set().clear();
}

void TopologyGenerator::updateCongestionRisk(TGModel& tg_model)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  GridMap<double>& congestion_risk_map = tg_model.get_congestion_risk_map();

  int32_t risk_radius = std::max(0, tg_model.get_tg_iter_param().get_congestion_risk_radius());
  double history_risk_decay = 0.5;
  GridMap<double> history_congestion_risk_map = congestion_risk_map;
  congestion_risk_map.init(tg_node_map.get_x_size(), tg_node_map.get_y_size(), 0.0);
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      double overflow = tg_node_map[x][y].getOverflow();
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
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      tg_node_map[x][y].set_congestion_risk(congestion_risk_map[x][y]);
    }
  }
}

void TopologyGenerator::collectTGHotspotInfo(TGModel& tg_model, std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set,
                                             std::set<int32_t>& overflow_net_set,
                                             std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set,
                                             std::set<int32_t>& high_usage_net_set)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();

  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      TGNode& tg_node = tg_node_map[x][y];
      if (tg_node.getOverflow() > 0) {
        overflow_coord_set.insert(PlanarCoord(x, y));
        std::set<int32_t> node_overflow_net_set = tg_node.getOverflowNetSet();
        overflow_net_set.insert(node_overflow_net_set.begin(), node_overflow_net_set.end());
      }
      if (tg_node.getMaxUsageRatio() > high_usage_ratio_threshold + RT_ERROR) {
        high_usage_coord_set.insert(PlanarCoord(x, y));
        std::set<int32_t> node_high_usage_net_set = tg_node.getHighUsageNetSet(high_usage_ratio_threshold);
        high_usage_net_set.insert(node_high_usage_net_set.begin(), node_high_usage_net_set.end());
      }
    }
  }
}

std::vector<TGSegmentTask> TopologyGenerator::initTGSegmentTaskList(TGModel& tg_model)
{
  std::vector<TGNet>& tg_net_list = tg_model.get_tg_net_list();
  int32_t max_task_num = tg_model.get_tg_iter_param().get_max_task_num();

  std::set<PlanarCoord, CmpPlanarCoordByXASC> overflow_coord_set;
  std::set<int32_t> overflow_net_set;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> high_usage_coord_set;
  std::set<int32_t> high_usage_net_set;
  collectTGHotspotInfo(tg_model, overflow_coord_set, overflow_net_set, high_usage_coord_set, high_usage_net_set);

  std::set<int32_t> candidate_net_set;
  candidate_net_set.insert(overflow_net_set.begin(), overflow_net_set.end());
  candidate_net_set.insert(high_usage_net_set.begin(), high_usage_net_set.end());
  std::set<int32_t> changed_candidate_net_set = tg_model.get_changed_net_set();
  candidate_net_set.insert(changed_candidate_net_set.begin(), changed_candidate_net_set.end());
  tg_model.get_changed_net_set().clear();

  std::set<Segment<LayerCoord>*> visited_segment_set;
  std::vector<TGSegmentTask> tg_segment_task_list;
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map = tg_model.get_net_global_result_map();
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
      double segment_overflow = getSegmentOverflow(tg_model, segment);
      double segment_congestion_risk = getSegmentCongestionRisk(tg_model, segment);
      double segment_high_usage = getSegmentHighUsage(tg_model, segment);
      double segment_max_usage_ratio = getSegmentMaxUsageRatio(tg_model, segment);
      bool changed_net = RTUTIL.exist(changed_candidate_net_set, net_idx);
      bool cross_overflow = (RTUTIL.exist(overflow_net_set, net_idx) || changed_net) && isSegmentCrossOverflow(tg_model, segment, overflow_coord_set);
      bool cross_high_usage = (RTUTIL.exist(high_usage_net_set, net_idx) || changed_net) && isSegmentCrossHighUsage(tg_model, segment, high_usage_coord_set);
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
            origin_overflow_penalty_map[coord] = tg_model.get_tg_node_map()[x][y].getOverflow();
          }
        }
      }
      TGSegmentTask tg_segment_task;
      tg_segment_task.set_net_idx(net_idx);
      tg_segment_task.set_connect_type(tg_net_list[net_idx].get_connect_type());
      tg_segment_task.set_origin_segment(segment);
      tg_segment_task.set_planar_segment(Segment<PlanarCoord>(segment->get_first().get_planar_coord(), segment->get_second().get_planar_coord()));
      tg_segment_task.set_wire_length(RTUTIL.getManhattanDistance(segment->get_first(), segment->get_second()));
      tg_segment_task.set_overflow(segment_overflow);
      tg_segment_task.set_congestion_risk(segment_congestion_risk);
      tg_segment_task.set_high_usage(segment_high_usage);
      tg_segment_task.set_max_usage_ratio(segment_max_usage_ratio);
      tg_segment_task.set_origin_overflow_penalty_map(origin_overflow_penalty_map);
      tg_segment_task_list.push_back(tg_segment_task);
    }
  }

  std::sort(tg_segment_task_list.begin(), tg_segment_task_list.end(), [](TGSegmentTask& a, TGSegmentTask& b) {
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
  if (0 < max_task_num && max_task_num < static_cast<int32_t>(tg_segment_task_list.size())) {
    tg_segment_task_list.resize(max_task_num);
  }
  RTLOG.info(Loc::current(), "Generated ", tg_segment_task_list.size(), " TG segment tasks");
  return tg_segment_task_list;
}

bool TopologyGenerator::routeTGSegmentTask(TGModel& tg_model, TGSegmentTask& tg_segment_task, bool enable_true_local_accept)
{
  Segment<LayerCoord>* curr_segment = getCurrentGlobalSegment(tg_segment_task.get_net_idx(), tg_segment_task.get_planar_segment());
  if (curr_segment == nullptr) {
    return false;
  }
  double segment_overflow = getSegmentOverflow(tg_model, curr_segment);
  double segment_high_usage = getSegmentHighUsage(tg_model, curr_segment);
  if (segment_overflow <= RT_ERROR && segment_high_usage <= RT_ERROR) {
    return false;
  }

  int32_t net_idx = tg_segment_task.get_net_idx();
  std::vector<Segment<PlanarCoord>> origin_segment_list = {tg_segment_task.get_planar_segment()};
  TGRouteMetrics old_metrics = getRouteMetrics(tg_model, net_idx, origin_segment_list);
  TGRouteMetrics accepted_metrics;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> accepted_affected_coord_set;
  TGLocalRerouteMetrics accepted_old_local_metrics;
  TGLocalRerouteMetrics accepted_new_local_metrics;
  std::vector<Segment<PlanarCoord>> routing_segment_list;

  updateDemandToGraph(tg_model, ChangeType::kDel, net_idx, origin_segment_list);
  for (PlanarRect& route_window : getRouteWindowList(tg_model, tg_segment_task)) {
    routing_segment_list.clear();
    if (!searchSegmentByAStar(tg_model, tg_segment_task, route_window, routing_segment_list) || routing_segment_list.empty()) {
      continue;
    }
    routing_segment_list = simplifyRoutingSegmentList(routing_segment_list);
    if (routing_segment_list.empty() || isSameRoutingResult(tg_segment_task, routing_segment_list)) {
      continue;
    }
    TGRouteMetrics new_metrics = getRouteMetrics(tg_model, net_idx, routing_segment_list);
    bool accepted = false;
    std::set<PlanarCoord, CmpPlanarCoordByXASC> affected_coord_set;
    TGLocalRerouteMetrics old_local_metrics;
    TGLocalRerouteMetrics new_local_metrics;
    if (enable_true_local_accept) {
      collectSegmentCoordSet(origin_segment_list, affected_coord_set);
      collectSegmentCoordSet(routing_segment_list, affected_coord_set);

      updateDemandToGraph(tg_model, ChangeType::kAdd, net_idx, origin_segment_list);
      old_local_metrics = getCoordSetLocalMetrics(tg_model, affected_coord_set);
      updateDemandToGraph(tg_model, ChangeType::kDel, net_idx, origin_segment_list);

      updateDemandToGraph(tg_model, ChangeType::kAdd, net_idx, routing_segment_list);
      new_local_metrics = getCoordSetLocalMetrics(tg_model, affected_coord_set);
      updateDemandToGraph(tg_model, ChangeType::kDel, net_idx, routing_segment_list);

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
    updateDemandToGraph(tg_model, ChangeType::kAdd, net_idx, origin_segment_list);
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
    updateDemandToGraph(tg_model, ChangeType::kAdd, net_idx, origin_segment_list);
    old_affected_overflow = getCoordSetOverflow(tg_model, affected_coord_set);
    old_affected_high_usage = getCoordSetHighUsage(tg_model, affected_coord_set);
    updateDemandToGraph(tg_model, ChangeType::kDel, net_idx, origin_segment_list);
  }

  updateDemandToGraph(tg_model, ChangeType::kAdd, net_idx, routing_segment_list);

  double new_affected_overflow = enable_true_local_accept ? accepted_new_local_metrics.overflow : getCoordSetOverflow(tg_model, affected_coord_set);
  double new_affected_high_usage = enable_true_local_accept ? accepted_new_local_metrics.high_usage : getCoordSetHighUsage(tg_model, affected_coord_set);
  if (tg_model.get_metric_valid()) {
    tg_model.set_curr_overflow(tg_model.get_curr_overflow() + new_affected_overflow - old_affected_overflow);
    tg_model.set_curr_high_usage(tg_model.get_curr_high_usage() + new_affected_high_usage - old_affected_high_usage);
    tg_model.set_curr_congestion_risk(tg_model.get_curr_congestion_risk() + accepted_metrics.congestion_risk - old_metrics.congestion_risk);
    tg_model.set_curr_wire_length(tg_model.get_curr_wire_length() + accepted_metrics.wire_length - old_metrics.wire_length);
  }
  tg_model.get_changed_net_set().insert(net_idx);

  tg_model.get_net_global_result_map()[net_idx].erase(curr_segment);
  RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, curr_segment);
  if (tg_model.get_net_global_result_map()[net_idx].empty()) {
    tg_model.get_net_global_result_map().erase(net_idx);
  }
  for (Segment<PlanarCoord>& routing_segment : routing_segment_list) {
    Segment<LayerCoord>* new_segment = new Segment<LayerCoord>(LayerCoord(routing_segment.get_first(), 0), LayerCoord(routing_segment.get_second(), 0));
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, net_idx, new_segment);
    tg_model.get_net_global_result_map()[net_idx].insert(new_segment);
  }
  tg_segment_task.set_origin_segment(nullptr);
  return true;
}

std::vector<PlanarRect> TopologyGenerator::getRouteWindowList(TGModel& tg_model, TGSegmentTask& tg_segment_task)
{
  TGIterParam& tg_iter_param = tg_model.get_tg_iter_param();
  std::vector<PlanarRect> route_window_list;

  int32_t expand_size = std::max(0, tg_iter_param.get_route_window_base_expand());
  int32_t max_expand_times = std::max(1, tg_iter_param.get_route_window_max_expand_times());
  double expand_ratio = std::max(1.0, tg_iter_param.get_route_window_expand_ratio());
  for (int32_t i = 0; i < max_expand_times; i++) {
    PlanarRect route_window = getRouteWindow(tg_model, tg_segment_task, expand_size);
    if (route_window_list.empty() || route_window_list.back() != route_window) {
      route_window_list.push_back(route_window);
    }
    expand_size = static_cast<int32_t>(std::ceil(expand_size * expand_ratio));
  }
  if (tg_iter_param.get_enable_full_die_fallback()) {
    PlanarRect die_window = getDieWindow(tg_model);
    if (route_window_list.empty() || route_window_list.back() != die_window) {
      route_window_list.push_back(die_window);
    }
  }
  return route_window_list;
}

PlanarRect TopologyGenerator::getRouteWindow(TGModel& tg_model, TGSegmentTask& tg_segment_task, int32_t expand_size)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  PlanarCoord& first_coord = tg_segment_task.get_planar_segment().get_first();
  PlanarCoord& second_coord = tg_segment_task.get_planar_segment().get_second();

  int32_t ll_x = std::min(first_coord.get_x(), second_coord.get_x()) - expand_size;
  int32_t ll_y = std::min(first_coord.get_y(), second_coord.get_y()) - expand_size;
  int32_t ur_x = std::max(first_coord.get_x(), second_coord.get_x()) + expand_size;
  int32_t ur_y = std::max(first_coord.get_y(), second_coord.get_y()) + expand_size;

  ll_x = std::max(0, ll_x);
  ll_y = std::max(0, ll_y);
  ur_x = std::min(tg_node_map.get_x_size() - 1, ur_x);
  ur_y = std::min(tg_node_map.get_y_size() - 1, ur_y);

  return PlanarRect(ll_x, ll_y, ur_x, ur_y);
}

PlanarRect TopologyGenerator::getDieWindow(TGModel& tg_model)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  return PlanarRect(0, 0, tg_node_map.get_x_size() - 1, tg_node_map.get_y_size() - 1);
}

bool TopologyGenerator::searchSegmentByAStar(TGModel& tg_model, TGSegmentTask& tg_segment_task, PlanarRect& route_window,
                                             std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  PlanarCoord start_coord = tg_segment_task.get_planar_segment().get_first();
  PlanarCoord end_coord = tg_segment_task.get_planar_segment().get_second();
  if (!tg_node_map.isInside(start_coord.get_x(), start_coord.get_y()) || !tg_node_map.isInside(end_coord.get_x(), end_coord.get_y())) {
    return false;
  }
  if (!RTUTIL.isInside(route_window, start_coord) || !RTUTIL.isInside(route_window, end_coord)) {
    return false;
  }
  if (start_coord == end_coord) {
    return false;
  }

  TGNode* start_node = &tg_node_map[start_coord.get_x()][start_coord.get_y()];
  TGNode* end_node = &tg_node_map[end_coord.get_x()][end_coord.get_y()];
  std::vector<TGNode*> visited_node_list;
  OpenQueue<TGNode> open_queue;
  TGAStarNodeCostCache node_cost_cache;
  node_cost_cache.route_window = route_window;
  int32_t window_x_size = route_window.get_ur_x() - route_window.get_ll_x() + 1;
  int32_t window_y_size = route_window.get_ur_y() - route_window.get_ll_y() + 1;
  node_cost_cache.cost_map.init(window_x_size, window_y_size, std::array<double, 2>{0.0, 0.0});
  node_cost_cache.valid_map.init(window_x_size, window_y_size, std::array<bool, 2>{false, false});

  initPathHead(tg_model, start_node, end_node, visited_node_list, open_queue);
  TGNode* path_head_node = popFromOpenList(open_queue);
  while (!searchEnded(path_head_node, end_node)) {
    expandSearching(tg_model, tg_segment_task, route_window, path_head_node, end_node, visited_node_list, open_queue, node_cost_cache);
    path_head_node = popFromOpenList(open_queue);
  }
  if (path_head_node == end_node) {
    routing_segment_list = getRoutingSegmentListByNode(path_head_node);
  }
  resetPathState(visited_node_list, open_queue);
  return !routing_segment_list.empty();
}

void TopologyGenerator::initPathHead(TGModel& tg_model, TGNode* start_node, TGNode* end_node, std::vector<TGNode*>& visited_node_list,
                                     OpenQueue<TGNode>& open_queue)
{
  start_node->set_known_cost(0);
  start_node->set_estimated_cost(getEstimateCost(tg_model, start_node, end_node));
  open_queue.push(start_node);
  start_node->set_state(TGNodeState::kOpen);
  visited_node_list.push_back(start_node);
}

bool TopologyGenerator::searchEnded(TGNode* path_head_node, TGNode* end_node)
{
  if (path_head_node == nullptr) {
    return true;
  }
  return path_head_node == end_node;
}

void TopologyGenerator::expandSearching(TGModel& tg_model, TGSegmentTask& tg_segment_task, PlanarRect& route_window, TGNode* path_head_node,
                                        TGNode* end_node, std::vector<TGNode*>& visited_node_list, OpenQueue<TGNode>& open_queue,
                                        TGAStarNodeCostCache& node_cost_cache)
{
  for (auto& [orientation, neighbor_node] : path_head_node->get_neighbor_node_map()) {
    if (neighbor_node == nullptr || neighbor_node->isClose()) {
      continue;
    }
    if (!RTUTIL.isInside(route_window, *neighbor_node)) {
      continue;
    }
    double known_cost = getKnownCost(tg_model, tg_segment_task, path_head_node, neighbor_node, node_cost_cache);
    if (neighbor_node->isOpen() && known_cost < neighbor_node->get_known_cost()) {
      neighbor_node->set_known_cost(known_cost);
      neighbor_node->set_parent_node(path_head_node);
      open_queue.push(neighbor_node);
    } else if (neighbor_node->isNone()) {
      neighbor_node->set_known_cost(known_cost);
      neighbor_node->set_parent_node(path_head_node);
      neighbor_node->set_estimated_cost(getEstimateCost(tg_model, neighbor_node, end_node));
      open_queue.push(neighbor_node);
      neighbor_node->set_state(TGNodeState::kOpen);
      visited_node_list.push_back(neighbor_node);
    }
  }
}

TGNode* TopologyGenerator::popFromOpenList(OpenQueue<TGNode>& open_queue)
{
  TGNode* node = open_queue.pop();
  if (node != nullptr) {
    node->set_state(TGNodeState::kClose);
  }
  return node;
}

void TopologyGenerator::resetPathState(std::vector<TGNode*>& visited_node_list, OpenQueue<TGNode>& open_queue)
{
  open_queue.clear();
  for (TGNode* visited_node : visited_node_list) {
    visited_node->set_state(TGNodeState::kNone);
    visited_node->set_parent_node(nullptr);
    visited_node->set_known_cost(0);
    visited_node->set_estimated_cost(0);
  }
  visited_node_list.clear();
}

std::vector<Segment<PlanarCoord>> TopologyGenerator::getRoutingSegmentListByNode(TGNode* node)
{
  std::vector<Segment<PlanarCoord>> routing_segment_list;

  TGNode* curr_node = node;
  TGNode* pre_node = curr_node->get_parent_node();
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

double TopologyGenerator::getKnownCost(TGModel& tg_model, TGSegmentTask& tg_segment_task, TGNode* start_node, TGNode* end_node,
                                       TGAStarNodeCostCache& node_cost_cache)
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
  cost += getNodeCost(tg_model, tg_segment_task, start_node, direction, node_cost_cache);
  cost += getNodeCost(tg_model, tg_segment_task, end_node, direction, node_cost_cache);
  cost += tg_model.get_tg_iter_param().get_wire_unit();
  if (start_node->get_parent_node() != nullptr) {
    Direction pre_direction = RTUTIL.getDirection(*start_node->get_parent_node(), *start_node);
    if (pre_direction != direction) {
      cost += tg_model.get_tg_iter_param().get_corner_unit();
    }
  }
  return cost;
}

double TopologyGenerator::getNodeCost(TGModel& tg_model, TGSegmentTask& tg_segment_task, TGNode* curr_node, Direction direction,
                                      TGAStarNodeCostCache& node_cost_cache)
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

  double overflow_unit = tg_model.get_tg_iter_param().get_overflow_unit();
  double congestion_risk_unit = tg_model.get_tg_iter_param().get_congestion_risk_unit();
  double high_usage_unit = tg_model.get_tg_iter_param().get_high_usage_unit();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();
  double node_cost = 0;
  TGNodeCost tg_node_cost = curr_node->getFastCost(tg_segment_task.get_net_idx(), getTGDirectionMask(direction), overflow_unit, false);
  node_cost += tg_node_cost.getTotalCost();
  if (tg_node_cost.max_usage_ratio >= high_usage_ratio_threshold) {
    double high_usage_ratio
        = (tg_node_cost.max_usage_ratio - high_usage_ratio_threshold) / std::max(RT_ERROR, 1.0 - high_usage_ratio_threshold);
    node_cost += high_usage_unit * std::pow(high_usage_ratio, 2);
  }
  if (tg_segment_task.get_overflow() > RT_ERROR) {
    auto iter = tg_segment_task.get_origin_overflow_penalty_map().find(PlanarCoord(x, y));
    if (iter != tg_segment_task.get_origin_overflow_penalty_map().end()) {
      constexpr double kEscapePenaltyScale = 0.5;
      node_cost += overflow_unit * kEscapePenaltyScale * std::pow(iter->second + 1, 4);
    }
  }
  node_cost += congestion_risk_unit * curr_node->get_congestion_risk();
  node_cost_cache.cost_map[cache_x][cache_y][direction_idx] = node_cost;
  node_cost_cache.valid_map[cache_x][cache_y][direction_idx] = true;
  return node_cost;
}

double TopologyGenerator::getEstimateCost(TGModel& tg_model, TGNode* start_node, TGNode* end_node)
{
  return RTUTIL.getManhattanDistance(*start_node, *end_node) * tg_model.get_tg_iter_param().get_wire_unit();
}

bool TopologyGenerator::isSegmentCrossOverflow(TGModel& tg_model, Segment<LayerCoord>* segment,
                                               std::set<PlanarCoord, CmpPlanarCoordByXASC>& overflow_coord_set)
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

bool TopologyGenerator::isSegmentCrossHighUsage(TGModel& tg_model, Segment<LayerCoord>* segment,
                                                std::set<PlanarCoord, CmpPlanarCoordByXASC>& high_usage_coord_set)
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

double TopologyGenerator::getSegmentOverflow(TGModel& tg_model, Segment<LayerCoord>* segment)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
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
      if (tg_node_map.isInside(x, y)) {
        segment_overflow += tg_node_map[x][y].getOverflow();
      }
    }
  }
  return segment_overflow;
}

double TopologyGenerator::getSegmentCongestionRisk(TGModel& tg_model, Segment<LayerCoord>* segment)
{
  GridMap<double>& congestion_risk_map = tg_model.get_congestion_risk_map();
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

double TopologyGenerator::getSegmentHighUsage(TGModel& tg_model, Segment<LayerCoord>* segment)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  double high_usage_ratio_threshold = tg_model.get_tg_iter_param().get_high_usage_ratio_threshold();
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
      if (tg_node_map.isInside(x, y)) {
        segment_high_usage += tg_node_map[x][y].getHighUsage(high_usage_ratio_threshold);
      }
    }
  }
  return segment_high_usage;
}

double TopologyGenerator::getSegmentMaxUsageRatio(TGModel& tg_model, Segment<LayerCoord>* segment)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
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
      if (tg_node_map.isInside(x, y)) {
        segment_max_usage_ratio = std::max(segment_max_usage_ratio, tg_node_map[x][y].getMaxUsageRatio());
      }
    }
  }
  return segment_max_usage_ratio;
}

void TopologyGenerator::updateBestResult(TGModel& tg_model)
{
  std::map<int32_t, std::vector<Segment<LayerCoord>>>& best_net_global_result_map = tg_model.get_best_net_global_result_map();

  double curr_overflow = tg_model.get_metric_valid() ? tg_model.get_curr_overflow() : getOverflow(tg_model);
  double curr_high_usage = tg_model.get_metric_valid() ? tg_model.get_curr_high_usage() : getHighUsage(tg_model);
  double curr_congestion_risk = tg_model.get_metric_valid() ? tg_model.get_curr_congestion_risk() : getCongestionRisk(tg_model);
  double curr_wire_length = tg_model.get_metric_valid() ? tg_model.get_curr_wire_length() : getWireLength(tg_model);
  if (!best_net_global_result_map.empty()) {
    if (tg_model.get_best_overflow() < curr_overflow) {
      return;
    }
    if (RTUTIL.equalDoubleByError(tg_model.get_best_overflow(), curr_overflow, RT_ERROR)
        && tg_model.get_best_high_usage() < curr_high_usage) {
      return;
    }
    if (RTUTIL.equalDoubleByError(tg_model.get_best_overflow(), curr_overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(tg_model.get_best_high_usage(), curr_high_usage, RT_ERROR)
        && tg_model.get_best_congestion_risk() < curr_congestion_risk) {
      return;
    }
    if (RTUTIL.equalDoubleByError(tg_model.get_best_overflow(), curr_overflow, RT_ERROR)
        && RTUTIL.equalDoubleByError(tg_model.get_best_high_usage(), curr_high_usage, RT_ERROR)
        && RTUTIL.equalDoubleByError(tg_model.get_best_congestion_risk(), curr_congestion_risk, RT_ERROR)
        && tg_model.get_best_wire_length() < curr_wire_length) {
      return;
    }
  }

  best_net_global_result_map.clear();
  for (auto& [net_idx, segment_set] : tg_model.get_net_global_result_map()) {
    for (Segment<LayerCoord>* segment : segment_set) {
      best_net_global_result_map[net_idx].push_back(*segment);
    }
  }
  tg_model.set_best_overflow(curr_overflow);
  tg_model.set_best_high_usage(curr_high_usage);
  tg_model.set_best_congestion_risk(curr_congestion_risk);
  tg_model.set_best_wire_length(curr_wire_length);
}

void TopologyGenerator::uploadBestResult(TGModel& tg_model)
{
  Die& die = RTDM.getDatabase().get_die();
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, segment);
    }
  }
  for (auto& [net_idx, segment_list] : tg_model.get_best_net_global_result_map()) {
    for (Segment<LayerCoord>& segment : segment_list) {
      RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, net_idx, new Segment<LayerCoord>(segment));
    }
  }
  initNetGlobalResultMap(tg_model);
}

#if 1  // update env

void TopologyGenerator::updateDemandToGraph(TGModel& tg_model, ChangeType change_type, MTree<PlanarCoord>& coord_tree)
{
  int32_t curr_net_idx = tg_model.get_curr_tg_task()->get_net_idx();

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
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  for (auto& [usage_coord, orientation_list] : usage_map) {
    TGNode& tg_node = tg_node_map[usage_coord.get_x()][usage_coord.get_y()];
    tg_node.updateDemand(curr_net_idx, orientation_list, change_type);
  }
}

void TopologyGenerator::updateDemandToGraph(TGModel& tg_model, ChangeType change_type, int32_t net_idx,
                                            std::vector<Segment<PlanarCoord>>& segment_list)
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
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  for (auto& [usage_coord, orientation_list] : usage_map) {
    TGNode& tg_node = tg_node_map[usage_coord.get_x()][usage_coord.get_y()];
    tg_node.updateDemand(net_idx, orientation_list, change_type);
  }
}

void TopologyGenerator::addCandidateToShadow(TGShadowDemandMap& shadow_map, TGCandidate& tg_candidate)
{
  for (Segment<PlanarCoord>& coord_segment : tg_candidate.get_routing_segment_list()) {
    PlanarCoord& first_coord = coord_segment.get_first();
    PlanarCoord& second_coord = coord_segment.get_second();

    Orientation orientation = RTUTIL.getOrientation(first_coord, second_coord);
    if (orientation == Orientation::kNone || orientation == Orientation::kOblique) {
      RTLOG.error(Loc::current(), "The orientation is error!");
    }
    Orientation opposite_orientation = RTUTIL.getOppositeOrientation(orientation);
    uint8_t orientation_mask = getTGOrientMask(orientation);
    uint8_t opposite_orientation_mask = getTGOrientMask(opposite_orientation);

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

void TopologyGenerator::updateSummary(TGModel& tg_model)
{
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  Summary& summary = RTDM.getDatabase().get_summary();
  int32_t enable_timing = RTDM.getConfig().enable_timing;

  double& total_demand = summary.tg_summary.total_demand;
  double& total_overflow = summary.tg_summary.total_overflow;
  double& total_wire_length = summary.tg_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.tg_summary.clock_timing_map;
  std::map<std::string, double>& type_power_map = summary.tg_summary.type_power_map;

  std::vector<TGNet>& tg_net_list = tg_model.get_tg_net_list();
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();

  total_demand = 0;
  total_overflow = 0;
  total_wire_length = 0;
  clock_timing_map.clear();
  type_power_map.clear();

  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      double node_demand = tg_node_map[x][y].getDemand();
      double node_overflow = tg_node_map[x][y].getOverflow();
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
    real_pin_coord_map_list.resize(tg_net_list.size());
    std::vector<std::vector<Segment<LayerCoord>>> routing_segment_list_list;
    routing_segment_list_list.resize(tg_net_list.size());
    for (TGNet& tg_net : tg_net_list) {
      for (TGPin& tg_pin : tg_net.get_tg_pin_list()) {
        LayerCoord layer_coord = tg_pin.get_access_point().getGridLayerCoord();
        real_pin_coord_map_list[tg_net.get_net_idx()][tg_pin.get_pin_name()].emplace_back(RTUTIL.getRealRectByGCell(layer_coord, gcell_axis).getMidPoint(), 0);
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
    RTI.updateTimingAndPower(real_pin_coord_map_list, routing_segment_list_list, clock_timing_map, type_power_map);
  }
}

void TopologyGenerator::printSummary(TGModel& tg_model)
{
  Summary& summary = RTDM.getDatabase().get_summary();
  int32_t enable_timing = RTDM.getConfig().enable_timing;

  double& total_demand = summary.tg_summary.total_demand;
  double& total_overflow = summary.tg_summary.total_overflow;
  double& total_wire_length = summary.tg_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.tg_summary.clock_timing_map;
  std::map<std::string, double>& type_power_map = summary.tg_summary.type_power_map;

  fort::char_table summary_table;
  {
    summary_table.set_cell_text_align(fort::text_align::right);
    summary_table << fort::header << "total_demand" << total_demand << fort::endr;
    summary_table << fort::header << "total_overflow" << total_overflow << fort::endr;
    summary_table << fort::header << "total_wire_length" << total_wire_length << fort::endr;
  }
  fort::char_table timing_table;
  timing_table.set_cell_text_align(fort::text_align::right);
  fort::char_table power_table;
  power_table.set_cell_text_align(fort::text_align::right);
  if (enable_timing) {
    timing_table << fort::header << "clock_name"
                 << "tns"
                 << "wns"
                 << "freq" << fort::endr;
    for (auto& [clock_name, timing_map] : clock_timing_map) {
      timing_table << clock_name << timing_map["TNS"] << timing_map["WNS"] << timing_map["Freq(MHz)"] << fort::endr;
    }
    power_table << fort::header << "power_type";
    for (auto& [type, power] : type_power_map) {
      power_table << fort::header << type;
    }
    power_table << fort::endr;
    power_table << "power_value";
    for (auto& [type, power] : type_power_map) {
      power_table << power;
    }
    power_table << fort::endr;
  }
  RTUTIL.printTableList({summary_table});
  RTUTIL.printTableList({timing_table, power_table});
}

void TopologyGenerator::outputGuide(TGModel& tg_model)
{
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<TGNet>& tg_net_list = tg_model.get_tg_net_list();

  std::ofstream* guide_file_stream = RTUTIL.getOutputFileStream(RTUTIL.getString(tg_temp_directory_path, "route.guide"));
  if (guide_file_stream == nullptr) {
    return;
  }
  RTUTIL.pushStream(guide_file_stream, "guide net_name\n");
  RTUTIL.pushStream(guide_file_stream, "pin grid_x grid_y real_x real_y layer energy name\n");
  RTUTIL.pushStream(guide_file_stream, "wire grid1_x grid1_y grid2_x grid2_y real1_x real1_y real2_x real2_y layer\n");
  RTUTIL.pushStream(guide_file_stream, "via grid_x grid_y real_x real_y layer1 layer2\n");

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    TGNet& tg_net = tg_net_list[net_idx];
    RTUTIL.pushStream(guide_file_stream, "guide ", tg_net.get_origin_net()->get_net_name(), "\n");

    for (TGPin& tg_pin : tg_net.get_tg_pin_list()) {
      AccessPoint& access_point = tg_pin.get_access_point();
      double grid_x = access_point.get_grid_x();
      double grid_y = access_point.get_grid_y();
      double real_x = access_point.get_real_x() / 1.0 / micron_dbu;
      double real_y = access_point.get_real_y() / 1.0 / micron_dbu;
      std::string layer = routing_layer_list[access_point.get_layer_idx()].get_layer_name();
      std::string connnect;
      if (tg_pin.get_is_driven()) {
        connnect = "driven";
      } else {
        connnect = "load";
      }
      RTUTIL.pushStream(guide_file_stream, "pin ", grid_x, " ", grid_y, " ", real_x, " ", real_y, " ", layer, " ", connnect, " ", tg_pin.get_pin_name(), "\n");
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

void TopologyGenerator::outputNetCSV(TGModel& tg_model)
{
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::ofstream* net_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(tg_temp_directory_path, "net_map.csv"));
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  for (int32_t y = tg_node_map.get_y_size() - 1; y >= 0; y--) {
    for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
      RTUTIL.pushStream(net_csv_file, tg_node_map[x][y].getDemand(), ",");
    }
    RTUTIL.pushStream(net_csv_file, "\n");
  }
  RTUTIL.closeFileStream(net_csv_file);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::outputOverflowCSV(TGModel& tg_model)
{
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::ofstream* overflow_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(tg_temp_directory_path, "overflow_map.csv"));
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  for (int32_t y = tg_node_map.get_y_size() - 1; y >= 0; y--) {
    for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
      RTUTIL.pushStream(overflow_csv_file, tg_node_map[x][y].getOverflow(), ",");
    }
    RTUTIL.pushStream(overflow_csv_file, "\n");
  }
  RTUTIL.closeFileStream(overflow_csv_file);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TopologyGenerator::outputJson(TGModel& tg_model)
{
  int32_t enable_notification = RTDM.getConfig().enable_notification;
  if (!enable_notification) {
    return;
  }
  std::map<std::string, std::string> json_path_map;
  json_path_map["net_map"] = outputNetJson(tg_model);
  json_path_map["overflow_map"] = outputOverflowJson(tg_model);
  json_path_map["summary"] = outputSummaryJson(tg_model);
  RTI.sendNotification("TG", 1, json_path_map);
}

std::string TopologyGenerator::outputNetJson(TGModel& tg_model)
{
  Die& die = RTDM.getDatabase().get_die();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;

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
  std::string net_json_file_path = RTUTIL.getString(tg_temp_directory_path, "net_map.json");
  std::ofstream* net_json_file = RTUTIL.getOutputFileStream(net_json_file_path);
  (*net_json_file) << net_json_list;
  RTUTIL.closeFileStream(net_json_file);
  return net_json_file_path;
}

std::string TopologyGenerator::outputOverflowJson(TGModel& tg_model)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;

  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  std::vector<nlohmann::json> overflow_json_list;
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      PlanarRect gcell = RTUTIL.getRealRectByGCell(PlanarCoord(x, y), gcell_axis);
      overflow_json_list.push_back(
          {gcell.get_ll_x(), gcell.get_ll_y(), gcell.get_ur_x(), gcell.get_ur_y(), routing_layer_list[0].get_layer_name(), tg_node_map[x][y].getOverflow()});
    }
  }
  std::string overflow_json_file_path = RTUTIL.getString(tg_temp_directory_path, "overflow_map.json");
  std::ofstream* overflow_json_file = RTUTIL.getOutputFileStream(overflow_json_file_path);
  (*overflow_json_file) << overflow_json_list;
  RTUTIL.closeFileStream(overflow_json_file);
  return overflow_json_file_path;
}

std::string TopologyGenerator::outputSummaryJson(TGModel& tg_model)
{
  Summary& summary = RTDM.getDatabase().get_summary();
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;

  double& total_demand = summary.tg_summary.total_demand;
  double& total_overflow = summary.tg_summary.total_overflow;
  double& total_wire_length = summary.tg_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.tg_summary.clock_timing_map;
  std::map<std::string, double>& type_power_map = summary.tg_summary.type_power_map;

  nlohmann::json summary_json;
  summary_json["total_demand"] = total_demand;
  summary_json["total_overflow"] = total_overflow;
  summary_json["total_wire_length"] = total_wire_length;
  for (auto& [clock_name, timing] : clock_timing_map) {
    summary_json["clock_timing_map"]["clock_name"] = clock_name;
    summary_json["clock_timing_map"]["timing"] = timing;
  }
  for (auto& [type, power] : type_power_map) {
    summary_json["type_power_map"]["type"] = type;
    summary_json["type_power_map"]["power"] = power;
  }
  std::string summary_json_file_path = RTUTIL.getString(tg_temp_directory_path, "summary.json");
  std::ofstream* summary_json_file = RTUTIL.getOutputFileStream(summary_json_file_path);
  (*summary_json_file) << summary_json;
  RTUTIL.closeFileStream(summary_json_file);
  return summary_json_file_path;
}

#endif

#if 1  // debug

void TopologyGenerator::debugPlotTGModel(TGModel& tg_model, std::string flag)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& tg_temp_directory_path = RTDM.getConfig().tg_temp_directory_path;

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
    GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
    // tg_node_map
    {
      GPStruct tg_node_map_struct("tg_node_map");
      for (int32_t grid_x = 0; grid_x < tg_node_map.get_x_size(); grid_x++) {
        for (int32_t grid_y = 0; grid_y < tg_node_map.get_y_size(); grid_y++) {
          TGNode& tg_node = tg_node_map[grid_x][grid_y];
          PlanarRect real_rect = RTUTIL.getRealRectByGCell(tg_node, gcell_axis);
          int32_t y_reduced_span = std::max(1, real_rect.getYSpan() / 12);
          int32_t y = real_rect.get_ur_y();

          y -= y_reduced_span;
          GPText gp_text_node_real_coord;
          gp_text_node_real_coord.set_coord(real_rect.get_ll_x(), y);
          gp_text_node_real_coord.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_node_real_coord.set_message(RTUTIL.getString("(", tg_node.get_x(), " , ", tg_node.get_y(), " , ", 0, ")"));
          gp_text_node_real_coord.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_node_real_coord.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_node_real_coord);

          y -= y_reduced_span;
          GPText gp_text_node_grid_coord;
          gp_text_node_grid_coord.set_coord(real_rect.get_ll_x(), y);
          gp_text_node_grid_coord.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_node_grid_coord.set_message(RTUTIL.getString("(", grid_x, " , ", grid_y, " , ", 0, ")"));
          gp_text_node_grid_coord.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_node_grid_coord.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_node_grid_coord);

          y -= y_reduced_span;
          GPText gp_text_orient_supply_map;
          gp_text_orient_supply_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_orient_supply_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_orient_supply_map.set_message("orient_supply_map: ");
          gp_text_orient_supply_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_orient_supply_map.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_orient_supply_map);

          if (!tg_node.get_orient_supply_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_orient_supply_map_info;
            gp_text_orient_supply_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_orient_supply_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string orient_supply_map_info_message = "--";
            for (auto& [orient, supply] : tg_node.get_orient_supply_map()) {
              orient_supply_map_info_message += RTUTIL.getString("(", GetOrientationName()(orient), ",", supply, ")");
            }
            gp_text_orient_supply_map_info.set_message(orient_supply_map_info_message);
            gp_text_orient_supply_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_orient_supply_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            tg_node_map_struct.push(gp_text_orient_supply_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_ignore_net_orient_map;
          gp_text_ignore_net_orient_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_ignore_net_orient_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_ignore_net_orient_map.set_message("ignore_net_orient_map: ");
          gp_text_ignore_net_orient_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_ignore_net_orient_map.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_ignore_net_orient_map);

          if (!tg_node.get_ignore_net_orient_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_ignore_net_orient_map_info;
            gp_text_ignore_net_orient_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_ignore_net_orient_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string ignore_net_orient_map_info_message = "--";
            for (auto& [net_idx, orient_set] : tg_node.get_ignore_net_orient_map()) {
              ignore_net_orient_map_info_message += RTUTIL.getString("(", net_idx);
              for (Orientation orient : orient_set) {
                ignore_net_orient_map_info_message += RTUTIL.getString(",", GetOrientationName()(orient));
              }
              ignore_net_orient_map_info_message += RTUTIL.getString(")");
            }
            gp_text_ignore_net_orient_map_info.set_message(ignore_net_orient_map_info_message);
            gp_text_ignore_net_orient_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_ignore_net_orient_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            tg_node_map_struct.push(gp_text_ignore_net_orient_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_orient_net_map;
          gp_text_orient_net_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_orient_net_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_orient_net_map.set_message("orient_net_map: ");
          gp_text_orient_net_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_orient_net_map.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_orient_net_map);

          if (!tg_node.get_orient_net_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_orient_net_map_info;
            gp_text_orient_net_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_orient_net_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string orient_net_map_info_message = "--";
            for (auto& [orient, net_set] : tg_node.get_orient_net_map()) {
              orient_net_map_info_message += RTUTIL.getString("(", GetOrientationName()(orient));
              for (int32_t net_idx : net_set) {
                orient_net_map_info_message += RTUTIL.getString(",", net_idx);
              }
              orient_net_map_info_message += RTUTIL.getString(")");
            }
            gp_text_orient_net_map_info.set_message(orient_net_map_info_message);
            gp_text_orient_net_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_orient_net_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            tg_node_map_struct.push(gp_text_orient_net_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_net_orient_map;
          gp_text_net_orient_map.set_coord(real_rect.get_ll_x(), y);
          gp_text_net_orient_map.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_net_orient_map.set_message("net_orient_map: ");
          gp_text_net_orient_map.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_net_orient_map.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_net_orient_map);

          if (!tg_node.get_net_orient_map().empty()) {
            y -= y_reduced_span;
            GPText gp_text_net_orient_map_info;
            gp_text_net_orient_map_info.set_coord(real_rect.get_ll_x(), y);
            gp_text_net_orient_map_info.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
            std::string net_orient_map_info_message = "--";
            for (auto& [net_idx, orient_set] : tg_node.get_net_orient_map()) {
              net_orient_map_info_message += RTUTIL.getString("(", net_idx);
              for (Orientation orient : orient_set) {
                net_orient_map_info_message += RTUTIL.getString(",", GetOrientationName()(orient));
              }
              net_orient_map_info_message += RTUTIL.getString(")");
            }
            gp_text_net_orient_map_info.set_message(net_orient_map_info_message);
            gp_text_net_orient_map_info.set_layer_idx(RTGP.getGDSIdxByRouting(0));
            gp_text_net_orient_map_info.set_presentation(GPTextPresentation::kLeftMiddle);
            tg_node_map_struct.push(gp_text_net_orient_map_info);
          }

          y -= y_reduced_span;
          GPText gp_text_overflow;
          gp_text_overflow.set_coord(real_rect.get_ll_x(), y);
          gp_text_overflow.set_text_type(static_cast<int32_t>(GPDataType::kInfo));
          gp_text_overflow.set_message(RTUTIL.getString("overflow: ", tg_node.getOverflow()));
          gp_text_overflow.set_layer_idx(RTGP.getGDSIdxByRouting(0));
          gp_text_overflow.set_presentation(GPTextPresentation::kLeftMiddle);
          tg_node_map_struct.push(gp_text_overflow);
        }
      }
      gp_gds.addStruct(tg_node_map_struct);
    }
    // overflow
    {
      GPStruct overflow_struct("overflow");
      for (int32_t grid_x = 0; grid_x < tg_node_map.get_x_size(); grid_x++) {
        for (int32_t grid_y = 0; grid_y < tg_node_map.get_y_size(); grid_y++) {
          TGNode& tg_node = tg_node_map[grid_x][grid_y];
          if (tg_node.getOverflow() <= 0) {
            continue;
          }
          PlanarRect real_rect = RTUTIL.getRealRectByGCell(tg_node, gcell_axis);

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

  std::string gds_file_path = RTUTIL.getString(tg_temp_directory_path, flag, "_tg_model.gds");
  RTGP.plot(gp_gds, gds_file_path);
}

void TopologyGenerator::debugCheckTGModel(TGModel& tg_model)
{
  GridMap<TGNode>& tg_node_map = tg_model.get_tg_node_map();
  for (int32_t x = 0; x < tg_node_map.get_x_size(); x++) {
    for (int32_t y = 0; y < tg_node_map.get_y_size(); y++) {
      TGNode& tg_node = tg_node_map[x][y];
      for (auto& [orient, neighbor] : tg_node.get_neighbor_node_map()) {
        Orientation opposite_orient = RTUTIL.getOppositeOrientation(orient);
        if (!RTUTIL.exist(neighbor->get_neighbor_node_map(), opposite_orient)) {
          RTLOG.error(Loc::current(), "The tg_node neighbor is not bidirectional!");
        }
        if (neighbor->get_neighbor_node_map()[opposite_orient] != &tg_node) {
          RTLOG.error(Loc::current(), "The tg_node neighbor is not bidirectional!");
        }
        if (RTUTIL.getOrientation(PlanarCoord(tg_node), PlanarCoord(*neighbor)) == orient) {
          continue;
        }
        RTLOG.error(Loc::current(), "The neighbor orient is different with real region!");
      }
    }
  }
}

#endif

}  // namespace irt
