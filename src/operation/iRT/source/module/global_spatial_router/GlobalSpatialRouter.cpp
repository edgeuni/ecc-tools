// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "GlobalSpatialRouter.hpp"

#include <chrono>
#include <limits>
#include <queue>

#include "Monitor.hpp"
#include "RTInterface.hpp"
#include "RoutingLayer.hpp"

namespace irt {

namespace {

using LayerSetByPlanarCoord = std::map<PlanarCoord, std::set<int32_t>, CmpPlanarCoordByXASC>;
using SegmentKey = std::tuple<int32_t, int32_t, int32_t, int32_t, int32_t, int32_t>;
using PlanarKey = std::tuple<int32_t, int32_t>;
using PlanarEdgeKey = std::tuple<int32_t, int32_t, int32_t, int32_t>;
using UndirectedPlanarEdgeKey = std::tuple<int32_t, int32_t, int32_t, int32_t>;

constexpr double kCapacityPressureWeight = 20.0;
constexpr double kCapacityBlockPenalty = 1.0e9;
constexpr double kCapacityOverflowPenalty = 1000.0;
constexpr double kViaSidePenalty = 200.0;

struct GSRStage1RouteOrderKey
{
  bool is_clock = false;
  int32_t bbox_total = 0;
  int32_t x_span = 0;
  int32_t y_span = 0;
  double aspect_ratio = 1.0;
  int32_t pin_num = 0;
  int32_t layer_span = 0;
  double avg_access_layer = 0;
  int64_t hpwl = 0;
  int32_t net_idx = -1;
};

struct GSRSparseMazeNode
{
  PlanarCoord coord;
  Direction direction = Direction::kNone;
  double known_cost = std::numeric_limits<double>::max();
  double estimated_cost = 0;
  int32_t parent_idx = -1;

  double get_known_cost() const { return known_cost; }
  double get_estimated_cost() const { return estimated_cost; }
};

struct GSRMazeHeapItem
{
  int32_t node_idx = -1;
  double known_cost = 0;
  double total_cost = 0;
};

struct CmpGSRMazeHeapItem
{
  bool operator()(const GSRMazeHeapItem& a, const GSRMazeHeapItem& b) const
  {
    if (!RTUTIL.equalDoubleByError(a.total_cost, b.total_cost, RT_ERROR)) {
      return a.total_cost > b.total_cost;
    }
    return a.known_cost > b.known_cost;
  }
};

struct GSRPlanarProjection
{
  std::vector<Segment<PlanarCoord>> split_segment_list;
  std::map<PlanarEdgeKey, std::set<SegmentKey>> edge_segment_key_set_map;
  std::set<PlanarKey> node_key_set;
};

std::string joinNetSet(const std::set<int32_t>& net_set)
{
  std::string net_list = "[";
  bool first = true;
  for (int32_t net_idx : net_set) {
    if (!first) {
      net_list += ";";
    }
    net_list += std::to_string(net_idx);
    first = false;
  }
  net_list += "]";
  return net_list;
}

GSRStage1RouteOrderKey buildStage1RouteOrderKey(const GSRNet& gsr_net)
{
  GSRStage1RouteOrderKey order_key;
  order_key.net_idx = gsr_net.get_net_idx();
  order_key.pin_num = static_cast<int32_t>(gsr_net.get_gsr_pin_list().size());

  const Net* origin_net = gsr_net.get_origin_net();
  order_key.is_clock = origin_net != nullptr && origin_net->get_connect_type() == ConnectType::kClock;

  if (order_key.pin_num == 0) {
    return order_key;
  }

  int32_t ll_x = std::numeric_limits<int32_t>::max();
  int32_t ll_y = std::numeric_limits<int32_t>::max();
  int32_t ur_x = std::numeric_limits<int32_t>::min();
  int32_t ur_y = std::numeric_limits<int32_t>::min();
  int32_t min_layer_idx = std::numeric_limits<int32_t>::max();
  int32_t max_layer_idx = std::numeric_limits<int32_t>::min();
  int64_t layer_sum = 0;

  for (const GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    const LayerCoord& access_coord = gsr_pin.get_access_coord();
    ll_x = std::min(ll_x, access_coord.get_x());
    ll_y = std::min(ll_y, access_coord.get_y());
    ur_x = std::max(ur_x, access_coord.get_x());
    ur_y = std::max(ur_y, access_coord.get_y());
    min_layer_idx = std::min(min_layer_idx, access_coord.get_layer_idx());
    max_layer_idx = std::max(max_layer_idx, access_coord.get_layer_idx());
    layer_sum += access_coord.get_layer_idx();
  }

  order_key.x_span = ur_x - ll_x;
  order_key.y_span = ur_y - ll_y;
  order_key.bbox_total = order_key.x_span + order_key.y_span;
  order_key.hpwl = order_key.bbox_total;

  int32_t safe_x_size = std::max(1, order_key.x_span + 1);
  int32_t safe_y_size = std::max(1, order_key.y_span + 1);
  order_key.aspect_ratio = std::max(safe_x_size, safe_y_size) / 1.0 / std::min(safe_x_size, safe_y_size);
  order_key.layer_span = max_layer_idx - min_layer_idx;
  order_key.avg_access_layer = layer_sum / 1.0 / order_key.pin_num;
  return order_key;
}

LayerCoord makeLayerCoord(const PlanarCoord& coord, const int32_t layer_idx)
{
  return LayerCoord(coord.get_x(), coord.get_y(), layer_idx);
}

SegmentKey makeSegmentKey(const Segment<LayerCoord>& segment)
{
  LayerCoord first_coord = segment.get_first();
  LayerCoord second_coord = segment.get_second();
  std::tuple<int32_t, int32_t, int32_t> first_key(first_coord.get_layer_idx(), first_coord.get_x(), first_coord.get_y());
  std::tuple<int32_t, int32_t, int32_t> second_key(second_coord.get_layer_idx(), second_coord.get_x(), second_coord.get_y());
  if (second_key < first_key) {
    std::swap(first_coord, second_coord);
  }
  return SegmentKey(first_coord.get_layer_idx(), first_coord.get_x(), first_coord.get_y(), second_coord.get_layer_idx(), second_coord.get_x(),
                    second_coord.get_y());
}

std::set<SegmentKey> makeSegmentKeySet(const std::vector<Segment<LayerCoord>>& segment_list)
{
  std::set<SegmentKey> segment_key_set;
  for (const Segment<LayerCoord>& segment : segment_list) {
    segment_key_set.insert(makeSegmentKey(segment));
  }
  return segment_key_set;
}

PlanarKey makePlanarKey(const PlanarCoord& coord)
{
  return PlanarKey(coord.get_x(), coord.get_y());
}

PlanarEdgeKey makePlanarEdgeKey(const PlanarCoord& first_coord, const PlanarCoord& second_coord)
{
  PlanarKey first_key = makePlanarKey(first_coord);
  PlanarKey second_key = makePlanarKey(second_coord);
  if (second_key < first_key) {
    std::swap(first_key, second_key);
  }
  return PlanarEdgeKey(std::get<0>(first_key), std::get<1>(first_key), std::get<0>(second_key), std::get<1>(second_key));
}

UndirectedPlanarEdgeKey makeUndirectedPlanarEdgeKey(const PlanarCoord& first_coord, const PlanarCoord& second_coord)
{
  PlanarKey first_key = makePlanarKey(first_coord);
  PlanarKey second_key = makePlanarKey(second_coord);
  if (second_key < first_key) {
    return UndirectedPlanarEdgeKey(second_coord.get_x(), second_coord.get_y(), first_coord.get_x(), first_coord.get_y());
  }
  return UndirectedPlanarEdgeKey(first_coord.get_x(), first_coord.get_y(), second_coord.get_x(), second_coord.get_y());
}

void addViaChain(std::vector<Segment<LayerCoord>>& segment_list, const PlanarCoord& coord, const int32_t first_layer_idx, const int32_t second_layer_idx)
{
  if (first_layer_idx == second_layer_idx) {
    return;
  }
  int32_t lower_layer_idx = std::min(first_layer_idx, second_layer_idx);
  int32_t upper_layer_idx = std::max(first_layer_idx, second_layer_idx);
  for (int32_t layer_idx = lower_layer_idx; layer_idx < upper_layer_idx; layer_idx++) {
    segment_list.emplace_back(makeLayerCoord(coord, layer_idx), makeLayerCoord(coord, layer_idx + 1));
  }
}

Direction getPlanarSegmentDirection(const Segment<LayerCoord>& segment)
{
  if (segment.get_first().get_layer_idx() != segment.get_second().get_layer_idx()) {
    return Direction::kProximal;
  }
  return RTUTIL.getDirection(segment.get_first().get_planar_coord(), segment.get_second().get_planar_coord());
}

int32_t getSegmentPlanarLength(const Segment<LayerCoord>& segment)
{
  return RTUTIL.getManhattanDistance(segment.get_first().get_planar_coord(), segment.get_second().get_planar_coord());
}

int64_t getRoutePlanarLength(const std::vector<Segment<LayerCoord>>& segment_list)
{
  int64_t length = 0;
  for (const Segment<LayerCoord>& segment : segment_list) {
    length += getSegmentPlanarLength(segment);
  }
  return length;
}

int32_t getRouteViaNum(const std::vector<Segment<LayerCoord>>& segment_list)
{
  int32_t via_num = 0;
  for (const Segment<LayerCoord>& segment : segment_list) {
    if (segment.get_first().get_layer_idx() != segment.get_second().get_layer_idx()) {
      via_num++;
    }
  }
  return via_num;
}

bool passRerouteShapeGuard(const std::vector<Segment<LayerCoord>>& old_segment_list,
                           const std::vector<Segment<LayerCoord>>& new_segment_list,
                           const double old_total_overflow, const double new_total_overflow,
                           const double old_route_cost = 0, const double new_route_cost = 0)
{
  if (old_segment_list.empty()) {
    return true;
  }
  double overflow_improve = old_total_overflow - new_total_overflow;
  if (overflow_improve > 1.0 + RT_ERROR) {
    return true;
  }

  int32_t old_segment_num = static_cast<int32_t>(old_segment_list.size());
  int32_t new_segment_num = static_cast<int32_t>(new_segment_list.size());
  int64_t old_wire_length = getRoutePlanarLength(old_segment_list);
  int64_t new_wire_length = getRoutePlanarLength(new_segment_list);
  int32_t old_via_num = getRouteViaNum(old_segment_list);
  int32_t new_via_num = getRouteViaNum(new_segment_list);

  double segment_ratio = overflow_improve > RT_ERROR ? 3.0 : 2.0;
  double wire_ratio = overflow_improve > RT_ERROR ? 2.0 : 1.5;
  int32_t via_allowance = overflow_improve > RT_ERROR ? 6 : 4;

  if (new_segment_num > static_cast<int32_t>(old_segment_num * segment_ratio) + 4) {
    return false;
  }
  if (new_wire_length > static_cast<int64_t>(old_wire_length * wire_ratio) + 10) {
    return false;
  }
  if (new_via_num > old_via_num + via_allowance) {
    return false;
  }
  if (overflow_improve <= RT_ERROR && new_route_cost > old_route_cost + RT_ERROR) {
    return false;
  }
  return true;
}

GSRPlanarProjection buildPlanarProjection(const std::vector<LayerCoord>& key_coord_list,
                                          const std::vector<Segment<LayerCoord>>& segment_list)
{
  struct PlanarWire
  {
    PlanarCoord first_coord;
    PlanarCoord second_coord;
    SegmentKey segment_key;
  };

  GSRPlanarProjection projection;
  std::map<PlanarKey, std::set<int32_t>> x_cut_set_map;
  std::map<PlanarKey, std::set<int32_t>> y_cut_set_map;
  std::vector<PlanarWire> planar_wire_list;

  auto addPlanarCoord = [&](const PlanarCoord& coord) {
    projection.node_key_set.insert(makePlanarKey(coord));
    x_cut_set_map[PlanarKey(coord.get_y(), 0)].insert(coord.get_x());
    y_cut_set_map[PlanarKey(coord.get_x(), 0)].insert(coord.get_y());
  };

  for (const LayerCoord& key_coord : key_coord_list) {
    addPlanarCoord(key_coord.get_planar_coord());
  }
  for (const Segment<LayerCoord>& segment : segment_list) {
    PlanarCoord first_planar_coord = segment.get_first().get_planar_coord();
    PlanarCoord second_planar_coord = segment.get_second().get_planar_coord();
    addPlanarCoord(first_planar_coord);
    addPlanarCoord(second_planar_coord);
    if (segment.get_first().get_layer_idx() == segment.get_second().get_layer_idx() && first_planar_coord != second_planar_coord) {
      PlanarWire planar_wire;
      planar_wire.first_coord = first_planar_coord;
      planar_wire.second_coord = second_planar_coord;
      planar_wire.segment_key = makeSegmentKey(segment);
      planar_wire_list.push_back(planar_wire);
    }
  }

  for (size_t i = 0; i < planar_wire_list.size(); i++) {
    PlanarCoord first_i = planar_wire_list[i].first_coord;
    PlanarCoord second_i = planar_wire_list[i].second_coord;
    bool i_horizontal = first_i.get_y() == second_i.get_y();
    bool i_vertical = first_i.get_x() == second_i.get_x();
    if (!i_horizontal && !i_vertical) {
      continue;
    }
    for (size_t j = i + 1; j < planar_wire_list.size(); j++) {
      PlanarCoord first_j = planar_wire_list[j].first_coord;
      PlanarCoord second_j = planar_wire_list[j].second_coord;
      bool j_horizontal = first_j.get_y() == second_j.get_y();
      bool j_vertical = first_j.get_x() == second_j.get_x();
      if (i_horizontal && j_horizontal && first_i.get_y() == first_j.get_y()) {
        int32_t lower_x = std::max(std::min(first_i.get_x(), second_i.get_x()), std::min(first_j.get_x(), second_j.get_x()));
        int32_t upper_x = std::min(std::max(first_i.get_x(), second_i.get_x()), std::max(first_j.get_x(), second_j.get_x()));
        if (lower_x < upper_x) {
          addPlanarCoord(PlanarCoord(lower_x, first_i.get_y()));
          addPlanarCoord(PlanarCoord(upper_x, first_i.get_y()));
        }
      } else if (i_vertical && j_vertical && first_i.get_x() == first_j.get_x()) {
        int32_t lower_y = std::max(std::min(first_i.get_y(), second_i.get_y()), std::min(first_j.get_y(), second_j.get_y()));
        int32_t upper_y = std::min(std::max(first_i.get_y(), second_i.get_y()), std::max(first_j.get_y(), second_j.get_y()));
        if (lower_y < upper_y) {
          addPlanarCoord(PlanarCoord(first_i.get_x(), lower_y));
          addPlanarCoord(PlanarCoord(first_i.get_x(), upper_y));
        }
      } else if (i_horizontal && j_vertical) {
        int32_t x = first_j.get_x();
        int32_t y = first_i.get_y();
        if (std::min(first_i.get_x(), second_i.get_x()) <= x && x <= std::max(first_i.get_x(), second_i.get_x())
            && std::min(first_j.get_y(), second_j.get_y()) <= y && y <= std::max(first_j.get_y(), second_j.get_y())) {
          addPlanarCoord(PlanarCoord(x, y));
        }
      } else if (i_vertical && j_horizontal) {
        int32_t x = first_i.get_x();
        int32_t y = first_j.get_y();
        if (std::min(first_j.get_x(), second_j.get_x()) <= x && x <= std::max(first_j.get_x(), second_j.get_x())
            && std::min(first_i.get_y(), second_i.get_y()) <= y && y <= std::max(first_i.get_y(), second_i.get_y())) {
          addPlanarCoord(PlanarCoord(x, y));
        }
      }
    }
  }

  std::set<PlanarEdgeKey> visited_planar_edge_key_set;
  for (PlanarWire& planar_wire : planar_wire_list) {
    PlanarCoord first_coord = planar_wire.first_coord;
    PlanarCoord second_coord = planar_wire.second_coord;
    std::vector<PlanarCoord> split_coord_list;
    if (first_coord.get_x() == second_coord.get_x()) {
      int32_t x = first_coord.get_x();
      int32_t lower_y = std::min(first_coord.get_y(), second_coord.get_y());
      int32_t upper_y = std::max(first_coord.get_y(), second_coord.get_y());
      for (int32_t y : y_cut_set_map[PlanarKey(x, 0)]) {
        if (lower_y <= y && y <= upper_y) {
          split_coord_list.emplace_back(x, y);
        }
      }
      std::sort(split_coord_list.begin(), split_coord_list.end(), CmpPlanarCoordByYASC());
    } else if (first_coord.get_y() == second_coord.get_y()) {
      int32_t y = first_coord.get_y();
      int32_t lower_x = std::min(first_coord.get_x(), second_coord.get_x());
      int32_t upper_x = std::max(first_coord.get_x(), second_coord.get_x());
      for (int32_t x : x_cut_set_map[PlanarKey(y, 0)]) {
        if (lower_x <= x && x <= upper_x) {
          split_coord_list.emplace_back(x, y);
        }
      }
      std::sort(split_coord_list.begin(), split_coord_list.end(), CmpPlanarCoordByXASC());
    }
    for (size_t i = 1; i < split_coord_list.size(); i++) {
      PlanarCoord first_split_coord = split_coord_list[i - 1];
      PlanarCoord second_split_coord = split_coord_list[i];
      if (first_split_coord == second_split_coord) {
        continue;
      }
      PlanarEdgeKey edge_key = makePlanarEdgeKey(first_split_coord, second_split_coord);
      projection.edge_segment_key_set_map[edge_key].insert(planar_wire.segment_key);
      projection.node_key_set.insert(makePlanarKey(first_split_coord));
      projection.node_key_set.insert(makePlanarKey(second_split_coord));
      if (visited_planar_edge_key_set.insert(edge_key).second) {
        projection.split_segment_list.emplace_back(first_split_coord, second_split_coord);
      }
    }
  }
  return projection;
}

std::vector<PlanarCoord> getLineCoordList(const PlanarCoord& first_coord, const PlanarCoord& second_coord)
{
  std::vector<PlanarCoord> coord_list;
  if (first_coord == second_coord) {
    coord_list.push_back(first_coord);
    return coord_list;
  }
  if (first_coord.get_x() == second_coord.get_x()) {
    int32_t y_step = first_coord.get_y() < second_coord.get_y() ? 1 : -1;
    for (int32_t y = first_coord.get_y(); y != second_coord.get_y(); y += y_step) {
      coord_list.emplace_back(first_coord.get_x(), y);
    }
    coord_list.push_back(second_coord);
  } else if (first_coord.get_y() == second_coord.get_y()) {
    int32_t x_step = first_coord.get_x() < second_coord.get_x() ? 1 : -1;
    for (int32_t x = first_coord.get_x(); x != second_coord.get_x(); x += x_step) {
      coord_list.emplace_back(x, first_coord.get_y());
    }
    coord_list.push_back(second_coord);
  }
  return coord_list;
}

void appendUniqueCoord(std::vector<PlanarCoord>& coord_list, const PlanarCoord& coord)
{
  if (coord_list.empty() || coord_list.back() != coord) {
    coord_list.push_back(coord);
  }
}

int32_t clampInt(const int32_t value, const int32_t low, const int32_t high)
{
  return std::max(low, std::min(high, value));
}

double calcUsageOverflow(GSRGridGraph& gsr_grid_graph, const std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC>& usage_map)
{
  double overflow = 0;
  for (auto& [usage_coord, orient_set] : usage_map) {
    (void) orient_set;
    if (gsr_grid_graph.isInside(usage_coord)) {
      overflow += gsr_grid_graph.get_layer_node_map()[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()].getOverflow();
    }
  }
  return overflow;
}

std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> getMergedRouteUsageMap(
    GSRGridGraph& gsr_grid_graph, const std::vector<Segment<LayerCoord>>& first_segment_list,
    const std::vector<Segment<LayerCoord>>& second_segment_list)
{
  std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> merged_usage_map = gsr_grid_graph.getRouteUsageMap(first_segment_list);
  for (auto& [usage_coord, orient_set] : gsr_grid_graph.getRouteUsageMap(second_segment_list)) {
    merged_usage_map[usage_coord].insert(orient_set.begin(), orient_set.end());
  }
  return merged_usage_map;
}

double getElapsedMs(const std::chrono::steady_clock::time_point& start_time)
{
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_time).count();
}

}  // namespace

// public

void GlobalSpatialRouter::initInst()
{
  if (_gsr_instance == nullptr) {
    _gsr_instance = new GlobalSpatialRouter();
  }
}

GlobalSpatialRouter& GlobalSpatialRouter::getInst()
{
  if (_gsr_instance == nullptr) {
    RTLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_gsr_instance;
}

void GlobalSpatialRouter::destroyInst()
{
  if (_gsr_instance != nullptr) {
    delete _gsr_instance;
    _gsr_instance = nullptr;
  }
}

// function

void GlobalSpatialRouter::route()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GSRRouteStats route_stats;
  GSRModel gsr_model = initGSRModel(route_stats);
  clearGlobalResult(route_stats);
  routeGSRModel(gsr_model, route_stats);
  rerouteGSRModel(gsr_model, route_stats);
  selectBestResult(gsr_model);
  uploadGSRModelResult(gsr_model, route_stats);
  outputGuide(gsr_model);
  updateHandoffStats(gsr_model, route_stats);
  outputSummaryCSV(gsr_model, route_stats);

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

GlobalSpatialRouter* GlobalSpatialRouter::_gsr_instance = nullptr;

// initialization

GSRModel GlobalSpatialRouter::initGSRModel(GSRRouteStats& route_stats)
{
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();

  GSRModel gsr_model;
  route_stats.total_net_num = static_cast<int32_t>(net_list.size());
  setGSRComParam(gsr_model);
  buildGSRGridGraph(gsr_model);

  std::vector<GSRNet> gsr_net_list;
  gsr_net_list.reserve(net_list.size());
  for (Net& net : net_list) {
    GSRNet gsr_net = convertToGSRNet(gsr_model, net, route_stats);
    if (static_cast<int32_t>(gsr_net.get_gsr_pin_list().size()) < 2) {
      route_stats.skipped_net_num++;
      continue;
    }
    route_stats.task_net_num++;
    gsr_net_list.push_back(gsr_net);
  }
  std::map<int32_t, GSRStage1RouteOrderKey> stage1_order_key_map;
  for (GSRNet& gsr_net : gsr_net_list) {
    stage1_order_key_map[gsr_net.get_net_idx()] = buildStage1RouteOrderKey(gsr_net);
  }
  std::sort(gsr_net_list.begin(), gsr_net_list.end(), [&](const GSRNet& a, const GSRNet& b) {
    const GSRStage1RouteOrderKey& a_key = stage1_order_key_map.at(a.get_net_idx());
    const GSRStage1RouteOrderKey& b_key = stage1_order_key_map.at(b.get_net_idx());
    if (a_key.is_clock != b_key.is_clock) {
      return a_key.is_clock;
    }
    if (a_key.bbox_total != b_key.bbox_total) {
      return a_key.bbox_total < b_key.bbox_total;
    }
    if (!RTUTIL.equalDoubleByError(a_key.aspect_ratio, b_key.aspect_ratio, RT_ERROR)) {
      return a_key.aspect_ratio > b_key.aspect_ratio;
    }
    if (a_key.pin_num != b_key.pin_num) {
      return a_key.pin_num > b_key.pin_num;
    }
    if (a_key.layer_span != b_key.layer_span) {
      return a_key.layer_span < b_key.layer_span;
    }
    if (!RTUTIL.equalDoubleByError(a_key.avg_access_layer, b_key.avg_access_layer, RT_ERROR)) {
      return a_key.avg_access_layer < b_key.avg_access_layer;
    }
    if (a_key.hpwl != b_key.hpwl) {
      return a_key.hpwl < b_key.hpwl;
    }
    return a_key.net_idx < b_key.net_idx;
  });
  outputStage1RouteOrderCSV(gsr_net_list);
  gsr_model.set_gsr_net_list(gsr_net_list);
  std::map<int32_t, int32_t> net_idx_to_gsr_net_idx_map;
  for (int32_t gsr_net_idx = 0; gsr_net_idx < static_cast<int32_t>(gsr_model.get_gsr_net_list().size()); gsr_net_idx++) {
    net_idx_to_gsr_net_idx_map[gsr_model.get_gsr_net_list()[gsr_net_idx].get_net_idx()] = gsr_net_idx;
  }
  gsr_model.set_net_idx_to_gsr_net_idx_map(net_idx_to_gsr_net_idx_map);
  return gsr_model;
}

GSRNet GlobalSpatialRouter::convertToGSRNet(GSRModel& gsr_model, Net& net, GSRRouteStats& route_stats)
{
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  int32_t bottom_routing_layer_idx = gsr_com_param.get_bottom_routing_layer_idx();
  int32_t top_routing_layer_idx = gsr_com_param.get_top_routing_layer_idx();

  GSRNet gsr_net;
  gsr_net.set_origin_net(&net);
  gsr_net.set_net_idx(net.get_net_idx());
  gsr_net.set_net_name(net.get_net_name());
  for (Pin& pin : net.get_pin_list()) {
    AccessPoint& access_point = pin.get_access_point();
    LayerCoord access_coord = access_point.getGridLayerCoord();
    int32_t access_layer_idx = access_coord.get_layer_idx();
    if (!gcell_map.isInside(access_coord.get_x(), access_coord.get_y()) || access_layer_idx < 0
        || static_cast<int32_t>(routing_layer_list.size()) <= access_layer_idx) {
      route_stats.invalid_access_point_num++;
      continue;
    }
    if (access_layer_idx < bottom_routing_layer_idx) {
      access_coord.set_layer_idx(bottom_routing_layer_idx);
    } else if (top_routing_layer_idx < access_layer_idx) {
      access_coord.set_layer_idx(top_routing_layer_idx);
    }
    gsr_net.get_gsr_pin_list().emplace_back(pin.get_pin_idx(), pin.get_pin_name(), access_coord);
  }
  return gsr_net;
}

void GlobalSpatialRouter::setGSRComParam(GSRModel& gsr_model)
{
  GSRComParam gsr_com_param = buildGSRComParam();

  RTLOG.info(Loc::current(), "bottom_routing_layer_idx: ", gsr_com_param.get_bottom_routing_layer_idx());
  RTLOG.info(Loc::current(), "top_routing_layer_idx: ", gsr_com_param.get_top_routing_layer_idx());
  RTLOG.info(Loc::current(), "horizontal_layer_idx: ", gsr_com_param.get_horizontal_layer_idx());
  RTLOG.info(Loc::current(), "vertical_layer_idx: ", gsr_com_param.get_vertical_layer_idx());
  RTLOG.info(Loc::current(), "unit_wire_cost: ", gsr_com_param.get_unit_wire_cost());
  gsr_model.set_gsr_com_param(gsr_com_param);
}

GSRComParam GlobalSpatialRouter::buildGSRComParam()
{
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  if (routing_layer_list.empty()) {
    RTLOG.error(Loc::current(), "The routing layer list is empty!");
  }

  int32_t bottom_routing_layer_idx = std::max(0, RTDM.getConfig().bottom_routing_layer_idx);
  int32_t top_routing_layer_idx = std::min(static_cast<int32_t>(routing_layer_list.size()) - 1, RTDM.getConfig().top_routing_layer_idx);
  if (top_routing_layer_idx < bottom_routing_layer_idx) {
    RTLOG.error(Loc::current(), "The routing layer range is invalid!");
  }

  int32_t horizontal_layer_idx = getLowestPreferLayerIdx(bottom_routing_layer_idx, top_routing_layer_idx, true);
  if (horizontal_layer_idx == -1) {
    RTLOG.error(Loc::current(), "No H-preferred routing layer in range!");
  }
  int32_t vertical_layer_idx = getLowestPreferLayerIdx(bottom_routing_layer_idx, top_routing_layer_idx, false);
  if (vertical_layer_idx == -1) {
    RTLOG.error(Loc::current(), "No V-preferred routing layer in range!");
  }
  GSRComParam gsr_com_param(bottom_routing_layer_idx, top_routing_layer_idx, horizontal_layer_idx, vertical_layer_idx);
  gsr_com_param.set_unit_wire_cost(1.0);
  gsr_com_param.set_unit_short_cost(10.0);
  gsr_com_param.set_unit_via_cost(5.0);
  gsr_com_param.set_cost_logistic_slope(1.0);
  gsr_com_param.set_maze_logistic_slope(1.0);
  gsr_com_param.set_via_min_area_demand_unit(0.5);
  gsr_com_param.set_via_multiplier(1.0);
  gsr_com_param.set_history_risk_decay(0.8);
  gsr_com_param.set_max_reroute_iter(3);
  gsr_com_param.set_max_routed_times(4);
  gsr_com_param.set_congestion_risk_radius(2);
  gsr_com_param.set_maze_window_size(8);
  gsr_com_param.set_maze_window_max_expand_times(3);
  gsr_com_param.set_min_reroute_task_num(2048);
  gsr_com_param.set_max_reroute_task_num(8192);
  gsr_com_param.set_reroute_task_growth_ratio(1.5);
  gsr_com_param.set_reroute_coverage_target(0.8);
  gsr_com_param.set_max_detour_ratio(0.5);
  gsr_com_param.set_target_detour_count(4);
  gsr_com_param.set_sparse_grid_interval(10);
  gsr_com_param.set_detour_congestion_threshold(0.0);
  gsr_com_param.set_congestion_risk_threshold(0.1);
  gsr_com_param.set_near_full_usage_ratio(0.85);
  gsr_com_param.set_congestion_view_radius(3);
  gsr_com_param.set_congestion_overflow_weight(1.0);
  gsr_com_param.set_congestion_near_full_weight(0.25);
  gsr_com_param.set_congestion_internal_weight(0.5);
  gsr_com_param.set_congestion_via_weight(0.5);
  gsr_com_param.set_congestion_history_weight(0.5);
  return gsr_com_param;
}

int32_t GlobalSpatialRouter::getLowestPreferLayerIdx(const int32_t bottom_routing_layer_idx, const int32_t top_routing_layer_idx, const bool prefer_h)
{
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  for (int32_t layer_idx = bottom_routing_layer_idx; layer_idx <= top_routing_layer_idx; layer_idx++) {
    if (routing_layer_list[layer_idx].isPreferH() == prefer_h) {
      return layer_idx;
    }
  }
  return -1;
}

void GlobalSpatialRouter::buildGSRGridGraph(GSRModel& gsr_model)
{
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  GSRGridGraph gsr_grid_graph(gcell_map.get_x_size(), gcell_map.get_y_size(), gsr_model.get_gsr_com_param());
  gsr_grid_graph.initFromGCellMap(gcell_map);
  gsr_model.set_gsr_grid_graph(gsr_grid_graph);
}

void GlobalSpatialRouter::clearGlobalResult(GSRRouteStats& route_stats)
{
  Die& die = RTDM.getDatabase().get_die();
  std::map<int32_t, std::set<Segment<LayerCoord>*>> net_global_result_map = RTDM.getNetGlobalResultMap(die);
  for (auto& [net_idx, segment_set] : net_global_result_map) {
    for (Segment<LayerCoord>* segment : segment_set) {
      RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, segment);
      route_stats.cleared_segment_num++;
    }
  }
}

// routing flow

void GlobalSpatialRouter::routeGSRModel(GSRModel& gsr_model, GSRRouteStats& route_stats)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    routeGSRNet(gsr_model, gsr_net, route_stats);
  }
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  gsr_model.get_gsr_grid_graph().updateCongestionRisk(gsr_model.get_layer_congestion_risk_map(), gsr_com_param.get_congestion_risk_radius(),
                                                    gsr_com_param.get_history_risk_decay());
  updateGSRModelCost(gsr_model);
  updateBestResult(gsr_model);

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void GlobalSpatialRouter::routeGSRNet(GSRModel& gsr_model, GSRNet& gsr_net, GSRRouteStats& route_stats)
{
  std::vector<Segment<LayerCoord>> routing_segment_list = routeByPattern(gsr_model, gsr_net, nullptr, false, route_stats);
  if (!isRouteConnected(gsr_net, routing_segment_list)) {
    std::vector<Segment<PlanarCoord>> planar_topo_list = getPlanarTopoList(gsr_net, route_stats);
    routing_segment_list = getRoutingSegmentList(gsr_model, gsr_net, planar_topo_list, route_stats);
    routing_segment_list = getValidUniqueSegmentList(gsr_model, routing_segment_list, route_stats);
    if (!isRouteConnected(gsr_net, routing_segment_list)) {
      routing_segment_list.clear();
    }
  }
  gsr_net.set_routing_segment_list(routing_segment_list);
  addRouteDemand(gsr_model, gsr_net, routing_segment_list);
  updateGSRNetCost(gsr_model, gsr_net);
  gsr_net.addRoutedTimes();

  if (!routing_segment_list.empty()) {
    route_stats.routed_net_num++;
  }
  if (std::none_of(routing_segment_list.begin(), routing_segment_list.end(), [](const Segment<LayerCoord>& segment) {
        return segment.get_first().get_layer_idx() == segment.get_second().get_layer_idx();
      })) {
    route_stats.net_without_ta_visible_segment_num++;
  }
}

void GlobalSpatialRouter::rerouteGSRModel(GSRModel& gsr_model, GSRRouteStats& route_stats)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  initRerouteDiagnostics();
  auto step_start_time = std::chrono::steady_clock::now();
  updateGSRModelCost(gsr_model);
  appendRerouteTiming(-1, 0, "update_model_cost_initial", getElapsedMs(step_start_time), 1);
  gsr_model.set_stage1_total_overflow(gsr_model.get_total_overflow());
  for (int32_t iter = 0; iter < gsr_com_param.get_max_reroute_iter(); iter++) {
    auto iter_start_time = std::chrono::steady_clock::now();
    double iter_start_overflow = gsr_model.get_total_overflow();
    double iter_start_congestion_risk = gsr_model.get_total_congestion_risk();

    step_start_time = std::chrono::steady_clock::now();
    gsr_model.get_gsr_grid_graph().updateCongestionRisk(gsr_model.get_layer_congestion_risk_map(), gsr_com_param.get_congestion_risk_radius(),
                                                      gsr_com_param.get_history_risk_decay());
    appendRerouteTiming(iter, 0, "update_congestion_risk", getElapsedMs(step_start_time), 1);
    step_start_time = std::chrono::steady_clock::now();
    updateGSRModelCost(gsr_model);
    appendRerouteTiming(iter, 0, "update_model_cost_before", getElapsedMs(step_start_time), 1);
    step_start_time = std::chrono::steady_clock::now();
    GSRCongestionView congestion_view = extractCongestionView(gsr_model);
    appendRerouteTiming(iter, 2, "extract_congestion_view", getElapsedMs(step_start_time), 1);
    route_stats.congestion_view_h_risk_sum = congestion_view.total_h_risk;
    route_stats.congestion_view_v_risk_sum = congestion_view.total_v_risk;
    route_stats.congestion_view_hotspot_sum = congestion_view.total_hotspot;
    appendRerouteIterSummary(gsr_model, &congestion_view, iter, "before", iter_start_overflow, iter_start_congestion_risk, 0, 0, 0, 0);
    outputOverflowHotspotCSV(gsr_model, congestion_view, iter, "before");
    step_start_time = std::chrono::steady_clock::now();
    std::vector<GSRNet*> stage2_task_list = getRerouteTaskList(gsr_model, congestion_view, iter, false, route_stats, false);
    appendRerouteTiming(iter, 2, "get_stage2_task", getElapsedMs(step_start_time), 1);
    if (stage2_task_list.empty() || gsr_model.get_total_overflow() <= 0) {
      appendRerouteTiming(iter, 0, "reroute_iter_total", getElapsedMs(iter_start_time), 1);
      break;
    }

    route_stats.reroute_iter_num++;
    route_stats.reroute_task_num += static_cast<int32_t>(stage2_task_list.size());
    route_stats.stage2_task_num += static_cast<int32_t>(stage2_task_list.size());
    RTLOG.info(Loc::current(), "GlobalSpatialRouter stage2 detour iter ", iter, " task_num: ", stage2_task_list.size(), " total_overflow: ",
               gsr_model.get_total_overflow(), " total_congestion_risk: ", gsr_model.get_total_congestion_risk());

    int32_t accepted_num = 0;
    initRerouteAttemptCSV(iter, 2);
    step_start_time = std::chrono::steady_clock::now();
    for (GSRNet* gsr_net : stage2_task_list) {
      GSRRerouteAttemptRecord attempt_record;
      attempt_record.iter = iter;
      attempt_record.stage = 2;
      if (tryDetourNet(gsr_model, *gsr_net, congestion_view, route_stats, &attempt_record)) {
        accepted_num++;
      }
      outputRerouteAttemptCSV(attempt_record);
    }
    flushRerouteAttemptCSV();
    appendRerouteTiming(iter, 2, "stage2_detour_total", getElapsedMs(step_start_time), static_cast<int32_t>(stage2_task_list.size()));
    step_start_time = std::chrono::steady_clock::now();
    updateGSRModelCost(gsr_model);
    appendRerouteTiming(iter, 2, "update_model_cost_after_stage2", getElapsedMs(step_start_time), 1);
    gsr_model.set_stage2_total_overflow(gsr_model.get_total_overflow());
    step_start_time = std::chrono::steady_clock::now();
    updateBestResult(gsr_model);
    appendRerouteTiming(iter, 2, "update_best_result_after_stage2", getElapsedMs(step_start_time), 1);
    RTLOG.info(Loc::current(), "GlobalSpatialRouter stage2 detour iter ", iter, " accepted_num: ", accepted_num, " total_overflow: ",
               gsr_model.get_total_overflow(), " total_congestion_risk: ", gsr_model.get_total_congestion_risk());

    step_start_time = std::chrono::steady_clock::now();
    congestion_view = extractCongestionView(gsr_model);
    appendRerouteTiming(iter, 2, "extract_congestion_view_after_stage2", getElapsedMs(step_start_time), 1);
    appendRerouteIterSummary(gsr_model, &congestion_view, iter, "after_stage2", iter_start_overflow, iter_start_congestion_risk,
                             static_cast<int32_t>(stage2_task_list.size()), accepted_num, 0, 0);
    outputOverflowHotspotCSV(gsr_model, congestion_view, iter, "after_stage2");

    if (gsr_model.get_total_overflow() <= 0) {
      appendRerouteTiming(iter, 0, "reroute_iter_total", getElapsedMs(iter_start_time), 1);
      break;
    }

    step_start_time = std::chrono::steady_clock::now();
    GSRWireCostView wire_cost_view = extractWireCostView(gsr_model);
    appendRerouteTiming(iter, 3, "extract_wire_cost_view", getElapsedMs(step_start_time), 1);
    step_start_time = std::chrono::steady_clock::now();
    std::vector<GSRNet*> stage3_task_list = getRerouteTaskList(gsr_model, congestion_view, iter, true, route_stats, true);
    appendRerouteTiming(iter, 3, "get_stage3_task", getElapsedMs(step_start_time), 1);
    route_stats.reroute_task_num += static_cast<int32_t>(stage3_task_list.size());
    route_stats.stage3_task_num += static_cast<int32_t>(stage3_task_list.size());
    RTLOG.info(Loc::current(), "GlobalSpatialRouter stage3 sparse maze iter ", iter, " task_num: ", stage3_task_list.size(), " total_overflow: ",
               gsr_model.get_total_overflow(), " total_congestion_risk: ", gsr_model.get_total_congestion_risk());

    int32_t stage3_accepted_num = 0;
    initRerouteAttemptCSV(iter, 3);
    step_start_time = std::chrono::steady_clock::now();
    for (GSRNet* gsr_net : stage3_task_list) {
      GSRRerouteAttemptRecord attempt_record;
      attempt_record.iter = iter;
      attempt_record.stage = 3;
      if (trySparseMazeNet(gsr_model, *gsr_net, congestion_view, wire_cost_view, route_stats, &attempt_record)) {
        stage3_accepted_num++;
        updateWireCostView(gsr_model, wire_cost_view, gsr_net->get_routing_segment_list(), &route_stats);
      }
      outputRerouteAttemptCSV(attempt_record);
    }
    flushRerouteAttemptCSV();
    appendRerouteTiming(iter, 3, "stage3_maze_total", getElapsedMs(step_start_time), static_cast<int32_t>(stage3_task_list.size()));
    step_start_time = std::chrono::steady_clock::now();
    updateGSRModelCost(gsr_model);
    appendRerouteTiming(iter, 3, "update_model_cost_after_stage3", getElapsedMs(step_start_time), 1);
    gsr_model.set_stage3_total_overflow(gsr_model.get_total_overflow());
    step_start_time = std::chrono::steady_clock::now();
    updateBestResult(gsr_model);
    appendRerouteTiming(iter, 3, "update_best_result_after_stage3", getElapsedMs(step_start_time), 1);
    RTLOG.info(Loc::current(), "GlobalSpatialRouter stage3 sparse maze iter ", iter, " accepted_num: ", stage3_accepted_num, " total_overflow: ",
               gsr_model.get_total_overflow(), " total_congestion_risk: ", gsr_model.get_total_congestion_risk());
    step_start_time = std::chrono::steady_clock::now();
    congestion_view = extractCongestionView(gsr_model);
    appendRerouteTiming(iter, 3, "extract_congestion_view_after_stage3", getElapsedMs(step_start_time), 1);
    appendRerouteIterSummary(gsr_model, &congestion_view, iter, "after_stage3", iter_start_overflow, iter_start_congestion_risk,
                             static_cast<int32_t>(stage2_task_list.size()), accepted_num, static_cast<int32_t>(stage3_task_list.size()),
                             stage3_accepted_num);
    outputOverflowHotspotCSV(gsr_model, congestion_view, iter, "after_stage3");
    appendRerouteTiming(iter, 0, "reroute_iter_total", getElapsedMs(iter_start_time), 1);
  }
  gsr_model.set_stage3_total_overflow(gsr_model.get_total_overflow());
  flushRerouteAttemptCSV();
  flushRerouteTimingCSV();

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// congestion and cost views

GlobalSpatialRouter::GSRCongestionView GlobalSpatialRouter::extractCongestionView(GSRModel& gsr_model)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();

  GSRCongestionView congestion_view;
  congestion_view.h_overflow_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.v_overflow_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.h_risk_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.v_risk_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.h_near_full_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.v_near_full_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.internal_overflow_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.via_risk_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.capacity_pressure_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.capacity_block_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  congestion_view.hotspot_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0.0);
  GridMap<int32_t> task_coord_mask;
  task_coord_mask.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 0);

  int32_t risk_radius = std::max(0, gsr_com_param.get_congestion_view_radius());
  double risk_threshold = std::max(0.0, gsr_com_param.get_congestion_risk_threshold());
  double near_full_threshold = std::max(0.0, gsr_com_param.get_near_full_usage_ratio());
  auto getNearFull = [&](const double demand, const double supply) {
    if (demand <= 0) {
      return 0.0;
    }
    if (supply <= 0) {
      return demand;
    }
    double usage_ratio = demand / supply;
    if (usage_ratio < near_full_threshold) {
      return 0.0;
    }
    double normalized = (usage_ratio - near_full_threshold) / std::max(RT_ERROR, 1.0 - near_full_threshold);
    return std::pow(normalized, 2);
  };
  auto markTaskCoord = [&](const int32_t x, const int32_t y) {
    if (!task_coord_mask.isInside(x, y) || task_coord_mask[x][y] != 0) {
      return;
    }
    task_coord_mask[x][y] = 1;
    congestion_view.task_coord_list.emplace_back(x, y);
  };

  for (int32_t layer_idx = gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
    GridMap<GSRNode>& gsr_node_map = gsr_grid_graph.get_layer_node_map()[layer_idx];
    bool prefer_h = routing_layer_list[layer_idx].isPreferH();
    for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
      for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
        GSRNode& gsr_node = gsr_node_map[x][y];
        double h_overflow = 0;
        double v_overflow = 0;
        double h_near_full = 0;
        double v_near_full = 0;
        for (Orientation orient : {Orientation::kEast, Orientation::kWest}) {
          double demand = gsr_node.getBoundaryDemand(orient);
          double supply = gsr_node.getSupply(orient);
          h_overflow += std::max(0.0, demand - supply);
          h_near_full += getNearFull(demand, supply);
        }
        for (Orientation orient : {Orientation::kNorth, Orientation::kSouth}) {
          double demand = gsr_node.getBoundaryDemand(orient);
          double supply = gsr_node.getSupply(orient);
          v_overflow += std::max(0.0, demand - supply);
          v_near_full += getNearFull(demand, supply);
        }
        double internal_demand = gsr_node.getInternalDemand();
        double internal_supply = gsr_node.getInternalSupply();
        double internal_overflow = std::max(0.0, internal_demand - internal_supply);
        double internal_near_full = getNearFull(internal_demand, internal_supply);
        double via_risk = 0;
        for (auto& [net_idx, orient_set] : gsr_node.get_net_orient_map()) {
          (void) net_idx;
          if (RTUTIL.exist(orient_set, Orientation::kAbove) || RTUTIL.exist(orient_set, Orientation::kBelow)) {
            via_risk += gsr_com_param.get_via_min_area_demand_unit() * gsr_com_param.get_via_multiplier();
          }
        }
        via_risk *= getNearFull(internal_demand + via_risk, internal_supply) > 0 ? 1.0 : 0.25;
        double capacity_pressure = internal_overflow + internal_near_full + via_risk;
        double capacity_block = 0;
        if ((internal_supply <= RT_ERROR && internal_demand > 0) || internal_overflow > RT_ERROR) {
          capacity_block = 1.0;
        }

        if (prefer_h) {
          congestion_view.h_overflow_map[x][y] += h_overflow;
          congestion_view.h_near_full_map[x][y] += h_near_full;
        } else {
          congestion_view.v_overflow_map[x][y] += v_overflow;
          congestion_view.v_near_full_map[x][y] += v_near_full;
        }
        congestion_view.internal_overflow_map[x][y] += internal_overflow + internal_near_full;
        congestion_view.via_risk_map[x][y] += via_risk;
        congestion_view.capacity_pressure_map[x][y] += capacity_pressure;
        congestion_view.capacity_block_map[x][y] += capacity_block;

        double history_risk = gsr_node.get_congestion_risk();
        double raw_h_risk = gsr_com_param.get_congestion_overflow_weight() * (prefer_h ? h_overflow : 0)
                            + gsr_com_param.get_congestion_near_full_weight() * (prefer_h ? h_near_full : 0)
                            + gsr_com_param.get_congestion_internal_weight() * (internal_overflow + internal_near_full)
                            + gsr_com_param.get_congestion_via_weight() * via_risk
                            + gsr_com_param.get_congestion_history_weight() * history_risk;
        double raw_v_risk = gsr_com_param.get_congestion_overflow_weight() * (prefer_h ? 0 : v_overflow)
                            + gsr_com_param.get_congestion_near_full_weight() * (prefer_h ? 0 : v_near_full)
                            + gsr_com_param.get_congestion_internal_weight() * (internal_overflow + internal_near_full)
                            + gsr_com_param.get_congestion_via_weight() * via_risk
                            + gsr_com_param.get_congestion_history_weight() * history_risk;
        double hotspot = h_overflow + v_overflow + internal_overflow + via_risk + h_near_full + v_near_full;
        if (h_overflow > 0 || v_overflow > 0 || internal_overflow > 0 || h_near_full > 0 || v_near_full > 0
            || raw_h_risk > risk_threshold || raw_v_risk > risk_threshold || hotspot > risk_threshold) {
          markTaskCoord(x, y);
        }
        if (raw_h_risk > 0) {
          addCongestionRisk(congestion_view.h_risk_map, x, y, raw_h_risk, risk_radius);
        }
        if (raw_v_risk > 0) {
          addCongestionRisk(congestion_view.v_risk_map, x, y, raw_v_risk, risk_radius);
        }
        if (hotspot > 0) {
          addCongestionRisk(congestion_view.hotspot_map, x, y, hotspot, risk_radius);
        }
      }
    }
  }
  for (int32_t x = 0; x < gsr_grid_graph.get_x_size(); x++) {
    for (int32_t y = 0; y < gsr_grid_graph.get_y_size(); y++) {
      double h_overflow = congestion_view.h_overflow_map[x][y];
      double v_overflow = congestion_view.v_overflow_map[x][y];
      double hotspot = congestion_view.hotspot_map[x][y];
      congestion_view.total_h_risk += congestion_view.h_risk_map[x][y];
      congestion_view.total_v_risk += congestion_view.v_risk_map[x][y];
      congestion_view.total_hotspot += hotspot;
      if (h_overflow > 0 || v_overflow > 0 || congestion_view.internal_overflow_map[x][y] > 0) {
        congestion_view.overflow_cell_num++;
      }
      if (hotspot > gsr_com_param.get_congestion_risk_threshold()) {
        congestion_view.hotspot_cell_num++;
      }
    }
  }
  rebuildCongestionRiskPrefix(congestion_view);
  rebuildCapacityPrefix(congestion_view);
  return congestion_view;
}

void GlobalSpatialRouter::addCongestionRisk(GridMap<double>& risk_map, const int32_t x, const int32_t y, const double value, const int32_t risk_radius)
{
  if (value <= 0) {
    return;
  }
  for (int32_t risk_x = x - risk_radius; risk_x <= x + risk_radius; risk_x++) {
    for (int32_t risk_y = y - risk_radius; risk_y <= y + risk_radius; risk_y++) {
      if (!risk_map.isInside(risk_x, risk_y)) {
        continue;
      }
      int32_t distance = std::abs(risk_x - x) + std::abs(risk_y - y);
      if (risk_radius < distance) {
        continue;
      }
      double decay = risk_radius == 0 ? 1.0 : static_cast<double>(risk_radius - distance + 1) / static_cast<double>(risk_radius + 1);
      risk_map[risk_x][risk_y] += value * decay;
    }
  }
}

void GlobalSpatialRouter::rebuildCongestionRiskPrefix(GSRCongestionView& congestion_view)
{
  int32_t x_size = congestion_view.h_risk_map.get_x_size();
  int32_t y_size = congestion_view.h_risk_map.get_y_size();
  congestion_view.h_risk_prefix_sum_map.init(x_size + 1, y_size, 0.0);
  congestion_view.v_risk_prefix_sum_map.init(x_size, y_size + 1, 0.0);
  for (int32_t y = 0; y < y_size; y++) {
    for (int32_t x = 0; x < x_size; x++) {
      congestion_view.h_risk_prefix_sum_map[x + 1][y]
          = congestion_view.h_risk_prefix_sum_map[x][y] + congestion_view.h_risk_map[x][y];
    }
  }
  for (int32_t x = 0; x < x_size; x++) {
    for (int32_t y = 0; y < y_size; y++) {
      congestion_view.v_risk_prefix_sum_map[x][y + 1]
          = congestion_view.v_risk_prefix_sum_map[x][y] + congestion_view.v_risk_map[x][y];
    }
  }
}

void GlobalSpatialRouter::rebuildCapacityPrefix(GSRCongestionView& congestion_view)
{
  int32_t x_size = congestion_view.capacity_pressure_map.get_x_size();
  int32_t y_size = congestion_view.capacity_pressure_map.get_y_size();
  congestion_view.h_capacity_prefix_sum_map.init(x_size + 1, y_size, 0.0);
  congestion_view.v_capacity_prefix_sum_map.init(x_size, y_size + 1, 0.0);
  congestion_view.h_block_prefix_sum_map.init(x_size + 1, y_size, 0.0);
  congestion_view.v_block_prefix_sum_map.init(x_size, y_size + 1, 0.0);
  for (int32_t y = 0; y < y_size; y++) {
    for (int32_t x = 0; x < x_size; x++) {
      congestion_view.h_capacity_prefix_sum_map[x + 1][y]
          = congestion_view.h_capacity_prefix_sum_map[x][y] + congestion_view.capacity_pressure_map[x][y];
      congestion_view.h_block_prefix_sum_map[x + 1][y]
          = congestion_view.h_block_prefix_sum_map[x][y] + congestion_view.capacity_block_map[x][y];
    }
  }
  for (int32_t x = 0; x < x_size; x++) {
    for (int32_t y = 0; y < y_size; y++) {
      congestion_view.v_capacity_prefix_sum_map[x][y + 1]
          = congestion_view.v_capacity_prefix_sum_map[x][y] + congestion_view.capacity_pressure_map[x][y];
      congestion_view.v_block_prefix_sum_map[x][y + 1]
          = congestion_view.v_block_prefix_sum_map[x][y] + congestion_view.capacity_block_map[x][y];
    }
  }
}

double GlobalSpatialRouter::getCongestionRisk(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& coord)
{
  if (direction == Direction::kHorizontal && congestion_view.h_risk_map.isInside(coord.get_x(), coord.get_y())) {
    return congestion_view.h_risk_map[coord.get_x()][coord.get_y()];
  }
  if (direction == Direction::kVertical && congestion_view.v_risk_map.isInside(coord.get_x(), coord.get_y())) {
    return congestion_view.v_risk_map[coord.get_x()][coord.get_y()];
  }
  return 0;
}

double GlobalSpatialRouter::queryLinePrefixSum(const GridMap<double>& h_prefix_sum_map, const GridMap<double>& v_prefix_sum_map, const Direction direction,
                                               const PlanarCoord& first_coord, const PlanarCoord& second_coord, const bool include_end,
                                               const double invalid_value)
{
  if (direction == Direction::kHorizontal && first_coord.get_y() == second_coord.get_y()) {
    int32_t low_x = std::min(first_coord.get_x(), second_coord.get_x());
    int32_t high_x = std::max(first_coord.get_x(), second_coord.get_x()) + (include_end ? 1 : 0);
    int32_t y = first_coord.get_y();
    if (!h_prefix_sum_map.isInside(low_x, y) || !h_prefix_sum_map.isInside(high_x, y)) {
      return invalid_value;
    }
    return h_prefix_sum_map[high_x][y] - h_prefix_sum_map[low_x][y];
  }
  if (direction == Direction::kVertical && first_coord.get_x() == second_coord.get_x()) {
    int32_t low_y = std::min(first_coord.get_y(), second_coord.get_y());
    int32_t high_y = std::max(first_coord.get_y(), second_coord.get_y()) + (include_end ? 1 : 0);
    int32_t x = first_coord.get_x();
    if (!v_prefix_sum_map.isInside(x, low_y) || !v_prefix_sum_map.isInside(x, high_y)) {
      return invalid_value;
    }
    return v_prefix_sum_map[x][high_y] - v_prefix_sum_map[x][low_y];
  }
  return invalid_value;
}

double GlobalSpatialRouter::getCongestionRiskSum(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& first_coord,
                                const PlanarCoord& second_coord)
{
  if (first_coord == second_coord) {
    return getCongestionRisk(congestion_view, direction, first_coord);
  }
  return queryLinePrefixSum(congestion_view.h_risk_prefix_sum_map, congestion_view.v_risk_prefix_sum_map, direction, first_coord, second_coord,
                            true, 0);
}

double GlobalSpatialRouter::getCapacityPressure(GSRCongestionView& congestion_view, const PlanarCoord& coord)
{
  if (!congestion_view.capacity_pressure_map.isInside(coord.get_x(), coord.get_y())) {
    return 0;
  }
  return congestion_view.capacity_pressure_map[coord.get_x()][coord.get_y()];
}

double GlobalSpatialRouter::getCapacityBlock(GSRCongestionView& congestion_view, const PlanarCoord& coord)
{
  if (!congestion_view.capacity_block_map.isInside(coord.get_x(), coord.get_y())) {
    return 0;
  }
  return congestion_view.capacity_block_map[coord.get_x()][coord.get_y()];
}

double GlobalSpatialRouter::getCapacityPressureSum(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& first_coord,
                                  const PlanarCoord& second_coord)
{
  if (first_coord == second_coord) {
    return getCapacityPressure(congestion_view, first_coord);
  }
  return queryLinePrefixSum(congestion_view.h_capacity_prefix_sum_map, congestion_view.v_capacity_prefix_sum_map, direction, first_coord,
                            second_coord, true, 0);
}

double GlobalSpatialRouter::getCapacityBlockSum(GSRCongestionView& congestion_view, const Direction direction, const PlanarCoord& first_coord,
                               const PlanarCoord& second_coord)
{
  if (first_coord == second_coord) {
    return getCapacityBlock(congestion_view, first_coord);
  }
  return queryLinePrefixSum(congestion_view.h_block_prefix_sum_map, congestion_view.v_block_prefix_sum_map, direction, first_coord, second_coord,
                            true, 0);
}

GlobalSpatialRouter::GSRWireCostView GlobalSpatialRouter::extractWireCostView(GSRModel& gsr_model)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  std::vector<int32_t> preferred_h_layer_list = getCandidateLayerList(gsr_com_param, true);
  std::vector<int32_t> preferred_v_layer_list = getCandidateLayerList(gsr_com_param, false);

  GSRWireCostView wire_cost_view;
  wire_cost_view.h_cost_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 1.0e12);
  wire_cost_view.v_cost_map.init(gsr_grid_graph.get_x_size(), gsr_grid_graph.get_y_size(), 1.0e12);
  for (int32_t x = 0; x < gsr_grid_graph.get_x_size(); x++) {
    for (int32_t y = 0; y < gsr_grid_graph.get_y_size(); y++) {
      PlanarCoord curr_coord(x, y);
      if (x + 1 < gsr_grid_graph.get_x_size()) {
        wire_cost_view.h_cost_map[x][y] = gsr_grid_graph.getBestProjectedPlanarEdgeCost(
            -1, Direction::kHorizontal, curr_coord, PlanarCoord(x + 1, y), preferred_h_layer_list, gsr_com_param,
            gsr_com_param.get_maze_logistic_slope());
      }
      if (y + 1 < gsr_grid_graph.get_y_size()) {
        wire_cost_view.v_cost_map[x][y] = gsr_grid_graph.getBestProjectedPlanarEdgeCost(
            -1, Direction::kVertical, curr_coord, PlanarCoord(x, y + 1), preferred_v_layer_list, gsr_com_param,
            gsr_com_param.get_maze_logistic_slope());
      }
    }
  }
  rebuildWireCostPrefix(wire_cost_view);
  return wire_cost_view;
}

void GlobalSpatialRouter::rebuildWireCostPrefix(GSRWireCostView& wire_cost_view)
{
  int32_t x_size = wire_cost_view.h_cost_map.get_x_size();
  int32_t y_size = wire_cost_view.h_cost_map.get_y_size();
  wire_cost_view.h_prefix_sum_map.init(x_size + 1, y_size, 0.0);
  wire_cost_view.v_prefix_sum_map.init(x_size, y_size + 1, 0.0);
  for (int32_t y = 0; y < y_size; y++) {
    for (int32_t x = 0; x < x_size; x++) {
      wire_cost_view.h_prefix_sum_map[x + 1][y] = wire_cost_view.h_prefix_sum_map[x][y] + wire_cost_view.h_cost_map[x][y];
    }
  }
  for (int32_t x = 0; x < x_size; x++) {
    for (int32_t y = 0; y < y_size; y++) {
      wire_cost_view.v_prefix_sum_map[x][y + 1] = wire_cost_view.v_prefix_sum_map[x][y] + wire_cost_view.v_cost_map[x][y];
    }
  }
}

void GlobalSpatialRouter::updateWireCostView(GSRModel& gsr_model, GSRWireCostView& wire_cost_view, const std::vector<Segment<LayerCoord>>& segment_list,
                            GSRRouteStats* route_stats)
{
  if (segment_list.empty()) {
    return;
  }

  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  std::vector<int32_t> preferred_h_layer_list = getCandidateLayerList(gsr_com_param, true);
  std::vector<int32_t> preferred_v_layer_list = getCandidateLayerList(gsr_com_param, false);
  std::map<int32_t, std::set<int32_t>> dirty_h_edge_x_set_by_y;
  std::map<int32_t, std::set<int32_t>> dirty_v_edge_y_set_by_x;

  auto markDirtyEdges = [&](const PlanarCoord& coord) {
    if (!gsr_grid_graph.isInside(coord)) {
      return;
    }
    int32_t x = coord.get_x();
    int32_t y = coord.get_y();
    if (0 <= x - 1) {
      dirty_h_edge_x_set_by_y[y].insert(x - 1);
    }
    if (x + 1 < gsr_grid_graph.get_x_size()) {
      dirty_h_edge_x_set_by_y[y].insert(x);
    }
    if (0 <= y - 1) {
      dirty_v_edge_y_set_by_x[x].insert(y - 1);
    }
    if (y + 1 < gsr_grid_graph.get_y_size()) {
      dirty_v_edge_y_set_by_x[x].insert(y);
    }
  };

  for (const Segment<LayerCoord>& segment : segment_list) {
    for (auto& [usage_coord, orient_set] : gsr_grid_graph.getRouteUsageMap({segment})) {
      (void) orient_set;
      markDirtyEdges(usage_coord.get_planar_coord());
    }
  }

  auto updateHCost = [&](const int32_t x, const int32_t y) {
    if (x + 1 >= gsr_grid_graph.get_x_size()) {
      wire_cost_view.h_cost_map[x][y] = 1.0e12;
      return;
    }
    wire_cost_view.h_cost_map[x][y]
        = gsr_grid_graph.getBestProjectedPlanarEdgeCost(-1, Direction::kHorizontal, PlanarCoord(x, y), PlanarCoord(x + 1, y),
                                                       preferred_h_layer_list, gsr_com_param, gsr_com_param.get_maze_logistic_slope());
  };

  auto updateVCost = [&](const int32_t x, const int32_t y) {
    if (y + 1 >= gsr_grid_graph.get_y_size()) {
      wire_cost_view.v_cost_map[x][y] = 1.0e12;
      return;
    }
    wire_cost_view.v_cost_map[x][y]
        = gsr_grid_graph.getBestProjectedPlanarEdgeCost(-1, Direction::kVertical, PlanarCoord(x, y), PlanarCoord(x, y + 1),
                                                       preferred_v_layer_list, gsr_com_param, gsr_com_param.get_maze_logistic_slope());
  };

  for (auto& [y, dirty_x_set] : dirty_h_edge_x_set_by_y) {
    if (y < 0 || wire_cost_view.h_cost_map.get_y_size() <= y) {
      continue;
    }
    for (int32_t x : dirty_x_set) {
      if (0 <= x && x < wire_cost_view.h_cost_map.get_x_size()) {
        updateHCost(x, y);
        if (route_stats != nullptr) {
          route_stats->wire_dirty_h_edge_num++;
        }
      }
    }
    wire_cost_view.h_prefix_sum_map[0][y] = 0.0;
    for (int32_t x = 0; x < wire_cost_view.h_cost_map.get_x_size(); x++) {
      wire_cost_view.h_prefix_sum_map[x + 1][y] = wire_cost_view.h_prefix_sum_map[x][y] + wire_cost_view.h_cost_map[x][y];
    }
    if (route_stats != nullptr) {
      route_stats->wire_dirty_h_row_num++;
    }
  }
  for (auto& [x, dirty_y_set] : dirty_v_edge_y_set_by_x) {
    if (x < 0 || wire_cost_view.v_cost_map.get_x_size() <= x) {
      continue;
    }
    for (int32_t y : dirty_y_set) {
      if (0 <= y && y < wire_cost_view.v_cost_map.get_y_size()) {
        updateVCost(x, y);
        if (route_stats != nullptr) {
          route_stats->wire_dirty_v_edge_num++;
        }
      }
    }
    wire_cost_view.v_prefix_sum_map[x][0] = 0.0;
    for (int32_t y = 0; y < wire_cost_view.v_cost_map.get_y_size(); y++) {
      wire_cost_view.v_prefix_sum_map[x][y + 1] = wire_cost_view.v_prefix_sum_map[x][y] + wire_cost_view.v_cost_map[x][y];
    }
    if (route_stats != nullptr) {
      route_stats->wire_dirty_v_col_num++;
    }
  }
}

double GlobalSpatialRouter::getWireCost(GSRWireCostView& wire_cost_view, const Direction direction, const PlanarCoord& first_coord, const PlanarCoord& second_coord)
{
  if (first_coord == second_coord) {
    return 0;
  }
  return queryLinePrefixSum(wire_cost_view.h_prefix_sum_map, wire_cost_view.v_prefix_sum_map, direction, first_coord, second_coord, false, 1.0e12);
}

// reroute task selection and attempts

std::vector<GSRNet*> GlobalSpatialRouter::getRerouteTaskList(GSRModel& gsr_model, GSRCongestionView& congestion_view, const int32_t iter,
                                           const bool prefer_uncovered, GSRRouteStats& route_stats, const bool stage3)
{
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  std::vector<GSRNet>& gsr_net_list = gsr_model.get_gsr_net_list();
  std::map<int32_t, int32_t>& net_idx_to_gsr_net_idx_map = gsr_model.get_net_idx_to_gsr_net_idx_map();
  std::vector<GridMap<GSRNode>>& layer_node_map = gsr_grid_graph.get_layer_node_map();

  std::map<int32_t, GSRRerouteTask> task_map;
  std::set<int32_t> overflow_net_idx_set;
  std::set<PlanarKey> overflow_cell_key_set;

  auto getTask = [&](const int32_t net_idx) -> GSRRerouteTask* {
    if (!RTUTIL.exist(net_idx_to_gsr_net_idx_map, net_idx)) {
      return nullptr;
    }
    GSRNet& gsr_net = gsr_net_list[net_idx_to_gsr_net_idx_map[net_idx]];
    GSRRerouteTask& task = task_map[net_idx];
    task.gsr_net = &gsr_net;
    task.routed_times = gsr_net.get_routed_times();
    return &task;
  };

  auto addNetSetScore = [&](const std::set<int32_t>& net_set, const double overflow_score, const double risk_score,
                            const double near_full_score, const double hotspot_score, const bool is_overflow_cell,
                            const PlanarCoord& coord) {
    if (net_set.empty()) {
      return;
    }
    if (is_overflow_cell) {
      overflow_cell_key_set.insert(makePlanarKey(coord));
    }
    for (int32_t net_idx : net_set) {
      GSRRerouteTask* task = getTask(net_idx);
      if (task == nullptr) {
        continue;
      }
      if (is_overflow_cell) {
        overflow_net_idx_set.insert(net_idx);
        task->overflow_touch_num++;
        task->overflow_coord_set.insert(coord);
      }
      if (hotspot_score > gsr_com_param.get_congestion_risk_threshold()) {
        task->hotspot_touch_num++;
      }
      task->overflow_score += overflow_score;
      task->risk_score += risk_score;
      task->near_full_score += near_full_score;
      task->hotspot_score += hotspot_score;
    }
  };

  auto scanCoord = [&](const int32_t layer_idx, const PlanarCoord& coord) {
    if (layer_idx < 0 || static_cast<int32_t>(layer_node_map.size()) <= layer_idx) {
      return;
    }
    GridMap<GSRNode>& gsr_node_map = layer_node_map[layer_idx];
    int32_t x = coord.get_x();
    int32_t y = coord.get_y();
    if (!gsr_node_map.isInside(x, y)) {
      return;
    }
    GSRNode& gsr_node = gsr_node_map[x][y];
    double h_overflow = congestion_view.h_overflow_map.isInside(x, y) ? congestion_view.h_overflow_map[x][y] : 0;
    double v_overflow = congestion_view.v_overflow_map.isInside(x, y) ? congestion_view.v_overflow_map[x][y] : 0;
    double h_risk = congestion_view.h_risk_map.isInside(x, y) ? congestion_view.h_risk_map[x][y] : 0;
    double v_risk = congestion_view.v_risk_map.isInside(x, y) ? congestion_view.v_risk_map[x][y] : 0;
    double h_near_full = congestion_view.h_near_full_map.isInside(x, y) ? congestion_view.h_near_full_map[x][y] : 0;
    double v_near_full = congestion_view.v_near_full_map.isInside(x, y) ? congestion_view.v_near_full_map[x][y] : 0;
    double hotspot = congestion_view.hotspot_map.isInside(x, y) ? congestion_view.hotspot_map[x][y] : 0;

    for (Orientation orient : {Orientation::kEast, Orientation::kWest}) {
      if (!RTUTIL.exist(gsr_node.get_orient_net_map(), orient)) {
        continue;
      }
      double demand = gsr_node.getBoundaryDemand(orient);
      double supply = gsr_node.getSupply(orient);
      double orient_overflow = std::max(0.0, demand - supply);
      bool is_overflow_cell = orient_overflow > 0;
      addNetSetScore(gsr_node.get_orient_net_map()[orient], orient_overflow + h_overflow, h_risk, h_near_full, hotspot, is_overflow_cell, coord);
    }
    for (Orientation orient : {Orientation::kNorth, Orientation::kSouth}) {
      if (!RTUTIL.exist(gsr_node.get_orient_net_map(), orient)) {
        continue;
      }
      double demand = gsr_node.getBoundaryDemand(orient);
      double supply = gsr_node.getSupply(orient);
      double orient_overflow = std::max(0.0, demand - supply);
      bool is_overflow_cell = orient_overflow > 0;
      addNetSetScore(gsr_node.get_orient_net_map()[orient], orient_overflow + v_overflow, v_risk, v_near_full, hotspot, is_overflow_cell, coord);
    }
    double internal_overflow = congestion_view.internal_overflow_map.isInside(x, y) ? congestion_view.internal_overflow_map[x][y] : 0;
    if (internal_overflow > 0 || hotspot > gsr_com_param.get_congestion_risk_threshold()) {
      std::set<int32_t> net_set;
      for (auto& [net_idx, orient_set] : gsr_node.get_net_orient_map()) {
        (void) orient_set;
        net_set.insert(net_idx);
      }
      addNetSetScore(net_set, internal_overflow, h_risk + v_risk, h_near_full + v_near_full, hotspot, internal_overflow > 0, coord);
    }
  };

  auto scanAllCoords = [&]() {
    for (int32_t layer_idx = gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
      GridMap<GSRNode>& gsr_node_map = layer_node_map[layer_idx];
      for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
        for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
          scanCoord(layer_idx, PlanarCoord(x, y));
        }
      }
    }
  };

  if (!congestion_view.task_coord_list.empty()) {
    for (const PlanarCoord& coord : congestion_view.task_coord_list) {
      for (int32_t layer_idx = gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
        scanCoord(layer_idx, coord);
      }
    }
  } else if (gsr_model.get_total_overflow() > 0) {
    scanAllCoords();
  }
  if (congestion_view.overflow_cell_num > 0
      && static_cast<int32_t>(overflow_cell_key_set.size()) < congestion_view.overflow_cell_num) {
    task_map.clear();
    overflow_net_idx_set.clear();
    overflow_cell_key_set.clear();
    scanAllCoords();
  }

  route_stats.overflow_net_num = std::max(route_stats.overflow_net_num, static_cast<int32_t>(overflow_net_idx_set.size()));

  std::vector<GSRRerouteTask> task_list;
  task_list.reserve(task_map.size());
  for (auto& [net_idx, task] : task_map) {
    if (task.gsr_net == nullptr) {
      continue;
    }
    if (task.gsr_net->get_routed_times() >= gsr_com_param.get_max_routed_times()) {
      route_stats.skipped_max_routed_times_num++;
      continue;
    }
    double hpwl_score = static_cast<double>(std::min<int64_t>(getGSRNetHPWL(*task.gsr_net), 1000));
    double unrouted_bonus = prefer_uncovered ? static_cast<double>(std::max(0, gsr_com_param.get_max_routed_times() - task.routed_times)) : 0;
    task.total_score = 1000.0 * task.overflow_score + 100.0 * task.hotspot_score + 20.0 * task.near_full_score + 10.0 * task.risk_score
                       + 0.01 * hpwl_score + 100.0 * unrouted_bonus;
    task_list.push_back(task);
  }

  std::sort(task_list.begin(), task_list.end(), [](const GSRRerouteTask& a, const GSRRerouteTask& b) {
    if (!RTUTIL.equalDoubleByError(a.total_score, b.total_score, RT_ERROR)) {
      return a.total_score > b.total_score;
    }
    if (a.overflow_touch_num != b.overflow_touch_num) {
      return a.overflow_touch_num > b.overflow_touch_num;
    }
    if (a.routed_times != b.routed_times) {
      return a.routed_times < b.routed_times;
    }
    return a.gsr_net->get_net_idx() < b.gsr_net->get_net_idx();
  });

  int32_t min_task_num = std::max(1, gsr_com_param.get_min_reroute_task_num());
  int32_t max_task_num = std::max(min_task_num, gsr_com_param.get_max_reroute_task_num());
  double growth = std::max(1.0, gsr_com_param.get_reroute_task_growth_ratio());
  int32_t dynamic_task_num = std::min(max_task_num, static_cast<int32_t>(std::ceil(static_cast<double>(min_task_num) * std::pow(growth, iter))));
  if (stage3) {
    dynamic_task_num = std::min(max_task_num, dynamic_task_num * 2);
  }
  dynamic_task_num = std::min(dynamic_task_num, static_cast<int32_t>(task_list.size()));
  int32_t selection_limit = dynamic_task_num;

  std::vector<GSRNet*> reroute_task_list;
  reroute_task_list.reserve(std::min(max_task_num, static_cast<int32_t>(task_list.size())));
  std::set<int32_t> selected_net_idx_set;
  std::set<PlanarKey> covered_overflow_cell_key_set;
  auto selectTask = [&](const GSRRerouteTask& task) {
    if (task.gsr_net == nullptr || static_cast<int32_t>(reroute_task_list.size()) >= selection_limit) {
      return;
    }
    int32_t net_idx = task.gsr_net->get_net_idx();
    if (!selected_net_idx_set.insert(net_idx).second) {
      return;
    }
    for (const PlanarCoord& coord : task.overflow_coord_set) {
      covered_overflow_cell_key_set.insert(makePlanarKey(coord));
    }
    reroute_task_list.push_back(task.gsr_net);
  };
  for (const GSRRerouteTask& task : task_list) {
    selectTask(task);
  }

  double target_coverage = std::min(1.0, std::max(0.0, gsr_com_param.get_reroute_coverage_target()));
  double coverage_ratio = overflow_cell_key_set.empty() ? 1.0
                                                        : static_cast<double>(covered_overflow_cell_key_set.size())
                                                              / static_cast<double>(overflow_cell_key_set.size());
  if (coverage_ratio + RT_ERROR < target_coverage && selection_limit < max_task_num) {
    selection_limit = std::min(max_task_num, static_cast<int32_t>(task_list.size()));
    for (const GSRRerouteTask& task : task_list) {
      if (coverage_ratio + RT_ERROR >= target_coverage || static_cast<int32_t>(reroute_task_list.size()) >= selection_limit) {
        break;
      }
      selectTask(task);
      coverage_ratio = overflow_cell_key_set.empty() ? 1.0
                                                     : static_cast<double>(covered_overflow_cell_key_set.size())
                                                           / static_cast<double>(overflow_cell_key_set.size());
    }
  }

  int32_t covered_cell_num = static_cast<int32_t>(covered_overflow_cell_key_set.size());
  coverage_ratio = overflow_cell_key_set.empty() ? 1.0 : static_cast<double>(covered_cell_num) / static_cast<double>(overflow_cell_key_set.size());
  route_stats.selected_overflow_net_num = std::max(route_stats.selected_overflow_net_num, static_cast<int32_t>(selected_net_idx_set.size()));
  route_stats.selected_hotspot_num = std::max(route_stats.selected_hotspot_num, congestion_view.hotspot_cell_num);
  if (stage3) {
    route_stats.stage3_coverage_cell_num = std::max(route_stats.stage3_coverage_cell_num, covered_cell_num);
    route_stats.stage3_coverage_ratio = std::max(route_stats.stage3_coverage_ratio, coverage_ratio);
  } else {
    route_stats.stage2_coverage_cell_num = std::max(route_stats.stage2_coverage_cell_num, covered_cell_num);
    route_stats.stage2_coverage_ratio = std::max(route_stats.stage2_coverage_ratio, coverage_ratio);
  }
  outputRerouteTaskCSV(task_list, selected_net_idx_set, iter, stage3 ? 3 : 2);
  return reroute_task_list;
}

bool GlobalSpatialRouter::tryDetourNet(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView& congestion_view, GSRRouteStats& route_stats,
                      GSRRerouteAttemptRecord* attempt_record)
{
  auto attempt_start_time = std::chrono::steady_clock::now();
  GSRRouteSnapshot route_snapshot = snapshotRoute(gsr_model, gsr_net);
  initRerouteAttemptRecord(gsr_net, route_snapshot, attempt_record);
  removeSnapshotRoute(gsr_model, gsr_net, route_snapshot, nullptr, nullptr);

  route_stats.stage2_detour_route_num++;
  std::vector<Segment<LayerCoord>> pattern_segment_list = routeByPattern(gsr_model, gsr_net, &congestion_view, true, route_stats);
  if (tryCommitCandidateRoute(gsr_model, gsr_net, route_snapshot, pattern_segment_list, route_stats, attempt_record, 2, "empty_route",
                              attempt_start_time)) {
    route_stats.stage2_detour_accept_num++;
    return true;
  }

  restoreSnapshotRoute(gsr_model, gsr_net, route_snapshot, nullptr, nullptr);
  if (attempt_record != nullptr) {
    attempt_record->runtime_ms = getElapsedMs(attempt_start_time);
  }
  return false;
}

bool GlobalSpatialRouter::trySparseMazeNet(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView& congestion_view, GSRWireCostView& wire_cost_view,
                          GSRRouteStats& route_stats, GSRRerouteAttemptRecord* attempt_record)
{
  auto attempt_start_time = std::chrono::steady_clock::now();
  GSRRouteSnapshot route_snapshot = snapshotRoute(gsr_model, gsr_net);
  initRerouteAttemptRecord(gsr_net, route_snapshot, attempt_record);
  removeSnapshotRoute(gsr_model, gsr_net, route_snapshot, &wire_cost_view, &route_stats);

  route_stats.maze_route_num++;
  route_stats.stage3_sparse_maze_route_num++;
  auto maze_start_time = std::chrono::steady_clock::now();
  std::vector<Segment<LayerCoord>> maze_segment_list = routeByMaze(gsr_model, gsr_net, congestion_view, wire_cost_view, route_stats);
  appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, 3, "route_by_maze", getElapsedMs(maze_start_time), 1);
  if (tryCommitCandidateRoute(gsr_model, gsr_net, route_snapshot, maze_segment_list, route_stats, attempt_record, 3, "maze_fail",
                              attempt_start_time, &wire_cost_view)) {
    route_stats.stage3_sparse_maze_success_num++;
    return true;
  }
  if (maze_segment_list.empty()) {
    route_stats.maze_fail_num++;
    route_stats.reroute_reject_num++;
  }

  restoreSnapshotRoute(gsr_model, gsr_net, route_snapshot, &wire_cost_view, &route_stats);
  if (attempt_record != nullptr) {
    attempt_record->runtime_ms = getElapsedMs(attempt_start_time);
  }
  return false;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::routeByPattern(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView* congestion_view, const bool enable_detour,
                                                    GSRRouteStats& route_stats)
{
  std::vector<Segment<PlanarCoord>> planar_topo_list = getPlanarTopoList(gsr_net, route_stats);
  GSRTree gsr_tree = buildGSRTree(gsr_model, gsr_net, planar_topo_list, route_stats);
  std::vector<Segment<LayerCoord>> raw_routing_segment_list
      = refineTreeByPatternLayerDP(gsr_model, gsr_net, gsr_tree, congestion_view, enable_detour, route_stats);
  std::vector<Segment<LayerCoord>> valid_segment_list = getValidUniqueSegmentList(gsr_model, raw_routing_segment_list, route_stats);
  if (!isRouteConnected(gsr_net, valid_segment_list)) {
    return {};
  }
  gsr_tree.set_segment_list(valid_segment_list);
  gsr_net.set_routing_tree(gsr_tree);
  return valid_segment_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::routeByMaze(GSRModel& gsr_model, GSRNet& gsr_net, GSRCongestionView& congestion_view,
                                                 GSRWireCostView& wire_cost_view, GSRRouteStats& route_stats)
{
  std::vector<PlanarCoord> terminal_coord_list;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> terminal_coord_set;
  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    LayerCoord access_coord = gsr_pin.get_access_coord();
    if (gsr_model.get_gsr_grid_graph().isInside(access_coord) && terminal_coord_set.insert(access_coord.get_planar_coord()).second) {
      terminal_coord_list.push_back(access_coord.get_planar_coord());
    }
  }
  if (terminal_coord_list.size() < 2) {
    return {};
  }

  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  std::vector<int32_t> preferred_h_layer_list = getCandidateLayerList(gsr_com_param, true);
  std::vector<int32_t> preferred_v_layer_list = getCandidateLayerList(gsr_com_param, false);
  int32_t base_ll_x = std::numeric_limits<int32_t>::max();
  int32_t base_ll_y = std::numeric_limits<int32_t>::max();
  int32_t base_ur_x = std::numeric_limits<int32_t>::min();
  int32_t base_ur_y = std::numeric_limits<int32_t>::min();
  auto updateBBox = [&](const PlanarCoord& coord) {
    base_ll_x = std::min(base_ll_x, coord.get_x());
    base_ll_y = std::min(base_ll_y, coord.get_y());
    base_ur_x = std::max(base_ur_x, coord.get_x());
    base_ur_y = std::max(base_ur_y, coord.get_y());
  };
  for (const PlanarCoord& terminal_coord : terminal_coord_list) {
    updateBBox(terminal_coord);
  }
  for (const Segment<LayerCoord>& segment : gsr_net.get_routing_segment_list()) {
    updateBBox(segment.get_first().get_planar_coord());
    updateBBox(segment.get_second().get_planar_coord());
  }

  auto getSparseEstimateCost = [&](const PlanarCoord& start_coord,
                                   const std::set<PlanarCoord, CmpPlanarCoordByXASC>& remaining_coord_set) {
    double estimate_cost = std::numeric_limits<double>::max();
    for (const PlanarCoord& end_coord : remaining_coord_set) {
      estimate_cost = std::min(estimate_cost,
                               gsr_com_param.get_unit_wire_cost() * static_cast<double>(RTUTIL.getManhattanDistance(start_coord, end_coord)));
    }
    return estimate_cost == std::numeric_limits<double>::max() ? 0 : estimate_cost;
  };

  for (int32_t expand_times = 0; expand_times <= gsr_com_param.get_maze_window_max_expand_times(); expand_times++) {
    int32_t window_size = gsr_com_param.get_maze_window_size() * (expand_times + 1);
    int32_t ll_x = std::max(0, base_ll_x - window_size);
    int32_t ll_y = std::max(0, base_ll_y - window_size);
    int32_t ur_x = std::min(gsr_grid_graph.get_x_size() - 1, base_ur_x + window_size);
    int32_t ur_y = std::min(gsr_grid_graph.get_y_size() - 1, base_ur_y + window_size);

    std::vector<int32_t> sparse_x_list;
    std::vector<int32_t> sparse_y_list;
    std::vector<uint8_t> x_used(gsr_grid_graph.get_x_size(), 0);
    std::vector<uint8_t> y_used(gsr_grid_graph.get_y_size(), 0);
    auto addSparseX = [&](const int32_t raw_x) {
      int32_t x = clampInt(raw_x, ll_x, ur_x);
      if (x < 0 || static_cast<int32_t>(x_used.size()) <= x || x_used[x] != 0) {
        return false;
      }
      x_used[x] = 1;
      sparse_x_list.push_back(x);
      return true;
    };
    auto addSparseY = [&](const int32_t raw_y) {
      int32_t y = clampInt(raw_y, ll_y, ur_y);
      if (y < 0 || static_cast<int32_t>(y_used.size()) <= y || y_used[y] != 0) {
        return false;
      }
      y_used[y] = 1;
      sparse_y_list.push_back(y);
      return true;
    };
    auto addSparseCoord = [&](const PlanarCoord& coord) {
      addSparseX(coord.get_x());
      addSparseY(coord.get_y());
    };
    for (const PlanarCoord& terminal_coord : terminal_coord_list) {
      addSparseCoord(terminal_coord);
    }
    for (const Segment<LayerCoord>& segment : gsr_net.get_routing_segment_list()) {
      addSparseCoord(segment.get_first().get_planar_coord());
      addSparseCoord(segment.get_second().get_planar_coord());
    }
    int32_t sparse_grid_interval = std::max(1, gsr_com_param.get_sparse_grid_interval());
    for (int32_t x = ll_x; x <= ur_x; x += sparse_grid_interval) {
      addSparseX(x);
    }
    for (int32_t y = ll_y; y <= ur_y; y += sparse_grid_interval) {
      addSparseY(y);
    }
    int32_t sparse_x_offset = sparse_grid_interval == 1 ? 0 : std::abs(gsr_net.get_net_idx() + expand_times) % sparse_grid_interval;
    int32_t sparse_y_offset = sparse_grid_interval == 1 ? 0 : std::abs(gsr_net.get_net_idx() * 3 + expand_times) % sparse_grid_interval;
    for (int32_t x = ll_x + sparse_x_offset; x <= ur_x; x += sparse_grid_interval) {
      if (addSparseX(x)) {
        route_stats.sparse_offset_line_num++;
      }
    }
    for (int32_t y = ll_y + sparse_y_offset; y <= ur_y; y += sparse_grid_interval) {
      if (addSparseY(y)) {
        route_stats.sparse_offset_line_num++;
      }
    }
    int32_t hotspot_line_cap = std::max(8, gsr_com_param.get_target_detour_count() * 4);
    int32_t hotspot_line_num = 0;
    int32_t hotspot_sample_step = std::max(1, sparse_grid_interval / 2);
    auto addDetourX = [&](const int32_t base_x, const int32_t delta) {
      if (addSparseX(base_x + delta * sparse_grid_interval)) {
        route_stats.sparse_hotspot_line_num++;
        hotspot_line_num++;
      }
    };
    auto addDetourY = [&](const int32_t base_y, const int32_t delta) {
      if (addSparseY(base_y + delta * sparse_grid_interval)) {
        route_stats.sparse_hotspot_line_num++;
        hotspot_line_num++;
      }
    };
    for (int32_t x = ll_x; x <= ur_x && hotspot_line_num < hotspot_line_cap; x += hotspot_sample_step) {
      for (int32_t y = ll_y; y <= ur_y && hotspot_line_num < hotspot_line_cap; y += hotspot_sample_step) {
        double h_overflow = congestion_view.h_overflow_map.isInside(x, y) ? congestion_view.h_overflow_map[x][y] : 0;
        double v_overflow = congestion_view.v_overflow_map.isInside(x, y) ? congestion_view.v_overflow_map[x][y] : 0;
        double capacity_pressure = congestion_view.capacity_pressure_map.isInside(x, y) ? congestion_view.capacity_pressure_map[x][y] : 0;
        double capacity_block = congestion_view.capacity_block_map.isInside(x, y) ? congestion_view.capacity_block_map[x][y] : 0;
        double hotspot = congestion_view.hotspot_map.isInside(x, y) ? congestion_view.hotspot_map[x][y] : 0;
        bool has_boundary_overflow = h_overflow > gsr_com_param.get_detour_congestion_threshold()
                                     || v_overflow > gsr_com_param.get_detour_congestion_threshold();
        bool has_capacity_hotspot = capacity_pressure > gsr_com_param.get_detour_congestion_threshold() || capacity_block > 0
                                    || hotspot > gsr_com_param.get_congestion_risk_threshold();
        if (!has_boundary_overflow && !has_capacity_hotspot) {
          continue;
        }
        if (h_overflow > 0) {
          for (int32_t delta : {-1, 1}) {
            addDetourY(y, delta);
          }
        }
        if (v_overflow > 0) {
          for (int32_t delta : {-1, 1}) {
            addDetourX(x, delta);
          }
        }
        if (has_capacity_hotspot) {
          for (int32_t delta : {-1, 1}) {
            addDetourX(x, delta);
            addDetourY(y, delta);
          }
        }
      }
    }
    addSparseX(ll_x);
    addSparseX(ur_x);
    addSparseY(ll_y);
    addSparseY(ur_y);

    std::sort(sparse_x_list.begin(), sparse_x_list.end());
    std::sort(sparse_y_list.begin(), sparse_y_list.end());
    std::vector<int32_t> x_idx_map(gsr_grid_graph.get_x_size(), -1);
    std::vector<int32_t> y_idx_map(gsr_grid_graph.get_y_size(), -1);
    for (int32_t i = 0; i < static_cast<int32_t>(sparse_x_list.size()); i++) {
      x_idx_map[sparse_x_list[i]] = i;
    }
    for (int32_t i = 0; i < static_cast<int32_t>(sparse_y_list.size()); i++) {
      y_idx_map[sparse_y_list[i]] = i;
    }

    int32_t x_num = static_cast<int32_t>(sparse_x_list.size());
    int32_t y_num = static_cast<int32_t>(sparse_y_list.size());
    std::vector<GSRSparseMazeNode> node_list;
    node_list.resize(static_cast<size_t>(x_num) * y_num * 2);
    auto getNodeIdxByIndex = [&](const int32_t x_idx, const int32_t y_idx, const Direction direction) {
      if (x_idx < 0 || x_num <= x_idx || y_idx < 0 || y_num <= y_idx) {
        return -1;
      }
      return ((x_idx * y_num + y_idx) << 1) + (direction == Direction::kVertical ? 1 : 0);
    };
    auto initNode = [&](const int32_t x_idx, const int32_t y_idx, const Direction direction) {
      int32_t node_idx = getNodeIdxByIndex(x_idx, y_idx, direction);
      GSRSparseMazeNode& sparse_node = node_list[node_idx];
      sparse_node.coord = PlanarCoord(sparse_x_list[x_idx], sparse_y_list[y_idx]);
      sparse_node.direction = direction;
      sparse_node.known_cost = std::numeric_limits<double>::max();
      sparse_node.estimated_cost = 0;
      sparse_node.parent_idx = -1;
    };
    for (int32_t x_idx = 0; x_idx < x_num; x_idx++) {
      for (int32_t y_idx = 0; y_idx < y_num; y_idx++) {
        initNode(x_idx, y_idx, Direction::kHorizontal);
        initNode(x_idx, y_idx, Direction::kVertical);
      }
    }

    auto getNodeIdx = [&](const PlanarCoord& coord, const Direction direction) {
      if (coord.get_x() < 0 || gsr_grid_graph.get_x_size() <= coord.get_x() || coord.get_y() < 0 || gsr_grid_graph.get_y_size() <= coord.get_y()) {
        return -1;
      }
      int32_t x_idx = x_idx_map[coord.get_x()];
      int32_t y_idx = y_idx_map[coord.get_y()];
      return getNodeIdxByIndex(x_idx, y_idx, direction);
    };

    auto getSparseEdgeCost = [&](const GSRSparseMazeNode& curr_node, const GSRSparseMazeNode& neighbor_node) {
      if (curr_node.coord == neighbor_node.coord) {
        double cost = gsr_grid_graph.getBestProjectedSwitchCost(gsr_net.get_net_idx(), curr_node.coord, preferred_h_layer_list, preferred_v_layer_list,
                                                               gsr_com_param, gsr_com_param.get_maze_logistic_slope());
        cost += gsr_com_param.get_unit_short_cost() * kCapacityPressureWeight * getCapacityPressure(congestion_view, curr_node.coord);
        cost += kCapacityBlockPenalty * getCapacityBlock(congestion_view, curr_node.coord);
        return cost;
      }
      Direction direction = curr_node.direction;
      double cost = getWireCost(wire_cost_view, direction, curr_node.coord, neighbor_node.coord);
      double risk_cost = getCongestionRiskSum(congestion_view, direction, curr_node.coord, neighbor_node.coord);
      cost += gsr_com_param.get_unit_short_cost() * risk_cost;
      double capacity_pressure = getCapacityPressureSum(congestion_view, direction, curr_node.coord, neighbor_node.coord);
      double capacity_block = getCapacityBlockSum(congestion_view, direction, curr_node.coord, neighbor_node.coord);
      cost += gsr_com_param.get_unit_short_cost() * kCapacityPressureWeight * capacity_pressure;
      cost += kCapacityBlockPenalty * capacity_block;
      return cost;
    };

    auto appendSparsePathSegmentList = [&](const std::vector<PlanarCoord>& path_coord_list,
                                           std::vector<Segment<LayerCoord>>& routing_segment_list) {
      if (path_coord_list.size() < 2) {
        return;
      }
      for (size_t i = 1; i < path_coord_list.size(); i++) {
        PlanarCoord first_coord = path_coord_list[i - 1];
        PlanarCoord second_coord = path_coord_list[i];
        Direction direction = RTUTIL.getDirection(first_coord, second_coord);
        if (direction == Direction::kHorizontal) {
          routing_segment_list.emplace_back(makeLayerCoord(first_coord, gsr_com_param.get_horizontal_layer_idx()),
                                            makeLayerCoord(second_coord, gsr_com_param.get_horizontal_layer_idx()));
        } else if (direction == Direction::kVertical) {
          routing_segment_list.emplace_back(makeLayerCoord(first_coord, gsr_com_param.get_vertical_layer_idx()),
                                            makeLayerCoord(second_coord, gsr_com_param.get_vertical_layer_idx()));
        }
      }
    };

    std::vector<Segment<LayerCoord>> maze_skeleton_segment_list;
    std::set<PlanarCoord, CmpPlanarCoordByXASC> remaining_coord_set;
    for (size_t i = 1; i < terminal_coord_list.size(); i++) {
      remaining_coord_set.insert(terminal_coord_list[i]);
    }

    std::priority_queue<GSRMazeHeapItem, std::vector<GSRMazeHeapItem>, CmpGSRMazeHeapItem> open_queue;
    auto pushNode = [&](const int32_t node_idx, const double known_cost, const int32_t parent_idx) {
      GSRSparseMazeNode& sparse_node = node_list[node_idx];
      sparse_node.known_cost = known_cost;
      sparse_node.estimated_cost = getSparseEstimateCost(sparse_node.coord, remaining_coord_set);
      sparse_node.parent_idx = parent_idx;
      open_queue.push(GSRMazeHeapItem{node_idx, known_cost, known_cost + sparse_node.estimated_cost});
    };
    int32_t source_h_idx = getNodeIdx(terminal_coord_list.front(), Direction::kHorizontal);
    int32_t source_v_idx = getNodeIdx(terminal_coord_list.front(), Direction::kVertical);
    if (source_h_idx != -1) {
      pushNode(source_h_idx, 0, -1);
    }
    if (source_v_idx != -1) {
      pushNode(source_v_idx, 0, -1);
    }

    bool failed = false;
    while (!remaining_coord_set.empty()) {
      int32_t end_node_idx = -1;
      while (!open_queue.empty()) {
        GSRMazeHeapItem heap_item = open_queue.top();
        open_queue.pop();
        if (heap_item.node_idx < 0 || static_cast<int32_t>(node_list.size()) <= heap_item.node_idx) {
          continue;
        }
        GSRSparseMazeNode& curr_node = node_list[heap_item.node_idx];
        if (!RTUTIL.equalDoubleByError(heap_item.known_cost, curr_node.known_cost, RT_ERROR)) {
          continue;
        }
        int32_t curr_node_idx = heap_item.node_idx;
        if (remaining_coord_set.find(curr_node.coord) != remaining_coord_set.end()) {
          end_node_idx = curr_node_idx;
          break;
        }
        int32_t curr_x_idx = x_idx_map[curr_node.coord.get_x()];
        int32_t curr_y_idx = y_idx_map[curr_node.coord.get_y()];
        auto relaxNode = [&](const int32_t neighbor_idx) {
          if (neighbor_idx == -1) {
            return;
          }
          GSRSparseMazeNode& neighbor_node = node_list[neighbor_idx];
          double known_cost = curr_node.known_cost + getSparseEdgeCost(curr_node, neighbor_node);
          if (known_cost + RT_ERROR < neighbor_node.known_cost) {
            pushNode(neighbor_idx, known_cost, curr_node_idx);
          }
        };
        if (curr_node.direction == Direction::kHorizontal) {
          if (curr_x_idx > 0) {
            relaxNode(getNodeIdxByIndex(curr_x_idx - 1, curr_y_idx, Direction::kHorizontal));
          }
          if (curr_x_idx + 1 < x_num) {
            relaxNode(getNodeIdxByIndex(curr_x_idx + 1, curr_y_idx, Direction::kHorizontal));
          }
          relaxNode(getNodeIdxByIndex(curr_x_idx, curr_y_idx, Direction::kVertical));
        } else {
          if (curr_y_idx > 0) {
            relaxNode(getNodeIdxByIndex(curr_x_idx, curr_y_idx - 1, Direction::kVertical));
          }
          if (curr_y_idx + 1 < y_num) {
            relaxNode(getNodeIdxByIndex(curr_x_idx, curr_y_idx + 1, Direction::kVertical));
          }
          relaxNode(getNodeIdxByIndex(curr_x_idx, curr_y_idx, Direction::kHorizontal));
        }
      }
      if (end_node_idx == -1) {
        failed = true;
        break;
      }

      std::vector<int32_t> path_node_idx_list;
      std::vector<PlanarCoord> path_coord_list;
      for (int32_t path_node_idx = end_node_idx; path_node_idx != -1; path_node_idx = node_list[path_node_idx].parent_idx) {
        path_node_idx_list.push_back(path_node_idx);
        appendUniqueCoord(path_coord_list, node_list[path_node_idx].coord);
      }
      std::reverse(path_node_idx_list.begin(), path_node_idx_list.end());
      std::reverse(path_coord_list.begin(), path_coord_list.end());
      appendSparsePathSegmentList(path_coord_list, maze_skeleton_segment_list);
      remaining_coord_set.erase(node_list[end_node_idx].coord);

      for (int32_t path_node_idx : path_node_idx_list) {
        for (Direction direction : {Direction::kHorizontal, Direction::kVertical}) {
          int32_t tree_node_idx = getNodeIdx(node_list[path_node_idx].coord, direction);
          if (tree_node_idx != -1) {
            pushNode(tree_node_idx, 0, -1);
          }
        }
      }
    }
    if (failed || maze_skeleton_segment_list.empty()) {
      continue;
    }

    GSRTree maze_tree = buildGSRTreeFromSegmentList(gsr_model, gsr_net, maze_skeleton_segment_list, route_stats);
    std::vector<Segment<LayerCoord>> refined_segment_list = refineTreeByPatternLayerDP(gsr_model, gsr_net, maze_tree, &congestion_view, true, route_stats);
    refined_segment_list = getValidUniqueSegmentList(gsr_model, refined_segment_list, route_stats);
    if (!refined_segment_list.empty() && isRoutePreferredOnly(gsr_model, refined_segment_list) && isRouteConnected(gsr_net, refined_segment_list)) {
      maze_tree.set_segment_list(refined_segment_list);
      gsr_net.set_routing_tree(maze_tree);
      return refined_segment_list;
    }
  }
  return {};
}

// pattern, maze, and tree refinement

std::vector<Segment<PlanarCoord>> GlobalSpatialRouter::getPlanarTopoList(GSRNet& gsr_net, GSRRouteStats& route_stats)
{
  std::vector<PlanarCoord> planar_coord_list;
  std::set<PlanarCoord, CmpPlanarCoordByXASC> visited_planar_coord_set;
  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    PlanarCoord planar_coord = gsr_pin.get_access_coord().get_planar_coord();
    if (visited_planar_coord_set.insert(planar_coord).second) {
      planar_coord_list.push_back(planar_coord);
    }
  }
  if (planar_coord_list.size() < 2) {
    return {};
  }

  std::vector<Segment<PlanarCoord>> planar_topo_list = RTI.getPlanarTopoList(planar_coord_list);
  if (!planar_topo_list.empty()) {
    return planar_topo_list;
  }

  route_stats.fallback_topology_net_num++;
  for (size_t i = 1; i < planar_coord_list.size(); i++) {
    if (planar_coord_list.front() != planar_coord_list[i]) {
      planar_topo_list.emplace_back(planar_coord_list.front(), planar_coord_list[i]);
    }
  }
  return planar_topo_list;
}

GSRTree GlobalSpatialRouter::buildGSRTree(GSRModel& gsr_model, GSRNet& gsr_net, std::vector<Segment<PlanarCoord>>& planar_topo_list, GSRRouteStats& route_stats)
{
  (void) gsr_model;

  GSRTree gsr_tree;
  std::vector<GSRTreeNode> node_list;
  std::map<PlanarKey, int32_t> coord_node_idx_map;

  auto getNodeIdx = [&](const PlanarCoord& coord) {
    PlanarKey key = makePlanarKey(coord);
    if (RTUTIL.exist(coord_node_idx_map, key)) {
      return coord_node_idx_map[key];
    }
    int32_t node_idx = static_cast<int32_t>(node_list.size());
    coord_node_idx_map[key] = node_idx;
    node_list.emplace_back(coord);
    return node_idx;
  };

  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    LayerCoord access_coord = gsr_pin.get_access_coord();
    int32_t node_idx = getNodeIdx(access_coord.get_planar_coord());
    GSRTreeNode& node = node_list[node_idx];
    if (node.fixed_layer_interval.first == -1) {
      node.fixed_layer_interval = {access_coord.get_layer_idx(), access_coord.get_layer_idx()};
    } else {
      node.fixed_layer_interval.first = std::min(node.fixed_layer_interval.first, access_coord.get_layer_idx());
      node.fixed_layer_interval.second = std::max(node.fixed_layer_interval.second, access_coord.get_layer_idx());
    }
  }
  for (Segment<PlanarCoord>& planar_topo : planar_topo_list) {
    int32_t first_idx = getNodeIdx(planar_topo.get_first());
    int32_t second_idx = getNodeIdx(planar_topo.get_second());
    if (first_idx != second_idx) {
      node_list[first_idx].child_idx_list.push_back(second_idx);
    }
  }

  gsr_tree.set_root_idx(node_list.empty() ? -1 : 0);
  gsr_tree.set_node_list(node_list);
  return canonicalizeGSRTree(gsr_model, gsr_net, gsr_tree, route_stats);
}

GSRTree GlobalSpatialRouter::buildGSRTreeFromSegmentList(GSRModel& gsr_model, GSRNet& gsr_net, std::vector<Segment<LayerCoord>>& segment_list,
                                      GSRRouteStats& route_stats)
{
  (void) gsr_model;

  GSRTree gsr_tree;
  std::vector<GSRTreeNode> node_list;
  std::map<PlanarKey, int32_t> coord_node_idx_map;

  auto getNodeIdx = [&](const PlanarCoord& coord) {
    PlanarKey key = makePlanarKey(coord);
    if (RTUTIL.exist(coord_node_idx_map, key)) {
      return coord_node_idx_map[key];
    }
    int32_t node_idx = static_cast<int32_t>(node_list.size());
    coord_node_idx_map[key] = node_idx;
    node_list.emplace_back(coord);
    return node_idx;
  };

  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    LayerCoord access_coord = gsr_pin.get_access_coord();
    int32_t node_idx = getNodeIdx(access_coord.get_planar_coord());
    GSRTreeNode& node = node_list[node_idx];
    if (node.fixed_layer_interval.first == -1) {
      node.fixed_layer_interval = {access_coord.get_layer_idx(), access_coord.get_layer_idx()};
    } else {
      node.fixed_layer_interval.first = std::min(node.fixed_layer_interval.first, access_coord.get_layer_idx());
      node.fixed_layer_interval.second = std::max(node.fixed_layer_interval.second, access_coord.get_layer_idx());
    }
  }
  for (Segment<LayerCoord>& segment : segment_list) {
    int32_t first_idx = getNodeIdx(segment.get_first().get_planar_coord());
    int32_t second_idx = getNodeIdx(segment.get_second().get_planar_coord());
    if (first_idx != second_idx) {
      node_list[first_idx].child_idx_list.push_back(second_idx);
    }
  }

  gsr_tree.set_root_idx(node_list.empty() ? -1 : 0);
  gsr_tree.set_node_list(node_list);
  return canonicalizeGSRTree(gsr_model, gsr_net, gsr_tree, route_stats);
}

GSRTree GlobalSpatialRouter::canonicalizeGSRTree(GSRModel& gsr_model, GSRNet& gsr_net, GSRTree& gsr_tree, GSRRouteStats& route_stats)
{
  (void) gsr_model;

  std::vector<GSRTreeNode>& input_node_list = gsr_tree.get_node_list();
  if (input_node_list.empty()) {
    return gsr_tree;
  }

  route_stats.canonical_tree_num++;
  std::set<PlanarKey> terminal_key_set;
  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    terminal_key_set.insert(makePlanarKey(gsr_pin.get_access_coord().get_planar_coord()));
  }

  std::vector<GSRTreeNode> node_list;
  std::map<PlanarKey, int32_t> coord_node_idx_map;
  auto getNodeIdx = [&](const PlanarCoord& coord) {
    PlanarKey key = makePlanarKey(coord);
    if (RTUTIL.exist(coord_node_idx_map, key)) {
      return coord_node_idx_map[key];
    }
    int32_t node_idx = static_cast<int32_t>(node_list.size());
    coord_node_idx_map[key] = node_idx;
    node_list.emplace_back(coord);
    return node_idx;
  };

  std::vector<int32_t> old_to_new_idx_list(input_node_list.size(), -1);
  for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(input_node_list.size()); node_idx++) {
    GSRTreeNode& old_node = input_node_list[node_idx];
    int32_t new_idx = getNodeIdx(old_node.coord);
    old_to_new_idx_list[node_idx] = new_idx;
    if (old_node.fixed_layer_interval.first != -1) {
      GSRTreeNode& new_node = node_list[new_idx];
      if (new_node.fixed_layer_interval.first == -1) {
        new_node.fixed_layer_interval = old_node.fixed_layer_interval;
      } else {
        new_node.fixed_layer_interval.first = std::min(new_node.fixed_layer_interval.first, old_node.fixed_layer_interval.first);
        new_node.fixed_layer_interval.second = std::max(new_node.fixed_layer_interval.second, old_node.fixed_layer_interval.second);
      }
    }
  }

  std::vector<std::set<int32_t>> adjacency_list(node_list.size());
  std::set<UndirectedPlanarEdgeKey> edge_key_set;
  for (int32_t old_parent_idx = 0; old_parent_idx < static_cast<int32_t>(input_node_list.size()); old_parent_idx++) {
    int32_t parent_idx = old_to_new_idx_list[old_parent_idx];
    if (parent_idx == -1) {
      continue;
    }
    for (int32_t old_child_idx : input_node_list[old_parent_idx].child_idx_list) {
      if (old_child_idx < 0 || static_cast<int32_t>(input_node_list.size()) <= old_child_idx) {
        route_stats.canonical_removed_edge_num++;
        continue;
      }
      int32_t child_idx = old_to_new_idx_list[old_child_idx];
      if (child_idx == -1 || parent_idx == child_idx || node_list[parent_idx].coord == node_list[child_idx].coord) {
        route_stats.canonical_removed_edge_num++;
        continue;
      }
      UndirectedPlanarEdgeKey edge_key = makeUndirectedPlanarEdgeKey(node_list[parent_idx].coord, node_list[child_idx].coord);
      if (!edge_key_set.insert(edge_key).second) {
        route_stats.canonical_removed_edge_num++;
        continue;
      }
      adjacency_list[parent_idx].insert(child_idx);
      adjacency_list[child_idx].insert(parent_idx);
    }
  }

  int32_t root_idx = 0;
  if (0 <= gsr_tree.get_root_idx() && gsr_tree.get_root_idx() < static_cast<int32_t>(old_to_new_idx_list.size())
      && old_to_new_idx_list[gsr_tree.get_root_idx()] != -1) {
    root_idx = old_to_new_idx_list[gsr_tree.get_root_idx()];
  }

  std::vector<std::set<int32_t>> tree_adjacency_list(node_list.size());
  std::vector<bool> visited_node_list(node_list.size(), false);
  auto bfsTree = [&](const int32_t component_root_idx) {
    std::queue<int32_t> node_queue;
    visited_node_list[component_root_idx] = true;
    node_queue.push(component_root_idx);
    while (!node_queue.empty()) {
      int32_t curr_idx = node_queue.front();
      node_queue.pop();
      for (int32_t neighbor_idx : adjacency_list[curr_idx]) {
        if (visited_node_list[neighbor_idx]) {
          if (tree_adjacency_list[curr_idx].find(neighbor_idx) == tree_adjacency_list[curr_idx].end()
              && tree_adjacency_list[neighbor_idx].find(curr_idx) == tree_adjacency_list[neighbor_idx].end()) {
            route_stats.canonical_cycle_break_num++;
          }
          continue;
        }
        visited_node_list[neighbor_idx] = true;
        tree_adjacency_list[curr_idx].insert(neighbor_idx);
        tree_adjacency_list[neighbor_idx].insert(curr_idx);
        node_queue.push(neighbor_idx);
      }
    }
  };
  bfsTree(root_idx);
  for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(node_list.size()); node_idx++) {
    if (!visited_node_list[node_idx]) {
      bfsTree(node_idx);
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(node_list.size()); node_idx++) {
      if (node_idx == root_idx || node_list[node_idx].coord.get_x() == std::numeric_limits<int32_t>::max()) {
        continue;
      }
      if (node_list[node_idx].fixed_layer_interval.first != -1 || RTUTIL.exist(terminal_key_set, makePlanarKey(node_list[node_idx].coord))) {
        continue;
      }
      if (tree_adjacency_list[node_idx].size() != 2) {
        continue;
      }
      std::vector<int32_t> neighbor_list(tree_adjacency_list[node_idx].begin(), tree_adjacency_list[node_idx].end());
      int32_t first_neighbor_idx = neighbor_list[0];
      int32_t second_neighbor_idx = neighbor_list[1];
      PlanarCoord node_coord = node_list[node_idx].coord;
      PlanarCoord first_coord = node_list[first_neighbor_idx].coord;
      PlanarCoord second_coord = node_list[second_neighbor_idx].coord;
      bool collinear_x = first_coord.get_x() == node_coord.get_x() && node_coord.get_x() == second_coord.get_x();
      bool collinear_y = first_coord.get_y() == node_coord.get_y() && node_coord.get_y() == second_coord.get_y();
      if (!collinear_x && !collinear_y) {
        continue;
      }
      tree_adjacency_list[first_neighbor_idx].erase(node_idx);
      tree_adjacency_list[second_neighbor_idx].erase(node_idx);
      tree_adjacency_list[node_idx].clear();
      tree_adjacency_list[first_neighbor_idx].insert(second_neighbor_idx);
      tree_adjacency_list[second_neighbor_idx].insert(first_neighbor_idx);
      node_list[node_idx].coord.set_x(std::numeric_limits<int32_t>::max());
      node_list[node_idx].coord.set_y(std::numeric_limits<int32_t>::max());
      route_stats.canonical_degree2_merge_num++;
      changed = true;
    }
  }

  std::vector<GSRTreeNode> compact_node_list;
  std::vector<int32_t> old_to_compact_idx_list(node_list.size(), -1);
  for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(node_list.size()); node_idx++) {
    if (node_list[node_idx].coord.get_x() == std::numeric_limits<int32_t>::max()) {
      route_stats.canonical_removed_node_num++;
      continue;
    }
    old_to_compact_idx_list[node_idx] = static_cast<int32_t>(compact_node_list.size());
    compact_node_list.push_back(node_list[node_idx]);
    compact_node_list.back().child_idx_list.clear();
  }
  for (int32_t old_idx = 0; old_idx < static_cast<int32_t>(tree_adjacency_list.size()); old_idx++) {
    int32_t compact_idx = old_to_compact_idx_list[old_idx];
    if (compact_idx == -1) {
      continue;
    }
    for (int32_t old_neighbor_idx : tree_adjacency_list[old_idx]) {
      int32_t compact_neighbor_idx = old_to_compact_idx_list[old_neighbor_idx];
      if (compact_neighbor_idx == -1 || compact_neighbor_idx == compact_idx) {
        continue;
      }
      if (compact_idx < compact_neighbor_idx) {
        compact_node_list[compact_idx].child_idx_list.push_back(compact_neighbor_idx);
      }
    }
  }

  GSRTree canonical_tree;
  canonical_tree.set_root_idx(old_to_compact_idx_list[root_idx] == -1 ? (compact_node_list.empty() ? -1 : 0) : old_to_compact_idx_list[root_idx]);
  canonical_tree.set_node_list(compact_node_list);
  return canonical_tree;
}

std::vector<GlobalSpatialRouter::GSRPatternCandidate> GlobalSpatialRouter::buildPatternCandidateList(GSRModel& gsr_model, const PlanarCoord& first_coord, const PlanarCoord& second_coord,
                                                                  GSRCongestionView* congestion_view, const bool enable_detour,
                                                                  GSRRouteStats& route_stats)
{
  std::vector<GSRPatternCandidate> candidate_list;
  if (first_coord == second_coord || !gsr_model.get_gsr_grid_graph().isInside(first_coord) || !gsr_model.get_gsr_grid_graph().isInside(second_coord)) {
    return candidate_list;
  }

  auto pushCandidate = [&](const std::vector<PlanarCoord>& coord_list) {
    if (coord_list.size() < 2) {
      return;
    }
    GSRPatternCandidate candidate;
    candidate.coord_list = coord_list;
    candidate_list.push_back(candidate);
  };

  if (first_coord.get_x() == second_coord.get_x() || first_coord.get_y() == second_coord.get_y()) {
    pushCandidate({first_coord, second_coord});
  } else {
    PlanarCoord h_first_bend(second_coord.get_x(), first_coord.get_y());
    PlanarCoord v_first_bend(first_coord.get_x(), second_coord.get_y());
    pushCandidate({first_coord, h_first_bend, second_coord});
    pushCandidate({first_coord, v_first_bend, second_coord});
  }

  bool need_detour = false;
  if (enable_detour && congestion_view != nullptr) {
    for (GSRPatternCandidate& candidate : candidate_list) {
      if (hasCongestion(*congestion_view, candidate.coord_list, gsr_model.get_gsr_com_param().get_detour_congestion_threshold())) {
        need_detour = true;
        break;
      }
    }
  }
  if (!need_detour) {
    return candidate_list;
  }

  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  int32_t target_detour_count = std::max(1, gsr_com_param.get_target_detour_count());
  int32_t trunk_length = RTUTIL.getManhattanDistance(first_coord, second_coord);
  int32_t max_shift = std::max(1, static_cast<int32_t>(std::ceil(trunk_length * gsr_com_param.get_max_detour_ratio())));
  int32_t step = std::max(1, max_shift / target_detour_count);

  auto addShiftedCandidate = [&](const bool horizontal_trunk, const int32_t shift) {
    if (shift == 0) {
      return;
    }
    std::vector<PlanarCoord> coord_list;
    if (horizontal_trunk) {
      int32_t y = clampInt(first_coord.get_y() + shift, 0, gsr_grid_graph.get_y_size() - 1);
      if (y == first_coord.get_y()) {
        return;
      }
      PlanarCoord first_stem(first_coord.get_x(), y);
      PlanarCoord second_stem(second_coord.get_x(), y);
      appendUniqueCoord(coord_list, first_coord);
      appendUniqueCoord(coord_list, first_stem);
      appendUniqueCoord(coord_list, second_stem);
      appendUniqueCoord(coord_list, second_coord);
    } else {
      int32_t x = clampInt(first_coord.get_x() + shift, 0, gsr_grid_graph.get_x_size() - 1);
      if (x == first_coord.get_x()) {
        return;
      }
      PlanarCoord first_stem(x, first_coord.get_y());
      PlanarCoord second_stem(x, second_coord.get_y());
      appendUniqueCoord(coord_list, first_coord);
      appendUniqueCoord(coord_list, first_stem);
      appendUniqueCoord(coord_list, second_stem);
      appendUniqueCoord(coord_list, second_coord);
    }
    if (coord_list.size() >= 2) {
      pushCandidate(coord_list);
      route_stats.detour_candidate_num++;
    }
  };

  for (int32_t shift = step; shift <= max_shift; shift += step) {
    addShiftedCandidate(true, shift);
    addShiftedCandidate(true, -shift);
    addShiftedCandidate(false, shift);
    addShiftedCandidate(false, -shift);
  }
  return candidate_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::refineTreeByPatternLayerDP(GSRModel& gsr_model, GSRNet& gsr_net, GSRTree& gsr_tree,
                                                                 GSRCongestionView* congestion_view, const bool enable_detour,
                                                                 GSRRouteStats& route_stats)
{
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  std::vector<int32_t> preferred_h_layer_list = getCandidateLayerList(gsr_com_param, true);
  std::vector<int32_t> preferred_v_layer_list = getCandidateLayerList(gsr_com_param, false);
  if (preferred_h_layer_list.empty() || preferred_v_layer_list.empty()) {
    RTLOG.error(Loc::current(), "GlobalSpatialRouter cannot route without both H-preferred and V-preferred layer candidates!");
  }

  const double kCostEpsilon = 1.0e-6;
  std::vector<GSRTreeNode>& node_list = gsr_tree.get_node_list();
  if (node_list.empty()) {
    route_stats.tree_layer_dp_fail_num++;
    return {};
  }

  route_stats.tree_layer_dp_route_num++;
  std::vector<int32_t> anchor_layer_list;
  for (int32_t layer_idx = gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
    anchor_layer_list.push_back(layer_idx);
  }
  if (anchor_layer_list.empty()) {
    route_stats.tree_layer_dp_fail_num++;
    return {};
  }

  std::vector<std::set<int32_t>> adjacency_list(node_list.size());
  for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(node_list.size()); node_idx++) {
    for (int32_t child_idx : node_list[node_idx].child_idx_list) {
      if (child_idx < 0 || static_cast<int32_t>(node_list.size()) <= child_idx || child_idx == node_idx) {
        continue;
      }
      adjacency_list[node_idx].insert(child_idx);
      adjacency_list[child_idx].insert(node_idx);
    }
  }

  std::vector<int32_t> parent_idx_list(node_list.size(), -2);
  std::vector<std::vector<int32_t>> child_idx_list(node_list.size());
  std::vector<int32_t> root_idx_list;
  int32_t preferred_root_idx = gsr_tree.get_root_idx();
  if (preferred_root_idx < 0 || static_cast<int32_t>(node_list.size()) <= preferred_root_idx) {
    preferred_root_idx = 0;
  }
  auto rootComponent = [&](const int32_t root_idx) {
    root_idx_list.push_back(root_idx);
    parent_idx_list[root_idx] = -1;
    std::queue<int32_t> node_queue;
    node_queue.push(root_idx);
    while (!node_queue.empty()) {
      int32_t curr_idx = node_queue.front();
      node_queue.pop();
      for (int32_t neighbor_idx : adjacency_list[curr_idx]) {
        if (parent_idx_list[neighbor_idx] != -2) {
          continue;
        }
        parent_idx_list[neighbor_idx] = curr_idx;
        child_idx_list[curr_idx].push_back(neighbor_idx);
        node_queue.push(neighbor_idx);
      }
    }
  };
  rootComponent(preferred_root_idx);
  for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(node_list.size()); node_idx++) {
    if (parent_idx_list[node_idx] == -2) {
      rootComponent(node_idx);
    }
  }

  struct GSREdgeOption
  {
    std::vector<Segment<LayerCoord>> segment_list;
    int32_t parent_endpoint_layer_idx = -1;
    int32_t child_endpoint_layer_idx = -1;
    double cost = std::numeric_limits<double>::max();
  };
  std::map<std::pair<int32_t, int32_t>, std::vector<GSREdgeOption>> edge_option_map;

  auto getEndpointLayerIdx = [](const std::vector<Segment<LayerCoord>>& segment_list, const PlanarCoord& endpoint_coord) {
    for (const Segment<LayerCoord>& segment : segment_list) {
      if (segment.get_first().get_layer_idx() != segment.get_second().get_layer_idx()) {
        continue;
      }
      if (segment.get_first().get_planar_coord() == endpoint_coord) {
        return segment.get_first().get_layer_idx();
      }
      if (segment.get_second().get_planar_coord() == endpoint_coord) {
        return segment.get_second().get_layer_idx();
      }
    }
    return -1;
  };

  auto isValidRoute = [&](std::vector<Segment<LayerCoord>>& segment_list) {
    if (segment_list.empty()) {
      return false;
    }
    for (Segment<LayerCoord>& segment : segment_list) {
      if (!isValidSegment(gsr_model, segment)) {
        return false;
      }
    }
    return true;
  };

  bool option_failed = false;
  for (int32_t parent_idx = 0; parent_idx < static_cast<int32_t>(child_idx_list.size()); parent_idx++) {
    for (int32_t child_idx : child_idx_list[parent_idx]) {
      std::vector<GSREdgeOption>& option_list = edge_option_map[{parent_idx, child_idx}];
      std::vector<GSRPatternCandidate> candidate_list = buildPatternCandidateList(gsr_model, node_list[parent_idx].coord, node_list[child_idx].coord,
                                                                                 congestion_view, enable_detour, route_stats);
      auto appendCapacitySafeOptions = [&](const bool allow_soft_overflow) {
        for (GSRPatternCandidate& candidate : candidate_list) {
          for (int32_t h_layer_idx : preferred_h_layer_list) {
            for (int32_t v_layer_idx : preferred_v_layer_list) {
              std::vector<Segment<LayerCoord>> candidate_segment_list = buildCandidateSegmentList(candidate.coord_list, h_layer_idx, v_layer_idx);
              if (!isValidRoute(candidate_segment_list)) {
                continue;
              }
              if (!isCapacitySafeRoute(gsr_model, gsr_net.get_net_idx(), candidate_segment_list, allow_soft_overflow)) {
                continue;
              }
              GSREdgeOption edge_option;
              edge_option.parent_endpoint_layer_idx = getEndpointLayerIdx(candidate_segment_list, node_list[parent_idx].coord);
              edge_option.child_endpoint_layer_idx = getEndpointLayerIdx(candidate_segment_list, node_list[child_idx].coord);
              if (edge_option.parent_endpoint_layer_idx == -1 || edge_option.child_endpoint_layer_idx == -1) {
                continue;
              }
              edge_option.segment_list = candidate_segment_list;
              edge_option.cost = gsr_grid_graph.getRouteCost(gsr_net.get_net_idx(), edge_option.segment_list, gsr_com_param)
                                 + getRouteCapacityPenalty(gsr_model, gsr_net.get_net_idx(), edge_option.segment_list);
              option_list.push_back(edge_option);
            }
          }
        }
      };
      auto appendFallbackOptions = [&]() {
        for (GSRPatternCandidate& candidate : candidate_list) {
          for (int32_t h_layer_idx : preferred_h_layer_list) {
            for (int32_t v_layer_idx : preferred_v_layer_list) {
              std::vector<Segment<LayerCoord>> candidate_segment_list = buildCandidateSegmentList(candidate.coord_list, h_layer_idx, v_layer_idx);
              if (!isValidRoute(candidate_segment_list)) {
                continue;
              }
              GSREdgeOption edge_option;
              edge_option.parent_endpoint_layer_idx = getEndpointLayerIdx(candidate_segment_list, node_list[parent_idx].coord);
              edge_option.child_endpoint_layer_idx = getEndpointLayerIdx(candidate_segment_list, node_list[child_idx].coord);
              if (edge_option.parent_endpoint_layer_idx == -1 || edge_option.child_endpoint_layer_idx == -1) {
                continue;
              }
              edge_option.segment_list = candidate_segment_list;
              edge_option.cost = gsr_grid_graph.getRouteCost(gsr_net.get_net_idx(), edge_option.segment_list, gsr_com_param)
                                 + getRouteCapacityPenalty(gsr_model, gsr_net.get_net_idx(), edge_option.segment_list);
              option_list.push_back(edge_option);
            }
          }
        }
      };
      appendCapacitySafeOptions(false);
      if (option_list.empty()) {
        appendCapacitySafeOptions(true);
      }
      if (option_list.empty()) {
        appendFallbackOptions();
      }
      route_stats.tree_layer_dp_edge_candidate_num += static_cast<int32_t>(option_list.size());
      if (option_list.empty()) {
        option_failed = true;
      }
    }
  }
  if (option_failed) {
    route_stats.tree_layer_dp_fail_num++;
    return {};
  }

  auto getViaCost = [&](const PlanarCoord& coord, const int32_t first_layer_idx, const int32_t second_layer_idx) {
    return gsr_grid_graph.getProjectedViaCost(gsr_net.get_net_idx(), coord, first_layer_idx, second_layer_idx, gsr_com_param,
                                             gsr_com_param.get_cost_logistic_slope());
  };

  auto getFixedIntervalCost = [&](const GSRTreeNode& node, const int32_t anchor_layer_idx) {
    if (node.fixed_layer_interval.first == -1) {
      return 0.0;
    }
    int32_t low_layer_idx = std::min(anchor_layer_idx, node.fixed_layer_interval.first);
    int32_t high_layer_idx = std::max(anchor_layer_idx, node.fixed_layer_interval.second);
    return getViaCost(node.coord, low_layer_idx, high_layer_idx);
  };

  struct GSRChildChoice
  {
    int32_t child_idx = -1;
    int32_t child_anchor_layer_list_idx = -1;
    int32_t edge_option_idx = -1;
  };
  double inf = std::numeric_limits<double>::max() / 4;
  std::vector<std::vector<double>> dp_cost_list(node_list.size(), std::vector<double>(anchor_layer_list.size(), inf));
  std::vector<std::vector<std::vector<GSRChildChoice>>> dp_choice_list(
      node_list.size(), std::vector<std::vector<GSRChildChoice>>(anchor_layer_list.size()));

  std::vector<int32_t> visit_order;
  visit_order.reserve(node_list.size());
  for (int32_t root_idx : root_idx_list) {
    std::queue<int32_t> node_queue;
    node_queue.push(root_idx);
    while (!node_queue.empty()) {
      int32_t node_idx = node_queue.front();
      node_queue.pop();
      visit_order.push_back(node_idx);
      for (int32_t child_idx : child_idx_list[node_idx]) {
        node_queue.push(child_idx);
      }
    }
  }

  for (auto iter = visit_order.rbegin(); iter != visit_order.rend(); iter++) {
    int32_t node_idx = *iter;
    for (int32_t anchor_list_idx = 0; anchor_list_idx < static_cast<int32_t>(anchor_layer_list.size()); anchor_list_idx++) {
      int32_t anchor_layer_idx = anchor_layer_list[anchor_list_idx];
      double node_cost = getFixedIntervalCost(node_list[node_idx], anchor_layer_idx);
      std::vector<GSRChildChoice> child_choice_list;
      bool state_valid = true;
      for (int32_t child_idx : child_idx_list[node_idx]) {
        std::vector<GSREdgeOption>& edge_option_list = edge_option_map[{node_idx, child_idx}];
        double best_child_cost = inf;
        GSRChildChoice best_child_choice;
        best_child_choice.child_idx = child_idx;
        for (int32_t edge_option_idx = 0; edge_option_idx < static_cast<int32_t>(edge_option_list.size()); edge_option_idx++) {
          GSREdgeOption& edge_option = edge_option_list[edge_option_idx];
          double parent_endpoint_cost = getViaCost(node_list[node_idx].coord, anchor_layer_idx, edge_option.parent_endpoint_layer_idx);
          for (int32_t child_anchor_list_idx = 0; child_anchor_list_idx < static_cast<int32_t>(anchor_layer_list.size()); child_anchor_list_idx++) {
            if (dp_cost_list[child_idx][child_anchor_list_idx] >= inf) {
              continue;
            }
            int32_t child_anchor_layer_idx = anchor_layer_list[child_anchor_list_idx];
            double child_endpoint_cost
                = getViaCost(node_list[child_idx].coord, child_anchor_layer_idx, edge_option.child_endpoint_layer_idx);
            double candidate_cost = edge_option.cost + parent_endpoint_cost + child_endpoint_cost
                                    + dp_cost_list[child_idx][child_anchor_list_idx];
            if (candidate_cost + kCostEpsilon < best_child_cost) {
              best_child_cost = candidate_cost;
              best_child_choice.child_anchor_layer_list_idx = child_anchor_list_idx;
              best_child_choice.edge_option_idx = edge_option_idx;
            }
          }
        }
        if (best_child_choice.edge_option_idx == -1) {
          state_valid = false;
          break;
        }
        node_cost += best_child_cost;
        child_choice_list.push_back(best_child_choice);
      }
      if (!state_valid) {
        continue;
      }
      dp_cost_list[node_idx][anchor_list_idx] = node_cost;
      dp_choice_list[node_idx][anchor_list_idx] = child_choice_list;
      route_stats.tree_layer_dp_state_num++;
    }
  }

  std::vector<Segment<LayerCoord>> routing_segment_list;
  auto appendFixedIntervalVia = [&](const GSRTreeNode& node, const int32_t anchor_layer_idx) {
    if (node.fixed_layer_interval.first == -1) {
      return;
    }
    int32_t low_layer_idx = std::min(anchor_layer_idx, node.fixed_layer_interval.first);
    int32_t high_layer_idx = std::max(anchor_layer_idx, node.fixed_layer_interval.second);
    addViaChain(routing_segment_list, node.coord, low_layer_idx, high_layer_idx);
  };

  std::function<void(int32_t, int32_t)> collectRoute = [&](const int32_t node_idx, const int32_t anchor_list_idx) {
    int32_t anchor_layer_idx = anchor_layer_list[anchor_list_idx];
    appendFixedIntervalVia(node_list[node_idx], anchor_layer_idx);
    for (GSRChildChoice& child_choice : dp_choice_list[node_idx][anchor_list_idx]) {
      int32_t child_idx = child_choice.child_idx;
      GSREdgeOption& edge_option = edge_option_map[{node_idx, child_idx}][child_choice.edge_option_idx];
      int32_t child_anchor_layer_idx = anchor_layer_list[child_choice.child_anchor_layer_list_idx];
      addViaChain(routing_segment_list, node_list[node_idx].coord, anchor_layer_idx, edge_option.parent_endpoint_layer_idx);
      addViaChain(routing_segment_list, node_list[child_idx].coord, child_anchor_layer_idx, edge_option.child_endpoint_layer_idx);
      routing_segment_list.insert(routing_segment_list.end(), edge_option.segment_list.begin(), edge_option.segment_list.end());
      collectRoute(child_idx, child_choice.child_anchor_layer_list_idx);
    }
  };

  for (int32_t root_idx : root_idx_list) {
    double best_root_cost = inf;
    int32_t best_root_anchor_list_idx = -1;
    for (int32_t anchor_list_idx = 0; anchor_list_idx < static_cast<int32_t>(anchor_layer_list.size()); anchor_list_idx++) {
      if (dp_cost_list[root_idx][anchor_list_idx] + kCostEpsilon < best_root_cost) {
        best_root_cost = dp_cost_list[root_idx][anchor_list_idx];
        best_root_anchor_list_idx = anchor_list_idx;
      }
    }
    if (best_root_anchor_list_idx == -1 || best_root_cost >= inf) {
      route_stats.tree_layer_dp_fail_num++;
      return {};
    }
    collectRoute(root_idx, best_root_anchor_list_idx);
  }

  if (!routing_segment_list.empty()) {
    route_stats.pattern_refine_success_num++;
    route_stats.tree_layer_dp_success_num++;
  } else {
    route_stats.tree_layer_dp_fail_num++;
  }
  return routing_segment_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::buildCandidateSegmentList(const std::vector<PlanarCoord>& coord_list, const int32_t h_layer_idx,
                                                               const int32_t v_layer_idx)
{
  std::vector<Segment<LayerCoord>> segment_list;
  if (coord_list.size() < 2) {
    return segment_list;
  }
  int32_t previous_layer_idx = -1;
  for (size_t i = 1; i < coord_list.size(); i++) {
    PlanarCoord first_coord = coord_list[i - 1];
    PlanarCoord second_coord = coord_list[i];
    if (first_coord == second_coord) {
      continue;
    }
    Direction direction = RTUTIL.getDirection(first_coord, second_coord);
    int32_t layer_idx = -1;
    if (direction == Direction::kHorizontal) {
      layer_idx = h_layer_idx;
    } else if (direction == Direction::kVertical) {
      layer_idx = v_layer_idx;
    } else {
      return {};
    }
    if (previous_layer_idx != -1 && previous_layer_idx != layer_idx) {
      addViaChain(segment_list, first_coord, previous_layer_idx, layer_idx);
    }
    segment_list.emplace_back(makeLayerCoord(first_coord, layer_idx), makeLayerCoord(second_coord, layer_idx));
    previous_layer_idx = layer_idx;
  }
  return segment_list;
}

// validation and capacity

bool GlobalSpatialRouter::hasCongestion(GSRCongestionView& congestion_view, const std::vector<PlanarCoord>& coord_list, const double threshold)
{
  double risk_threshold = std::max(threshold, 0.1);
  for (size_t i = 1; i < coord_list.size(); i++) {
    PlanarCoord first_coord = coord_list[i - 1];
    PlanarCoord second_coord = coord_list[i];
    Direction direction = RTUTIL.getDirection(first_coord, second_coord);
    std::vector<PlanarCoord> line_coord_list = getLineCoordList(first_coord, second_coord);
    for (PlanarCoord& coord : line_coord_list) {
      if (direction == Direction::kHorizontal && congestion_view.h_overflow_map.isInside(coord.get_x(), coord.get_y())
          && congestion_view.h_overflow_map[coord.get_x()][coord.get_y()] > threshold) {
        return true;
      }
      if (direction == Direction::kVertical && congestion_view.v_overflow_map.isInside(coord.get_x(), coord.get_y())
          && congestion_view.v_overflow_map[coord.get_x()][coord.get_y()] > threshold) {
        return true;
      }
      if (congestion_view.internal_overflow_map.isInside(coord.get_x(), coord.get_y())
          && congestion_view.internal_overflow_map[coord.get_x()][coord.get_y()] > threshold) {
        return true;
      }
      if (congestion_view.via_risk_map.isInside(coord.get_x(), coord.get_y())
          && congestion_view.via_risk_map[coord.get_x()][coord.get_y()] > threshold) {
        return true;
      }
      if (congestion_view.capacity_pressure_map.isInside(coord.get_x(), coord.get_y())
          && congestion_view.capacity_pressure_map[coord.get_x()][coord.get_y()] > threshold) {
        return true;
      }
      if (congestion_view.capacity_block_map.isInside(coord.get_x(), coord.get_y())
          && congestion_view.capacity_block_map[coord.get_x()][coord.get_y()] > 0) {
        return true;
      }
      if (getCongestionRisk(congestion_view, direction, coord) > risk_threshold) {
        return true;
      }
    }
  }
  return false;
}

std::vector<Orientation> GlobalSpatialRouter::getViaSideOrientList(const GSRNode& gsr_node)
{
  double east_west_supply = gsr_node.getSupply(Orientation::kEast) + gsr_node.getSupply(Orientation::kWest);
  double north_south_supply = gsr_node.getSupply(Orientation::kNorth) + gsr_node.getSupply(Orientation::kSouth);
  if (east_west_supply >= north_south_supply) {
    return {Orientation::kEast, Orientation::kWest};
  }
  return {Orientation::kNorth, Orientation::kSouth};
}

GlobalSpatialRouter::GSRUsageCapacityEval GlobalSpatialRouter::evalUsageCapacity(const GSRModel& gsr_model, const GSRNode& gsr_node,
                                                                                 const int32_t net_idx,
                                                                                 const std::set<Orientation>& orient_set)
{
  GSRUsageCapacityEval capacity_eval;
  double internal_demand = gsr_node.getInternalDemand(net_idx, &orient_set);
  double internal_supply = gsr_node.getInternalSupply();
  if (internal_supply <= RT_ERROR && internal_demand > 0) {
    capacity_eval.internal_hard_blocked = true;
  } else {
    capacity_eval.internal_overflow = std::max(0.0, internal_demand - internal_supply);
  }

  if (!RTUTIL.exist(orient_set, Orientation::kAbove) && !RTUTIL.exist(orient_set, Orientation::kBelow)) {
    return capacity_eval;
  }

  double demand_unit = gsr_model.get_gsr_com_param().get_via_min_area_demand_unit() * gsr_model.get_gsr_com_param().get_via_multiplier();
  for (Orientation orient : getViaSideOrientList(gsr_node)) {
    double side_demand = gsr_node.getBoundaryDemand(orient) + demand_unit;
    double side_supply = gsr_node.getSupply(orient);
    if (side_supply <= RT_ERROR && side_demand > 0) {
      capacity_eval.via_side_hard_block_num++;
    } else {
      capacity_eval.via_side_overflow += std::max(0.0, side_demand - side_supply);
    }
  }
  return capacity_eval;
}

bool GlobalSpatialRouter::isCapacitySafeRoute(GSRModel& gsr_model, const int32_t net_idx, const std::vector<Segment<LayerCoord>>& segment_list,
                             const bool allow_soft_overflow)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  if (segment_list.empty()) {
    return false;
  }
  std::vector<GridMap<GSRNode>>& layer_node_map = gsr_grid_graph.get_layer_node_map();
  for (auto& [usage_coord, orient_set] : gsr_grid_graph.getRouteUsageMap(segment_list)) {
    if (!gsr_grid_graph.isInside(usage_coord)) {
      return false;
    }
    GSRNode& gsr_node = layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()];
    GSRUsageCapacityEval capacity_eval = evalUsageCapacity(gsr_model, gsr_node, net_idx, orient_set);
    if (capacity_eval.internal_hard_blocked || capacity_eval.via_side_hard_block_num > 0) {
      return false;
    }
    if (!allow_soft_overflow && (capacity_eval.internal_overflow > RT_ERROR || capacity_eval.via_side_overflow > RT_ERROR)) {
      return false;
    }
  }
  return true;
}

double GlobalSpatialRouter::getRouteCapacityPenalty(GSRModel& gsr_model, const int32_t net_idx, const std::vector<Segment<LayerCoord>>& segment_list)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  double penalty = 0;
  std::vector<GridMap<GSRNode>>& layer_node_map = gsr_grid_graph.get_layer_node_map();
  for (auto& [usage_coord, orient_set] : gsr_grid_graph.getRouteUsageMap(segment_list)) {
    if (!gsr_grid_graph.isInside(usage_coord)) {
      penalty += kCapacityBlockPenalty;
      continue;
    }
    GSRNode& gsr_node = layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()];
    GSRUsageCapacityEval capacity_eval = evalUsageCapacity(gsr_model, gsr_node, net_idx, orient_set);
    if (capacity_eval.internal_hard_blocked) {
      penalty += kCapacityBlockPenalty;
    } else {
      penalty += kCapacityOverflowPenalty * capacity_eval.internal_overflow;
    }
    penalty += kCapacityBlockPenalty * capacity_eval.via_side_hard_block_num;
    penalty += kViaSidePenalty * capacity_eval.via_side_overflow;
  }
  return penalty;
}

bool GlobalSpatialRouter::isRouteConnected(GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& segment_list)
{
  std::vector<LayerCoord> key_coord_list;
  std::set<LayerCoord, CmpLayerCoordByXASC> visited_key_coord_set;
  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    LayerCoord access_coord = gsr_pin.get_access_coord();
    if (visited_key_coord_set.insert(access_coord).second) {
      key_coord_list.push_back(access_coord);
    }
  }
  if (key_coord_list.size() < 2) {
    return true;
  }
  std::vector<Segment<LayerCoord>> routing_segment_list = segment_list;
  return RTUTIL.passCheckingConnectivity(key_coord_list, routing_segment_list);
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::getRoutingSegmentList(GSRModel& gsr_model, GSRNet& gsr_net, std::vector<Segment<PlanarCoord>>& planar_topo_list,
                                                           GSRRouteStats& route_stats)
{
  (void) route_stats;

  const double kCostEpsilon = 1.0e-6;
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  std::vector<int32_t> preferred_h_layer_list = getCandidateLayerList(gsr_com_param, true);
  std::vector<int32_t> preferred_v_layer_list = getCandidateLayerList(gsr_com_param, false);
  if (preferred_h_layer_list.empty() || preferred_v_layer_list.empty()) {
    RTLOG.error(Loc::current(), "GlobalSpatialRouter cannot route without both H-preferred and V-preferred layer candidates!");
  }

  std::vector<Segment<LayerCoord>> routing_segment_list;
  LayerSetByPlanarCoord layer_set_by_planar_coord;
  for (Segment<PlanarCoord>& planar_topo : planar_topo_list) {
    PlanarCoord first_coord = planar_topo.get_first();
    PlanarCoord second_coord = planar_topo.get_second();
    if (!gsr_model.get_gsr_grid_graph().isInside(first_coord) || !gsr_model.get_gsr_grid_graph().isInside(second_coord)) {
      continue;
    }

    std::vector<Segment<LayerCoord>> best_segment_list;
    double best_cost = std::numeric_limits<double>::max();
    for (bool h_first : {true, false}) {
      if ((first_coord.get_x() == second_coord.get_x()) && h_first) {
        continue;
      }
      if ((first_coord.get_y() == second_coord.get_y()) && !h_first) {
        continue;
      }
      for (int32_t h_layer_idx : preferred_h_layer_list) {
        for (int32_t v_layer_idx : preferred_v_layer_list) {
          std::vector<Segment<LayerCoord>> candidate_segment_list = buildPatternRoute(first_coord, second_coord, h_first, h_layer_idx, v_layer_idx);
          if (candidate_segment_list.empty()) {
            continue;
          }
          double candidate_cost = gsr_model.get_gsr_grid_graph().getRouteCost(gsr_net.get_net_idx(), candidate_segment_list, gsr_com_param);
          if (candidate_cost + kCostEpsilon < best_cost) {
            best_cost = candidate_cost;
            best_segment_list = candidate_segment_list;
          }
        }
      }
    }
    for (Segment<LayerCoord>& segment : best_segment_list) {
      routing_segment_list.push_back(segment);
      layer_set_by_planar_coord[segment.get_first().get_planar_coord()].insert(segment.get_first().get_layer_idx());
      layer_set_by_planar_coord[segment.get_second().get_planar_coord()].insert(segment.get_second().get_layer_idx());
    }
  }

  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    LayerCoord access_coord = gsr_pin.get_access_coord();
    if (gsr_model.get_gsr_grid_graph().isInside(access_coord)) {
      layer_set_by_planar_coord[access_coord.get_planar_coord()].insert(access_coord.get_layer_idx());
    }
  }
  for (auto& [planar_coord, layer_idx_set] : layer_set_by_planar_coord) {
    if (layer_idx_set.size() < 2) {
      continue;
    }
    addViaChain(routing_segment_list, planar_coord, *layer_idx_set.begin(), *layer_idx_set.rbegin());
  }

  return routing_segment_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::getValidUniqueSegmentList(GSRModel& gsr_model, std::vector<Segment<LayerCoord>>& routing_segment_list,
                                                               GSRRouteStats& route_stats)
{
  std::vector<Segment<LayerCoord>> valid_segment_list;
  std::set<SegmentKey> visited_segment_key_set;
  for (Segment<LayerCoord>& routing_segment : routing_segment_list) {
    if (!isValidSegment(gsr_model, routing_segment)) {
      route_stats.invalid_segment_num++;
      if (routing_segment.get_first().get_layer_idx() == routing_segment.get_second().get_layer_idx()
          && gsr_model.get_gsr_grid_graph().isInside(routing_segment.get_first())
          && gsr_model.get_gsr_grid_graph().isInside(routing_segment.get_second())) {
        Direction segment_direction = getPlanarSegmentDirection(routing_segment);
        if (segment_direction == Direction::kHorizontal || segment_direction == Direction::kVertical) {
          std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
          int32_t layer_idx = routing_segment.get_first().get_layer_idx();
          if (0 <= layer_idx && layer_idx < static_cast<int32_t>(routing_layer_list.size())
              && routing_layer_list[layer_idx].get_prefer_direction() != segment_direction) {
            route_stats.non_preferred_reject_num++;
          }
        }
      }
      continue;
    }
    if (!visited_segment_key_set.insert(makeSegmentKey(routing_segment)).second) {
      continue;
    }
    valid_segment_list.push_back(routing_segment);
  }
  return valid_segment_list;
}

// result upload and model state

void GlobalSpatialRouter::uploadNetResult(GSRNet& gsr_net, std::vector<Segment<LayerCoord>>& routing_segment_list, GSRRouteStats& route_stats)
{
  for (Segment<LayerCoord>& routing_segment : routing_segment_list) {
    if (routing_segment.get_first().get_layer_idx() == routing_segment.get_second().get_layer_idx()) {
      route_stats.same_layer_segment_num++;
    } else {
      route_stats.via_segment_num++;
    }
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, gsr_net.get_net_idx(), new Segment<LayerCoord>(routing_segment));
    route_stats.uploaded_segment_num++;
  }
}

void GlobalSpatialRouter::uploadGSRModelResult(GSRModel& gsr_model, GSRRouteStats& route_stats)
{
  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    std::vector<Segment<LayerCoord>> routing_segment_list = gsr_net.get_routing_segment_list();
    if (!routing_segment_list.empty()) {
      std::vector<Segment<LayerCoord>> sanitized_segment_list = sanitizeRouteSegmentList(gsr_model, gsr_net, routing_segment_list, route_stats);
      if (!sanitized_segment_list.empty() && makeSegmentKeySet(routing_segment_list) != makeSegmentKeySet(sanitized_segment_list)) {
        removeRouteDemand(gsr_model, gsr_net, routing_segment_list);
        gsr_net.set_routing_segment_list(sanitized_segment_list);
        addRouteDemand(gsr_model, gsr_net, gsr_net.get_routing_segment_list());
        updateGSRNetCost(gsr_model, gsr_net);
      }
      routing_segment_list = gsr_net.get_routing_segment_list();
      if (!routing_segment_list.empty() && hasPlanarProjectionCycle(gsr_net, routing_segment_list, route_stats)) {
        std::vector<Segment<LayerCoord>> repaired_segment_list = repairPlanarProjectionCycle(gsr_model, gsr_net, routing_segment_list, route_stats);
        if (!repaired_segment_list.empty()) {
          removeRouteDemand(gsr_model, gsr_net, routing_segment_list);
          gsr_net.set_routing_segment_list(repaired_segment_list);
          addRouteDemand(gsr_model, gsr_net, gsr_net.get_routing_segment_list());
          updateGSRNetCost(gsr_model, gsr_net);
        }
      }
      routing_segment_list = gsr_net.get_routing_segment_list();
      if (!routing_segment_list.empty() && hasPlanarProjectionOverlap(gsr_net, routing_segment_list, route_stats)) {
        std::vector<Segment<LayerCoord>> repaired_segment_list = repairPlanarProjectionOverlap(gsr_model, gsr_net, routing_segment_list, route_stats);
        if (!repaired_segment_list.empty()) {
          removeRouteDemand(gsr_model, gsr_net, routing_segment_list);
          gsr_net.set_routing_segment_list(repaired_segment_list);
          addRouteDemand(gsr_model, gsr_net, gsr_net.get_routing_segment_list());
          updateGSRNetCost(gsr_model, gsr_net);
        }
      }
    }
    routing_segment_list = gsr_net.get_routing_segment_list();
    uploadNetResult(gsr_net, routing_segment_list, route_stats);
  }
  updateGSRModelCost(gsr_model);
}

void GlobalSpatialRouter::addRouteDemand(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& routing_segment_list)
{
  gsr_model.get_gsr_grid_graph().updateDemandToGraph(ChangeType::kAdd, gsr_net.get_net_idx(), routing_segment_list);
}

void GlobalSpatialRouter::removeRouteDemand(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& routing_segment_list)
{
  gsr_model.get_gsr_grid_graph().updateDemandToGraph(ChangeType::kDel, gsr_net.get_net_idx(), routing_segment_list);
}

void GlobalSpatialRouter::updateGSRModelCost(GSRModel& gsr_model)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  gsr_model.set_total_overflow(gsr_grid_graph.getTotalOverflow());
  gsr_model.set_total_congestion_risk(gsr_grid_graph.getTotalCongestionRisk(gsr_model.get_gsr_net_list()));
  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    updateGSRNetCost(gsr_model, gsr_net);
  }
}

void GlobalSpatialRouter::updateGSRNetCost(GSRModel& gsr_model, GSRNet& gsr_net)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  gsr_net.set_route_overflow(gsr_grid_graph.getRouteOverflow(gsr_net.get_routing_segment_list()));
  gsr_net.set_route_congestion_risk(gsr_grid_graph.getRouteCongestionRisk(gsr_net.get_routing_segment_list()));
}

void GlobalSpatialRouter::updateBestResult(GSRModel& gsr_model)
{
  if (gsr_model.get_total_overflow() > gsr_model.get_best_total_overflow()) {
    return;
  }
  if (RTUTIL.equalDoubleByError(gsr_model.get_total_overflow(), gsr_model.get_best_total_overflow(), RT_ERROR)
      && gsr_model.get_total_congestion_risk() >= gsr_model.get_best_total_congestion_risk()) {
    return;
  }

  std::map<int32_t, std::vector<Segment<LayerCoord>>> best_net_result_map;
  std::map<int32_t, GSRTree> best_net_tree_map;
  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    best_net_result_map[gsr_net.get_net_idx()] = gsr_net.get_routing_segment_list();
    best_net_tree_map[gsr_net.get_net_idx()] = gsr_net.get_routing_tree();
    gsr_net.set_best_routing_segment_list(gsr_net.get_routing_segment_list());
    gsr_net.set_best_routing_tree(gsr_net.get_routing_tree());
  }
  gsr_model.set_best_net_result_map(best_net_result_map);
  gsr_model.set_best_net_tree_map(best_net_tree_map);
  gsr_model.set_best_total_overflow(gsr_model.get_total_overflow());
  gsr_model.set_best_total_congestion_risk(gsr_model.get_total_congestion_risk());
}

void GlobalSpatialRouter::selectBestResult(GSRModel& gsr_model)
{
  std::map<int32_t, std::vector<Segment<LayerCoord>>>& best_net_result_map = gsr_model.get_best_net_result_map();
  std::map<int32_t, GSRTree>& best_net_tree_map = gsr_model.get_best_net_tree_map();
  if (best_net_result_map.empty()) {
    return;
  }

  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    if (!gsr_net.get_routing_segment_list().empty()) {
      removeRouteDemand(gsr_model, gsr_net, gsr_net.get_routing_segment_list());
    }
  }
  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    if (RTUTIL.exist(best_net_result_map, gsr_net.get_net_idx())) {
      gsr_net.set_routing_segment_list(best_net_result_map[gsr_net.get_net_idx()]);
      if (RTUTIL.exist(best_net_tree_map, gsr_net.get_net_idx())) {
        gsr_net.set_routing_tree(best_net_tree_map[gsr_net.get_net_idx()]);
      }
      addRouteDemand(gsr_model, gsr_net, gsr_net.get_routing_segment_list());
      updateGSRNetCost(gsr_model, gsr_net);
    }
  }
  GSRComParam& gsr_com_param = gsr_model.get_gsr_com_param();
  gsr_grid_graph.updateCongestionRisk(gsr_model.get_layer_congestion_risk_map(), gsr_com_param.get_congestion_risk_radius(),
                                     gsr_com_param.get_history_risk_decay());
  updateGSRModelCost(gsr_model);
}

// reroute transaction and acceptance

GlobalSpatialRouter::GSRRouteSnapshot GlobalSpatialRouter::snapshotRoute(GSRModel& gsr_model, GSRNet& gsr_net)
{
  GSRRouteSnapshot route_snapshot;
  route_snapshot.old_segment_list = gsr_net.get_routing_segment_list();
  route_snapshot.old_tree = gsr_net.get_routing_tree();
  route_snapshot.old_total_overflow = gsr_model.get_total_overflow();
  route_snapshot.old_total_congestion_risk = gsr_model.get_total_congestion_risk();
  route_snapshot.old_route_overflow = gsr_net.get_route_overflow();
  route_snapshot.old_route_congestion_risk = gsr_net.get_route_congestion_risk();
  route_snapshot.old_route_cost
      = gsr_model.get_gsr_grid_graph().getRouteCost(gsr_net.get_net_idx(), route_snapshot.old_segment_list, gsr_model.get_gsr_com_param());
  return route_snapshot;
}

void GlobalSpatialRouter::initRerouteAttemptRecord(const GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot,
                                                   GSRRerouteAttemptRecord* attempt_record)
{
  if (attempt_record == nullptr) {
    return;
  }
  attempt_record->net_idx = gsr_net.get_net_idx();
  attempt_record->old_segment_num = static_cast<int32_t>(route_snapshot.old_segment_list.size());
  attempt_record->old_total_overflow = route_snapshot.old_total_overflow;
  attempt_record->new_total_overflow = route_snapshot.old_total_overflow;
  attempt_record->old_route_overflow = route_snapshot.old_route_overflow;
  attempt_record->new_route_overflow = route_snapshot.old_route_overflow;
  attempt_record->old_total_congestion_risk = route_snapshot.old_total_congestion_risk;
  attempt_record->new_total_congestion_risk = route_snapshot.old_total_congestion_risk;
  attempt_record->old_route_congestion_risk = route_snapshot.old_route_congestion_risk;
  attempt_record->new_route_congestion_risk = route_snapshot.old_route_congestion_risk;
  attempt_record->old_route_cost = route_snapshot.old_route_cost;
  attempt_record->new_route_cost = route_snapshot.old_route_cost;
  attempt_record->result = "reject";
}

void GlobalSpatialRouter::removeSnapshotRoute(GSRModel& gsr_model, GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot,
                                              GSRWireCostView* wire_cost_view, GSRRouteStats* route_stats)
{
  removeRouteDemand(gsr_model, gsr_net, route_snapshot.old_segment_list);
  if (wire_cost_view != nullptr && route_stats != nullptr) {
    updateWireCostView(gsr_model, *wire_cost_view, route_snapshot.old_segment_list, route_stats);
  }
  gsr_net.set_routing_segment_list(route_snapshot.old_segment_list);
  gsr_net.set_routing_tree(route_snapshot.old_tree);
}

void GlobalSpatialRouter::restoreSnapshotRoute(GSRModel& gsr_model, GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot,
                                               GSRWireCostView* wire_cost_view, GSRRouteStats* route_stats)
{
  addRouteDemand(gsr_model, gsr_net, route_snapshot.old_segment_list);
  if (wire_cost_view != nullptr && route_stats != nullptr) {
    updateWireCostView(gsr_model, *wire_cost_view, route_snapshot.old_segment_list, route_stats);
  }
  gsr_net.set_routing_segment_list(route_snapshot.old_segment_list);
  gsr_net.set_routing_tree(route_snapshot.old_tree);
  updateGSRNetCost(gsr_model, gsr_net);
  gsr_model.set_total_overflow(route_snapshot.old_total_overflow);
  gsr_model.set_total_congestion_risk(route_snapshot.old_total_congestion_risk);
}

bool GlobalSpatialRouter::tryCommitCandidateRoute(GSRModel& gsr_model, GSRNet& gsr_net, const GSRRouteSnapshot& route_snapshot,
                                                  std::vector<Segment<LayerCoord>>& candidate_segment_list, GSRRouteStats& route_stats,
                                                  GSRRerouteAttemptRecord* attempt_record, const int32_t stage,
                                                  const std::string& empty_result,
                                                  const std::chrono::steady_clock::time_point& attempt_start_time,
                                                  GSRWireCostView* wire_cost_view)
{
  if (candidate_segment_list.empty()) {
    if (attempt_record != nullptr) {
      attempt_record->result = empty_result;
      attempt_record->new_segment_num = static_cast<int32_t>(candidate_segment_list.size());
    }
    return false;
  }

  auto sanitize_start_time = std::chrono::steady_clock::now();
  std::vector<Segment<LayerCoord>> sanitized_segment_list = sanitizeRouteSegmentList(gsr_model, gsr_net, candidate_segment_list, route_stats);
  appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, stage, "sanitize_candidate_route", getElapsedMs(sanitize_start_time), 1);
  if (sanitized_segment_list.empty() || !isRoutePreferredOnly(gsr_model, sanitized_segment_list)) {
    if (attempt_record != nullptr) {
      attempt_record->result = isRoutePreferredOnly(gsr_model, candidate_segment_list) ? "sanitize_fail" : "non_preferred";
      attempt_record->new_segment_num = static_cast<int32_t>(sanitized_segment_list.size());
    }
    return false;
  }
  candidate_segment_list = sanitized_segment_list;

  auto planar_cycle_start_time = std::chrono::steady_clock::now();
  if (hasPlanarProjectionCycle(gsr_net, candidate_segment_list, route_stats)) {
    std::vector<Segment<LayerCoord>> repaired_segment_list = repairPlanarProjectionCycle(gsr_model, gsr_net, candidate_segment_list, route_stats);
    appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, stage, "repair_planar_cycle", getElapsedMs(planar_cycle_start_time), 1);
    if (repaired_segment_list.empty()) {
      if (attempt_record != nullptr) {
        attempt_record->result = "reject_planar_cycle";
        attempt_record->accept_reason = "planar_cycle_repair_fail";
        attempt_record->new_segment_num = static_cast<int32_t>(candidate_segment_list.size());
      }
      return false;
    }
    candidate_segment_list = repaired_segment_list;
    if (attempt_record != nullptr) {
      attempt_record->accept_reason = "planar_cycle_repair";
    }
  } else {
    appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, stage, "check_planar_cycle", getElapsedMs(planar_cycle_start_time), 1);
  }

  auto planar_overlap_start_time = std::chrono::steady_clock::now();
  if (hasPlanarProjectionOverlap(gsr_net, candidate_segment_list, route_stats)) {
    std::vector<Segment<LayerCoord>> repaired_segment_list = repairPlanarProjectionOverlap(gsr_model, gsr_net, candidate_segment_list, route_stats);
    appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, stage, "repair_planar_overlap", getElapsedMs(planar_overlap_start_time), 1);
    if (repaired_segment_list.empty()) {
      if (attempt_record != nullptr) {
        attempt_record->result = "reject_planar_overlap";
        attempt_record->accept_reason = "planar_overlap_repair_fail";
        attempt_record->new_segment_num = static_cast<int32_t>(candidate_segment_list.size());
      }
      return false;
    }
    candidate_segment_list = repaired_segment_list;
    if (attempt_record != nullptr) {
      attempt_record->accept_reason = "planar_overlap_repair";
    }
  } else {
    appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, stage, "check_planar_overlap", getElapsedMs(planar_overlap_start_time), 1);
  }

  auto eval_start_time = std::chrono::steady_clock::now();
  GSRLocalRouteEval route_eval
      = evalCandidateRouteByLocalDelta(gsr_model, gsr_net, route_snapshot.old_segment_list, candidate_segment_list,
                                       route_snapshot.old_total_overflow, route_snapshot.old_total_congestion_risk,
                                       route_snapshot.old_route_congestion_risk);
  appendRerouteTiming(attempt_record == nullptr ? -1 : attempt_record->iter, stage, "eval_candidate_route", getElapsedMs(eval_start_time), 1);

  std::string accept_reason = "reject";
  if (attempt_record != nullptr) {
    attempt_record->new_segment_num = static_cast<int32_t>(candidate_segment_list.size());
    attempt_record->old_touched_overflow = route_eval.old_touched_overflow;
    attempt_record->new_touched_overflow = route_eval.new_touched_overflow;
    attempt_record->new_route_overflow = route_eval.new_route_overflow;
    attempt_record->new_route_congestion_risk = route_eval.new_route_congestion_risk;
    attempt_record->new_route_cost = route_eval.new_route_cost;
    attempt_record->new_total_overflow = route_eval.new_total_overflow;
    attempt_record->new_total_congestion_risk = route_eval.new_total_congestion_risk;
  }

  if (acceptNewRoute(gsr_model, gsr_net, route_snapshot.old_segment_list, candidate_segment_list, route_snapshot.old_total_overflow,
                     route_eval.new_total_overflow, route_snapshot.old_total_congestion_risk, route_eval.new_total_congestion_risk,
                     route_eval.old_touched_overflow, route_eval.new_touched_overflow, route_snapshot.old_route_cost,
                     route_eval.new_route_cost, route_stats, &accept_reason)) {
    gsr_model.set_total_overflow(route_eval.new_total_overflow);
    gsr_model.set_total_congestion_risk(route_eval.new_total_congestion_risk);
    gsr_net.set_route_overflow(route_eval.new_route_overflow);
    gsr_net.set_route_congestion_risk(route_eval.new_route_congestion_risk);
    gsr_net.set_routing_segment_list(candidate_segment_list);
    route_stats.reroute_accept_num++;
    gsr_net.addRoutedTimes();
    if (attempt_record != nullptr) {
      attempt_record->result = "accept";
      attempt_record->accept_reason = accept_reason;
      attempt_record->runtime_ms = getElapsedMs(attempt_start_time);
    }
    return true;
  }

  removeRouteDemand(gsr_model, gsr_net, candidate_segment_list);
  if (wire_cost_view != nullptr) {
    updateWireCostView(gsr_model, *wire_cost_view, candidate_segment_list, &route_stats);
  }
  gsr_net.set_routing_segment_list({});
  if (attempt_record != nullptr) {
    attempt_record->result = "reject";
    attempt_record->accept_reason = accept_reason;
  }
  return false;
}

GlobalSpatialRouter::GSRLocalRouteEval GlobalSpatialRouter::evalCandidateRouteByLocalDelta(GSRModel& gsr_model, GSRNet& gsr_net,
                                                        const std::vector<Segment<LayerCoord>>& old_segment_list,
                                                        const std::vector<Segment<LayerCoord>>& new_segment_list,
                                                        const double old_total_overflow, const double old_total_congestion_risk,
                                                        const double old_route_congestion_risk)
{
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  GSRLocalRouteEval route_eval;
  std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> touched_usage_map
      = getMergedRouteUsageMap(gsr_grid_graph, old_segment_list, new_segment_list);

  addRouteDemand(gsr_model, gsr_net, old_segment_list);
  route_eval.old_touched_overflow = calcUsageOverflow(gsr_grid_graph, touched_usage_map);
  removeRouteDemand(gsr_model, gsr_net, old_segment_list);

  addRouteDemand(gsr_model, gsr_net, new_segment_list);
  route_eval.new_touched_overflow = calcUsageOverflow(gsr_grid_graph, touched_usage_map);
  route_eval.new_route_overflow = gsr_grid_graph.getRouteOverflow(new_segment_list);
  route_eval.new_route_congestion_risk = gsr_grid_graph.getRouteCongestionRisk(new_segment_list);
  route_eval.new_route_cost = gsr_grid_graph.getRouteCost(gsr_net.get_net_idx(), new_segment_list, gsr_model.get_gsr_com_param());
  route_eval.new_total_overflow = old_total_overflow + route_eval.new_touched_overflow - route_eval.old_touched_overflow;
  route_eval.new_total_congestion_risk = old_total_congestion_risk - old_route_congestion_risk + route_eval.new_route_congestion_risk;
  return route_eval;
}

bool GlobalSpatialRouter::acceptNewRoute(GSRModel& gsr_model, GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& old_segment_list,
                        const std::vector<Segment<LayerCoord>>& new_segment_list, const double old_total_overflow,
                        const double new_total_overflow, const double old_total_congestion_risk, const double new_total_congestion_risk,
                        const double old_touched_overflow, const double new_touched_overflow,
                        const double old_route_cost, const double new_route_cost, GSRRouteStats& route_stats, std::string* accept_reason)
{
  (void) gsr_net;

  if (new_total_overflow + RT_ERROR < old_total_overflow) {
    if (!passRerouteShapeGuard(old_segment_list, new_segment_list, old_total_overflow, new_total_overflow, old_route_cost, new_route_cost)) {
      route_stats.shape_guard_reject_num++;
      if (accept_reason != nullptr) {
        *accept_reason = "reject_shape_guard";
      }
      return false;
    }
    route_stats.total_overflow_accept_num++;
    if (accept_reason != nullptr) {
      *accept_reason = "total_overflow";
    }
    return true;
  }
  if (!RTUTIL.equalDoubleByError(new_total_overflow, old_total_overflow, RT_ERROR)) {
    if (accept_reason != nullptr) {
      *accept_reason = "reject_total_overflow_worse";
    }
    return false;
  }
  if (new_touched_overflow + RT_ERROR < old_touched_overflow) {
    if (!passRerouteShapeGuard(old_segment_list, new_segment_list, old_total_overflow, new_total_overflow, old_route_cost, new_route_cost)) {
      route_stats.shape_guard_reject_num++;
      if (accept_reason != nullptr) {
        *accept_reason = "reject_shape_guard";
      }
      return false;
    }
    route_stats.touched_overflow_accept_num++;
    if (accept_reason != nullptr) {
      *accept_reason = "touched_overflow";
    }
    return true;
  }
  if (!RTUTIL.equalDoubleByError(new_touched_overflow, old_touched_overflow, RT_ERROR)) {
    if (accept_reason != nullptr) {
      *accept_reason = "reject_touched_overflow_worse";
    }
    return false;
  }
  if (new_total_congestion_risk + RT_ERROR < old_total_congestion_risk) {
    if (!passRerouteShapeGuard(old_segment_list, new_segment_list, old_total_overflow, new_total_overflow, old_route_cost, new_route_cost)) {
      route_stats.shape_guard_reject_num++;
      if (accept_reason != nullptr) {
        *accept_reason = "reject_shape_guard";
      }
      return false;
    }
    route_stats.congestion_risk_accept_num++;
    if (accept_reason != nullptr) {
      *accept_reason = "congestion_risk";
    }
    return true;
  }
  if (!RTUTIL.equalDoubleByError(new_total_congestion_risk, old_total_congestion_risk, RT_ERROR)) {
    if (accept_reason != nullptr) {
      *accept_reason = "reject_congestion_risk_worse";
    }
    return false;
  }
  if (new_route_cost + RT_ERROR < old_route_cost) {
    if (!passRerouteShapeGuard(old_segment_list, new_segment_list, old_total_overflow, new_total_overflow, old_route_cost, new_route_cost)) {
      route_stats.shape_guard_reject_num++;
      if (accept_reason != nullptr) {
        *accept_reason = "reject_shape_guard";
      }
      return false;
    }
    route_stats.route_cost_accept_num++;
    if (accept_reason != nullptr) {
      *accept_reason = "route_cost";
    }
    return true;
  }
  if (!RTUTIL.equalDoubleByError(new_route_cost, old_route_cost, RT_ERROR)) {
    if (accept_reason != nullptr) {
      *accept_reason = "reject_route_cost_worse";
    }
    return false;
  }

  int64_t old_length = 0;
  int64_t new_length = 0;
  int32_t old_via_num = 0;
  int32_t new_via_num = 0;
  for (const Segment<LayerCoord>& segment : old_segment_list) {
    old_length += getSegmentPlanarLength(segment);
    if (segment.get_first().get_layer_idx() != segment.get_second().get_layer_idx()) {
      old_via_num++;
    }
  }
  for (const Segment<LayerCoord>& segment : new_segment_list) {
    new_length += getSegmentPlanarLength(segment);
    if (segment.get_first().get_layer_idx() != segment.get_second().get_layer_idx()) {
      new_via_num++;
    }
  }
  if (new_via_num != old_via_num) {
    bool accept = new_via_num < old_via_num;
    if (accept && !passRerouteShapeGuard(old_segment_list, new_segment_list, old_total_overflow, new_total_overflow, old_route_cost, new_route_cost)) {
      route_stats.shape_guard_reject_num++;
      if (accept_reason != nullptr) {
        *accept_reason = "reject_shape_guard";
      }
      return false;
    }
    if (accept_reason != nullptr) {
      *accept_reason = accept ? "via_num" : "reject_via_num_worse";
    }
    return accept;
  }
  bool accept = new_length <= old_length;
  if (accept && !passRerouteShapeGuard(old_segment_list, new_segment_list, old_total_overflow, new_total_overflow, old_route_cost, new_route_cost)) {
    route_stats.shape_guard_reject_num++;
    if (accept_reason != nullptr) {
      *accept_reason = "reject_shape_guard";
    }
    return false;
  }
  if (accept_reason != nullptr) {
    *accept_reason = accept ? "wire_length" : "reject_wire_length_worse";
  }
  return accept;
}

// shared helpers and diagnostics

std::vector<int32_t> GlobalSpatialRouter::getCandidateLayerList(const GSRComParam& gsr_com_param, const bool prefer_h)
{
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::vector<int32_t> candidate_layer_list;
  for (int32_t layer_idx = gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
    if (routing_layer_list[layer_idx].isPreferH() == prefer_h) {
      candidate_layer_list.push_back(layer_idx);
    }
  }
  return candidate_layer_list;
}

bool GlobalSpatialRouter::isRoutePreferredOnly(GSRModel& gsr_model, const std::vector<Segment<LayerCoord>>& routing_segment_list)
{
  for (const Segment<LayerCoord>& routing_segment : routing_segment_list) {
    Segment<LayerCoord> segment = routing_segment;
    if (!isValidSegment(gsr_model, segment)) {
      return false;
    }
  }
  return true;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::sanitizeRouteSegmentList(GSRModel& gsr_model, GSRNet& gsr_net,
                                                                               const std::vector<Segment<LayerCoord>>& segment_list,
                                                                               GSRRouteStats& route_stats)
{
  route_stats.route_sanitize_num++;
  if (segment_list.empty()) {
    route_stats.route_sanitize_fail_num++;
    return {};
  }

  std::vector<Segment<LayerCoord>> working_segment_list = segment_list;
  working_segment_list = getValidUniqueSegmentList(gsr_model, working_segment_list, route_stats);
  if (working_segment_list.empty() || !isRoutePreferredOnly(gsr_model, working_segment_list)
      || !isRouteConnected(gsr_net, working_segment_list)) {
    route_stats.route_sanitize_fail_num++;
    return {};
  }

  std::vector<LayerCoord> candidate_root_coord_list;
  std::map<LayerCoord, std::set<int32_t>, CmpLayerCoordByXASC> key_coord_pin_map;
  for (size_t pin_idx = 0; pin_idx < gsr_net.get_gsr_pin_list().size(); pin_idx++) {
    LayerCoord access_coord = gsr_net.get_gsr_pin_list()[pin_idx].get_access_coord();
    if (!gsr_model.get_gsr_grid_graph().isInside(access_coord)) {
      continue;
    }
    candidate_root_coord_list.push_back(access_coord);
    key_coord_pin_map[access_coord].insert(static_cast<int32_t>(pin_idx));
  }
  if (candidate_root_coord_list.size() < 2 || key_coord_pin_map.size() < 2) {
    return working_segment_list;
  }

  std::vector<Segment<LayerCoord>> tree_input_segment_list = working_segment_list;
  MTree<LayerCoord> coord_tree = RTUTIL.getTreeByFullFlow(candidate_root_coord_list, tree_input_segment_list, key_coord_pin_map);

  std::vector<Segment<LayerCoord>> sanitized_segment_list;
  for (Segment<TNode<LayerCoord>*>& coord_segment : RTUTIL.getSegListByTree(coord_tree)) {
    sanitized_segment_list.emplace_back(coord_segment.get_first()->value(), coord_segment.get_second()->value());
  }
  sanitized_segment_list = getValidUniqueSegmentList(gsr_model, sanitized_segment_list, route_stats);
  if (sanitized_segment_list.empty() || !isRoutePreferredOnly(gsr_model, sanitized_segment_list)
      || !isRouteConnected(gsr_net, sanitized_segment_list)) {
    route_stats.route_sanitize_fail_num++;
    return {};
  }

  if (makeSegmentKeySet(working_segment_list) != makeSegmentKeySet(sanitized_segment_list)) {
    route_stats.route_sanitize_changed_num++;
  }
  return sanitized_segment_list;
}

bool GlobalSpatialRouter::hasPlanarProjectionCycle(GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& segment_list,
                                                   GSRRouteStats& route_stats)
{
  route_stats.planar_cycle_check_num++;

  std::vector<LayerCoord> key_coord_list;
  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    key_coord_list.push_back(gsr_pin.get_access_coord());
  }
  GSRPlanarProjection projection = buildPlanarProjection(key_coord_list, segment_list);

  if (projection.node_key_set.empty()) {
    return false;
  }

  std::map<PlanarKey, std::vector<PlanarKey>> adjacency_map;
  for (Segment<PlanarCoord>& split_segment : projection.split_segment_list) {
    PlanarKey first_key = makePlanarKey(split_segment.get_first());
    PlanarKey second_key = makePlanarKey(split_segment.get_second());
    adjacency_map[first_key].push_back(second_key);
    adjacency_map[second_key].push_back(first_key);
  }

  int32_t component_num = 0;
  std::set<PlanarKey> visited_key_set;
  for (PlanarKey node_key : projection.node_key_set) {
    if (RTUTIL.exist(visited_key_set, node_key)) {
      continue;
    }
    component_num++;
    std::queue<PlanarKey> node_queue;
    node_queue.push(node_key);
    visited_key_set.insert(node_key);
    while (!node_queue.empty()) {
      PlanarKey curr_key = node_queue.front();
      node_queue.pop();
      for (PlanarKey neighbor_key : adjacency_map[curr_key]) {
        if (visited_key_set.insert(neighbor_key).second) {
          node_queue.push(neighbor_key);
        }
      }
    }
  }

  bool has_cycle = static_cast<int32_t>(projection.split_segment_list.size()) - static_cast<int32_t>(projection.node_key_set.size()) + component_num > 0;
  if (has_cycle) {
    route_stats.planar_cycle_detect_num++;
  }
  return has_cycle;
}

bool GlobalSpatialRouter::hasPlanarProjectionOverlap(GSRNet& gsr_net, const std::vector<Segment<LayerCoord>>& segment_list,
                                                     GSRRouteStats& route_stats)
{
  route_stats.planar_overlap_check_num++;

  std::vector<LayerCoord> key_coord_list;
  for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    key_coord_list.push_back(gsr_pin.get_access_coord());
  }
  GSRPlanarProjection projection = buildPlanarProjection(key_coord_list, segment_list);
  for (auto& [edge_key, segment_key_set] : projection.edge_segment_key_set_map) {
    if (segment_key_set.size() > 1) {
      route_stats.planar_overlap_detect_num++;
      return true;
    }
  }
  return false;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::rebuildRouteByPlanarProjectionTree(GSRModel& gsr_model, GSRNet& gsr_net,
                                                                                         const std::vector<Segment<LayerCoord>>& segment_list,
                                                                                         const bool require_no_cycle,
                                                                                         const bool require_no_overlap,
                                                                                         GSRRouteStats& route_stats)
{
  std::vector<PlanarCoord> candidate_root_coord_list;
  std::map<PlanarCoord, std::set<int32_t>, CmpPlanarCoordByXASC> key_coord_pin_map;
  std::vector<LayerCoord> key_coord_list;
  for (size_t pin_idx = 0; pin_idx < gsr_net.get_gsr_pin_list().size(); pin_idx++) {
    LayerCoord access_coord = gsr_net.get_gsr_pin_list()[pin_idx].get_access_coord();
    if (!gsr_model.get_gsr_grid_graph().isInside(access_coord)) {
      continue;
    }
    key_coord_list.push_back(access_coord);
    PlanarCoord planar_coord = access_coord.get_planar_coord();
    candidate_root_coord_list.push_back(planar_coord);
    key_coord_pin_map[planar_coord].insert(static_cast<int32_t>(pin_idx));
  }
  if (candidate_root_coord_list.size() < 2 || key_coord_pin_map.size() < 2) {
    return {};
  }

  GSRPlanarProjection projection = buildPlanarProjection(key_coord_list, segment_list);
  std::vector<Segment<PlanarCoord>> planar_segment_list = projection.split_segment_list;
  if (planar_segment_list.empty()) {
    return {};
  }

  if (!RTUTIL.passCheckingConnectivity(candidate_root_coord_list, planar_segment_list)) {
    return {};
  }

  MTree<PlanarCoord> planar_tree = RTUTIL.getTreeByFullFlow(candidate_root_coord_list, planar_segment_list, key_coord_pin_map);
  std::vector<Segment<PlanarCoord>> planar_topo_list;
  for (Segment<TNode<PlanarCoord>*>& coord_segment : RTUTIL.getSegListByTree(planar_tree)) {
    planar_topo_list.emplace_back(coord_segment.get_first()->value(), coord_segment.get_second()->value());
  }
  if (planar_topo_list.empty()) {
    return {};
  }

  GSRTree repaired_tree = buildGSRTree(gsr_model, gsr_net, planar_topo_list, route_stats);
  std::vector<Segment<LayerCoord>> repaired_segment_list = refineTreeByPatternLayerDP(gsr_model, gsr_net, repaired_tree, nullptr, false, route_stats);
  repaired_segment_list = getValidUniqueSegmentList(gsr_model, repaired_segment_list, route_stats);
  if (repaired_segment_list.empty() || !isRoutePreferredOnly(gsr_model, repaired_segment_list)
      || !isRouteConnected(gsr_net, repaired_segment_list)) {
    return {};
  }
  if (require_no_cycle && hasPlanarProjectionCycle(gsr_net, repaired_segment_list, route_stats)) {
    return {};
  }
  if (require_no_overlap && hasPlanarProjectionOverlap(gsr_net, repaired_segment_list, route_stats)) {
    return {};
  }

  return repaired_segment_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::repairPlanarProjectionCycle(GSRModel& gsr_model, GSRNet& gsr_net,
                                                                                  const std::vector<Segment<LayerCoord>>& segment_list,
                                                                                  GSRRouteStats& route_stats)
{
  std::vector<Segment<LayerCoord>> repaired_segment_list
      = rebuildRouteByPlanarProjectionTree(gsr_model, gsr_net, segment_list, true, false, route_stats);
  if (repaired_segment_list.empty()) {
    route_stats.planar_cycle_repair_fail_num++;
    return {};
  }
  route_stats.planar_cycle_repair_num++;
  return repaired_segment_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::repairPlanarProjectionOverlap(GSRModel& gsr_model, GSRNet& gsr_net,
                                                                                    const std::vector<Segment<LayerCoord>>& segment_list,
                                                                                    GSRRouteStats& route_stats)
{
  std::vector<Segment<LayerCoord>> repaired_segment_list
      = rebuildRouteByPlanarProjectionTree(gsr_model, gsr_net, segment_list, true, true, route_stats);
  if (repaired_segment_list.empty()) {
    route_stats.planar_overlap_repair_fail_num++;
    return {};
  }
  route_stats.planar_overlap_repair_num++;
  return repaired_segment_list;
}

std::vector<Segment<LayerCoord>> GlobalSpatialRouter::buildPatternRoute(const PlanarCoord& first_coord, const PlanarCoord& second_coord, const bool h_first,
                                                       const int32_t h_layer_idx, const int32_t v_layer_idx)
{
  std::vector<Segment<LayerCoord>> segment_list;
  if (first_coord == second_coord) {
    return segment_list;
  }
  PlanarCoord bend_coord = h_first ? PlanarCoord(second_coord.get_x(), first_coord.get_y()) : PlanarCoord(first_coord.get_x(), second_coord.get_y());
  if (h_first) {
    if (first_coord.get_x() != second_coord.get_x()) {
      segment_list.emplace_back(makeLayerCoord(first_coord, h_layer_idx), makeLayerCoord(bend_coord, h_layer_idx));
    }
    if (first_coord.get_y() != second_coord.get_y()) {
      addViaChain(segment_list, bend_coord, h_layer_idx, v_layer_idx);
      segment_list.emplace_back(makeLayerCoord(bend_coord, v_layer_idx), makeLayerCoord(second_coord, v_layer_idx));
    }
  } else {
    if (first_coord.get_y() != second_coord.get_y()) {
      segment_list.emplace_back(makeLayerCoord(first_coord, v_layer_idx), makeLayerCoord(bend_coord, v_layer_idx));
    }
    if (first_coord.get_x() != second_coord.get_x()) {
      addViaChain(segment_list, bend_coord, v_layer_idx, h_layer_idx);
      segment_list.emplace_back(makeLayerCoord(bend_coord, h_layer_idx), makeLayerCoord(second_coord, h_layer_idx));
    }
  }
  return segment_list;
}

int64_t GlobalSpatialRouter::getGSRNetHPWL(const GSRNet& gsr_net)
{
  if (gsr_net.get_gsr_pin_list().empty()) {
    return 0;
  }
  int32_t ll_x = std::numeric_limits<int32_t>::max();
  int32_t ll_y = std::numeric_limits<int32_t>::max();
  int32_t ur_x = std::numeric_limits<int32_t>::min();
  int32_t ur_y = std::numeric_limits<int32_t>::min();
  for (const GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
    const LayerCoord& access_coord = gsr_pin.get_access_coord();
    ll_x = std::min(ll_x, access_coord.get_x());
    ll_y = std::min(ll_y, access_coord.get_y());
    ur_x = std::max(ur_x, access_coord.get_x());
    ur_y = std::max(ur_y, access_coord.get_y());
  }
  return static_cast<int64_t>(ur_x - ll_x) + static_cast<int64_t>(ur_y - ll_y);
}

std::map<int32_t, GlobalSpatialRouter::GSRLayerUsage> GlobalSpatialRouter::getLayerUsageMap(std::vector<RoutingLayer>& routing_layer_list, GSRGridGraph& gsr_grid_graph,
                                                         GSRRouteStats& route_stats)
{
  (void) gsr_grid_graph;
  (void) route_stats;

  std::map<int32_t, GSRLayerUsage> layer_usage_map;
  for (RoutingLayer& routing_layer : routing_layer_list) {
    layer_usage_map[routing_layer.get_layer_idx()];
  }
  Die& die = RTDM.getDatabase().get_die();
  std::map<int32_t, std::set<Segment<LayerCoord>*>> net_global_result_map = RTDM.getNetGlobalResultMap(die);
  for (auto& [net_idx, segment_set] : net_global_result_map) {
    (void) net_idx;
    for (Segment<LayerCoord>* segment : segment_set) {
      LayerCoord& first_coord = segment->get_first();
      LayerCoord& second_coord = segment->get_second();
      if (first_coord.get_layer_idx() == second_coord.get_layer_idx()) {
        GSRLayerUsage& layer_usage = layer_usage_map[first_coord.get_layer_idx()];
        layer_usage.same_layer_segment_num++;
        layer_usage.wire_grid_length += getSegmentPlanarLength(*segment);
      } else {
        layer_usage_map[first_coord.get_layer_idx()].via_touch_num++;
        layer_usage_map[second_coord.get_layer_idx()].via_touch_num++;
      }
    }
  }
  return layer_usage_map;
}

void GlobalSpatialRouter::updateHandoffStats(GSRModel& gsr_model, GSRRouteStats& route_stats)
{
  (void) gsr_model;

  route_stats.ta_visible_net_num = 0;
  route_stats.via_only_net_num = 0;
  route_stats.same_layer_guide_segment_num = 0;
  route_stats.guide_endpoint_on_track_num = 0;
  route_stats.guide_endpoint_off_track_num = 0;

  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  for (GSRNet& gsr_net : gsr_model.get_gsr_net_list()) {
    bool has_same_layer_segment = false;
    for (Segment<LayerCoord>& segment : gsr_net.get_routing_segment_list()) {
      LayerCoord& first_coord = segment.get_first();
      LayerCoord& second_coord = segment.get_second();
      if (first_coord.get_layer_idx() != second_coord.get_layer_idx()) {
        continue;
      }
      has_same_layer_segment = true;
      route_stats.same_layer_guide_segment_num++;
      int32_t layer_idx = first_coord.get_layer_idx();
      if (layer_idx < 0 || static_cast<int32_t>(routing_layer_list.size()) <= layer_idx) {
        route_stats.guide_endpoint_off_track_num += 2;
        continue;
      }
      RoutingLayer& routing_layer = routing_layer_list[layer_idx];
      for (LayerCoord* coord : {&first_coord, &second_coord}) {
        PlanarCoord real_mid_coord = RTUTIL.getRealRectByGCell(coord->get_planar_coord(), gcell_axis).getMidPoint();
        if (RTUTIL.existTrackGrid(real_mid_coord, routing_layer.get_track_axis())) {
          route_stats.guide_endpoint_on_track_num++;
        } else {
          route_stats.guide_endpoint_off_track_num++;
        }
      }
    }
    if (has_same_layer_segment) {
      route_stats.ta_visible_net_num++;
    } else if (!gsr_net.get_routing_segment_list().empty()) {
      route_stats.via_only_net_num++;
    }
  }
}

void GlobalSpatialRouter::initRerouteDiagnostics()
{
  _reroute_timing_buffer.clear();
  _reroute_attempt_buffer.str("");
  _reroute_attempt_buffer.clear();
  _reroute_attempt_buffer_path.clear();
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  std::string& gsr_temp_directory_path = RTDM.getConfig().gsr_temp_directory_path;
  std::ofstream* summary_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(gsr_temp_directory_path, "reroute_iter_summary.csv"));
  RTUTIL.pushStream(summary_csv_file,
                    "iter,phase,total_overflow,total_congestion_risk,delta_overflow,delta_congestion_risk,overflow_cell_num,overflow_net_num,"
                    "hotspot_cell_num,task_coord_num,stage2_task_num,stage2_accept_num,stage3_task_num,stage3_accept_num\n");
  RTUTIL.closeFileStream(summary_csv_file);

  std::ofstream* timing_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(gsr_temp_directory_path, "reroute_step_timing.csv"));
  RTUTIL.pushStream(timing_csv_file, "iter,stage,step,time_ms,count,avg_ms\n");
  RTUTIL.closeFileStream(timing_csv_file);
}

void GlobalSpatialRouter::initRerouteAttemptCSV(const int32_t iter, const int32_t stage)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  flushRerouteAttemptCSV();
  _reroute_attempt_buffer.str("");
  _reroute_attempt_buffer.clear();
  _reroute_attempt_buffer_path = RTUTIL.getString(RTDM.getConfig().gsr_temp_directory_path, "reroute_attempt_iter_", iter, "_stage", stage, ".csv");
  std::ofstream* attempt_csv_file = RTUTIL.getOutputFileStream(_reroute_attempt_buffer_path);
  RTUTIL.pushStream(attempt_csv_file,
                    "iter,stage,net_idx,result,accept_reason,old_total_overflow,new_total_overflow,delta_total_overflow,old_touched_overflow,"
                    "new_touched_overflow,delta_touched_overflow,old_route_overflow,new_route_overflow,old_total_congestion_risk,"
                    "new_total_congestion_risk,delta_total_congestion_risk,old_route_congestion_risk,new_route_congestion_risk,old_route_cost,"
                    "new_route_cost,old_segment_num,new_segment_num,runtime_ms\n");
  RTUTIL.closeFileStream(attempt_csv_file);
}

void GlobalSpatialRouter::flushRerouteTimingCSV()
{
  if (!RTDM.getConfig().output_inter_result || _reroute_timing_buffer.empty()) {
    return;
  }
  std::ofstream timing_csv_file(RTUTIL.getString(RTDM.getConfig().gsr_temp_directory_path, "reroute_step_timing.csv"), std::ios::app);
  for (GSRRerouteTimingRecord& timing_record : _reroute_timing_buffer) {
    int32_t safe_count = std::max(1, timing_record.count);
    RTUTIL.pushStream(timing_csv_file, timing_record.iter, ",", timing_record.stage, ",", timing_record.step, ",", timing_record.time_ms, ",",
                      timing_record.count, ",", timing_record.time_ms / safe_count, "\n");
  }
  _reroute_timing_buffer.clear();
}

void GlobalSpatialRouter::appendRerouteTiming(const int32_t iter, const int32_t stage, const std::string& step, const double time_ms, const int32_t count)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  _reroute_timing_buffer.push_back(GSRRerouteTimingRecord{iter, stage, step, time_ms, count});
}

void GlobalSpatialRouter::appendRerouteIterSummary(GSRModel& gsr_model, GSRCongestionView* congestion_view, const int32_t iter, const std::string& phase,
                                  const double prev_total_overflow, const double prev_total_congestion_risk, const int32_t stage2_task_num,
                                  const int32_t stage2_accept_num, const int32_t stage3_task_num, const int32_t stage3_accept_num)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  int32_t overflow_cell_num = congestion_view == nullptr ? -1 : congestion_view->overflow_cell_num;
  int32_t hotspot_cell_num = congestion_view == nullptr ? -1 : congestion_view->hotspot_cell_num;
  int32_t task_coord_num = congestion_view == nullptr ? -1 : static_cast<int32_t>(congestion_view->task_coord_list.size());
  int32_t overflow_net_num = -1;
  if (congestion_view != nullptr) {
    std::set<int32_t> overflow_net_idx_set;
    for (std::set<int32_t>& overflow_net_set : gsr_model.get_gsr_grid_graph().getOverflowNetSetList()) {
      overflow_net_idx_set.insert(overflow_net_set.begin(), overflow_net_set.end());
    }
    overflow_net_num = static_cast<int32_t>(overflow_net_idx_set.size());
  }
  std::ofstream summary_csv_file(RTUTIL.getString(RTDM.getConfig().gsr_temp_directory_path, "reroute_iter_summary.csv"), std::ios::app);
  RTUTIL.pushStream(summary_csv_file, iter, ",", phase, ",", gsr_model.get_total_overflow(), ",", gsr_model.get_total_congestion_risk(), ",",
                    gsr_model.get_total_overflow() - prev_total_overflow, ",",
                    gsr_model.get_total_congestion_risk() - prev_total_congestion_risk, ",", overflow_cell_num, ",", overflow_net_num, ",",
                    hotspot_cell_num, ",", task_coord_num, ",", stage2_task_num, ",", stage2_accept_num, ",", stage3_task_num, ",",
                    stage3_accept_num, "\n");
}

void GlobalSpatialRouter::outputOverflowHotspotCSV(GSRModel& gsr_model, GSRCongestionView& congestion_view, const int32_t iter, const std::string& phase)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  constexpr int32_t kMaxHotspotRowNum = 5000;
  std::ofstream* hotspot_csv_file
      = RTUTIL.getOutputFileStream(RTUTIL.getString(RTDM.getConfig().gsr_temp_directory_path, "overflow_hotspot_iter_", iter, "_", phase, ".csv"));
  RTUTIL.pushStream(hotspot_csv_file,
                    "iter,phase,rank,layer_idx,layer_name,x,y,resource,demand,supply,overflow,usage_ratio,h_overflow,v_overflow,"
                    "internal_overflow,h_risk,v_risk,hotspot,net_count,net_list\n");

  struct HotspotRow
  {
    int32_t layer_idx = -1;
    std::string layer_name;
    int32_t x = -1;
    int32_t y = -1;
    std::string resource;
    double demand = 0;
    double supply = 0;
    double overflow = 0;
    double usage_ratio = 0;
    double h_overflow = 0;
    double v_overflow = 0;
    double internal_overflow = 0;
    double h_risk = 0;
    double v_risk = 0;
    double hotspot = 0;
    std::set<int32_t> net_set;
  };

  std::vector<HotspotRow> hotspot_row_list;
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  for (RoutingLayer& routing_layer : routing_layer_list) {
    int32_t layer_idx = routing_layer.get_layer_idx();
    if (!gsr_grid_graph.isLegalLayer(layer_idx)) {
      continue;
    }
    GridMap<GSRNode>& gsr_node_map = gsr_grid_graph.get_layer_node_map()[layer_idx];
    for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
      for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
        GSRNode& gsr_node = gsr_node_map[x][y];
        double h_overflow = congestion_view.h_overflow_map.isInside(x, y) ? congestion_view.h_overflow_map[x][y] : 0;
        double v_overflow = congestion_view.v_overflow_map.isInside(x, y) ? congestion_view.v_overflow_map[x][y] : 0;
        double internal_overflow = congestion_view.internal_overflow_map.isInside(x, y) ? congestion_view.internal_overflow_map[x][y] : 0;
        double h_risk = congestion_view.h_risk_map.isInside(x, y) ? congestion_view.h_risk_map[x][y] : 0;
        double v_risk = congestion_view.v_risk_map.isInside(x, y) ? congestion_view.v_risk_map[x][y] : 0;
        double hotspot = congestion_view.hotspot_map.isInside(x, y) ? congestion_view.hotspot_map[x][y] : 0;
        for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
          double demand = gsr_node.getBoundaryDemand(orient);
          double supply = gsr_node.getSupply(orient);
          double overflow = std::max(0.0, demand - supply);
          if (overflow <= 0 && hotspot <= gsr_model.get_gsr_com_param().get_congestion_risk_threshold()) {
            continue;
          }
          HotspotRow hotspot_row;
          hotspot_row.layer_idx = layer_idx;
          hotspot_row.layer_name = routing_layer.get_layer_name();
          hotspot_row.x = x;
          hotspot_row.y = y;
          hotspot_row.resource = GetOrientationName()(orient);
          hotspot_row.demand = demand;
          hotspot_row.supply = supply;
          hotspot_row.overflow = overflow;
          hotspot_row.usage_ratio = supply > 0 ? demand / supply : (demand > 0 ? 1.0e6 : 0);
          hotspot_row.h_overflow = h_overflow;
          hotspot_row.v_overflow = v_overflow;
          hotspot_row.internal_overflow = internal_overflow;
          hotspot_row.h_risk = h_risk;
          hotspot_row.v_risk = v_risk;
          hotspot_row.hotspot = hotspot;
          if (RTUTIL.exist(gsr_node.get_orient_net_map(), orient)) {
            hotspot_row.net_set = gsr_node.get_orient_net_map()[orient];
          }
          hotspot_row_list.push_back(hotspot_row);
        }
        if (internal_overflow > 0) {
          HotspotRow hotspot_row;
          hotspot_row.layer_idx = layer_idx;
          hotspot_row.layer_name = routing_layer.get_layer_name();
          hotspot_row.x = x;
          hotspot_row.y = y;
          hotspot_row.resource = "internal";
          hotspot_row.demand = gsr_node.getInternalDemand();
          hotspot_row.supply = gsr_node.getInternalSupply();
          hotspot_row.overflow = internal_overflow;
          hotspot_row.usage_ratio = hotspot_row.supply > 0 ? hotspot_row.demand / hotspot_row.supply : (hotspot_row.demand > 0 ? 1.0e6 : 0);
          hotspot_row.h_overflow = h_overflow;
          hotspot_row.v_overflow = v_overflow;
          hotspot_row.internal_overflow = internal_overflow;
          hotspot_row.h_risk = h_risk;
          hotspot_row.v_risk = v_risk;
          hotspot_row.hotspot = hotspot;
          for (auto& [net_idx, orient_set] : gsr_node.get_net_orient_map()) {
            (void) orient_set;
            hotspot_row.net_set.insert(net_idx);
          }
          hotspot_row_list.push_back(hotspot_row);
        }
      }
    }
  }
  std::sort(hotspot_row_list.begin(), hotspot_row_list.end(), [](const HotspotRow& a, const HotspotRow& b) {
    if (!RTUTIL.equalDoubleByError(a.overflow, b.overflow, RT_ERROR)) {
      return a.overflow > b.overflow;
    }
    if (!RTUTIL.equalDoubleByError(a.hotspot, b.hotspot, RT_ERROR)) {
      return a.hotspot > b.hotspot;
    }
    if (a.layer_idx != b.layer_idx) {
      return a.layer_idx < b.layer_idx;
    }
    return std::tie(a.x, a.y, a.resource) < std::tie(b.x, b.y, b.resource);
  });
  int32_t row_num = std::min(kMaxHotspotRowNum, static_cast<int32_t>(hotspot_row_list.size()));
  for (int32_t row_idx = 0; row_idx < row_num; row_idx++) {
    HotspotRow& row = hotspot_row_list[row_idx];
    RTUTIL.pushStream(hotspot_csv_file, iter, ",", phase, ",", row_idx, ",", row.layer_idx, ",", row.layer_name, ",", row.x, ",", row.y, ",",
                      row.resource, ",", row.demand, ",", row.supply, ",", row.overflow, ",", row.usage_ratio, ",", row.h_overflow, ",",
                      row.v_overflow, ",", row.internal_overflow, ",", row.h_risk, ",", row.v_risk, ",", row.hotspot, ",", row.net_set.size(),
                      ",", joinNetSet(row.net_set), "\n");
  }
  RTUTIL.closeFileStream(hotspot_csv_file);
}

void GlobalSpatialRouter::outputRerouteTaskCSV(const std::vector<GSRRerouteTask>& task_list, const std::set<int32_t>& selected_net_idx_set, const int32_t iter,
                              const int32_t stage)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  constexpr int32_t kMaxTaskRowNum = 20000;
  std::ofstream* task_csv_file
      = RTUTIL.getOutputFileStream(RTUTIL.getString(RTDM.getConfig().gsr_temp_directory_path, "reroute_task_iter_", iter, "_stage", stage, ".csv"));
  RTUTIL.pushStream(task_csv_file,
                    "iter,stage,rank,net_idx,selected,routed_times,total_score,overflow_score,risk_score,near_full_score,hotspot_score,"
                    "overflow_touch_num,hotspot_touch_num,overflow_coord_num,hpwl\n");
  int32_t row_num = std::min(kMaxTaskRowNum, static_cast<int32_t>(task_list.size()));
  for (int32_t task_idx = 0; task_idx < row_num; task_idx++) {
    const GSRRerouteTask& task = task_list[task_idx];
    int32_t net_idx = task.gsr_net == nullptr ? -1 : task.gsr_net->get_net_idx();
    int64_t hpwl = task.gsr_net == nullptr ? 0 : getGSRNetHPWL(*task.gsr_net);
    RTUTIL.pushStream(task_csv_file, iter, ",", stage, ",", task_idx, ",", net_idx, ",", (RTUTIL.exist(selected_net_idx_set, net_idx) ? 1 : 0),
                      ",", task.routed_times, ",", task.total_score, ",", task.overflow_score, ",", task.risk_score, ",", task.near_full_score,
                      ",", task.hotspot_score, ",", task.overflow_touch_num, ",", task.hotspot_touch_num, ",", task.overflow_coord_set.size(),
                      ",", hpwl, "\n");
  }
  RTUTIL.closeFileStream(task_csv_file);
}

void GlobalSpatialRouter::flushRerouteAttemptCSV()
{
  if (!RTDM.getConfig().output_inter_result || _reroute_attempt_buffer_path.empty()) {
    return;
  }
  std::string buffered_record = _reroute_attempt_buffer.str();
  if (buffered_record.empty()) {
    return;
  }
  std::ofstream attempt_csv_file(_reroute_attempt_buffer_path, std::ios::app);
  attempt_csv_file << buffered_record;
  _reroute_attempt_buffer.str("");
  _reroute_attempt_buffer.clear();
}

void GlobalSpatialRouter::outputRerouteAttemptCSV(const GSRRerouteAttemptRecord& attempt_record)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }
  RTUTIL.pushStream(_reroute_attempt_buffer, attempt_record.iter, ",", attempt_record.stage, ",", attempt_record.net_idx, ",", attempt_record.result, ",",
                    attempt_record.accept_reason, ",", attempt_record.old_total_overflow, ",", attempt_record.new_total_overflow, ",",
                    attempt_record.new_total_overflow - attempt_record.old_total_overflow, ",", attempt_record.old_touched_overflow, ",",
                    attempt_record.new_touched_overflow, ",", attempt_record.new_touched_overflow - attempt_record.old_touched_overflow, ",",
                    attempt_record.old_route_overflow, ",", attempt_record.new_route_overflow, ",", attempt_record.old_total_congestion_risk, ",",
                    attempt_record.new_total_congestion_risk, ",",
                    attempt_record.new_total_congestion_risk - attempt_record.old_total_congestion_risk, ",",
                    attempt_record.old_route_congestion_risk, ",", attempt_record.new_route_congestion_risk, ",", attempt_record.old_route_cost, ",",
                    attempt_record.new_route_cost, ",", attempt_record.old_segment_num, ",", attempt_record.new_segment_num, ",",
                    attempt_record.runtime_ms, "\n");
}

void GlobalSpatialRouter::outputStage1RouteOrderCSV(const std::vector<GSRNet>& gsr_net_list)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }

  std::ofstream* route_order_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(RTDM.getConfig().gsr_temp_directory_path, "stage1_route_order.csv"));
  RTUTIL.pushStream(route_order_csv_file,
                    "order,net_idx,net_name,is_clock,pin_num,bbox_total,x_span,y_span,aspect_ratio,layer_span,avg_access_layer,hpwl\n");
  for (int32_t order = 0; order < static_cast<int32_t>(gsr_net_list.size()); order++) {
    const GSRNet& gsr_net = gsr_net_list[order];
    GSRStage1RouteOrderKey order_key = buildStage1RouteOrderKey(gsr_net);
    RTUTIL.pushStream(route_order_csv_file, order, ",", order_key.net_idx, ",", gsr_net.get_net_name(), ",", (order_key.is_clock ? 1 : 0),
                      ",", order_key.pin_num, ",", order_key.bbox_total, ",", order_key.x_span, ",", order_key.y_span, ",",
                      order_key.aspect_ratio, ",", order_key.layer_span, ",", order_key.avg_access_layer, ",", order_key.hpwl, "\n");
  }
  RTUTIL.closeFileStream(route_order_csv_file);
}

void GlobalSpatialRouter::outputGuide(GSRModel& gsr_model)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }

  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& gsr_temp_directory_path = RTDM.getConfig().gsr_temp_directory_path;
  std::map<int32_t, int32_t>& net_idx_to_gsr_net_idx_map = gsr_model.get_net_idx_to_gsr_net_idx_map();
  std::vector<GSRNet>& gsr_net_list = gsr_model.get_gsr_net_list();

  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::ofstream* guide_file_stream = RTUTIL.getOutputFileStream(RTUTIL.getString(gsr_temp_directory_path, "route.guide"));
  if (guide_file_stream == nullptr) {
    return;
  }
  RTUTIL.pushStream(guide_file_stream, "guide net_name\n");
  RTUTIL.pushStream(guide_file_stream, "pin grid_x grid_y real_x real_y layer energy name\n");
  RTUTIL.pushStream(guide_file_stream, "wire grid1_x grid1_y grid2_x grid2_y real1_x real1_y real2_x real2_y layer\n");
  RTUTIL.pushStream(guide_file_stream, "via grid_x grid_y real_x real_y layer1 layer2\n");

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    if (!RTUTIL.exist(net_idx_to_gsr_net_idx_map, net_idx)) {
      continue;
    }
    GSRNet& gsr_net = gsr_net_list[net_idx_to_gsr_net_idx_map[net_idx]];
    RTUTIL.pushStream(guide_file_stream, "guide ", gsr_net.get_net_name(), "\n");

    Net* origin_net = gsr_net.get_origin_net();
    for (GSRPin& gsr_pin : gsr_net.get_gsr_pin_list()) {
      LayerCoord access_coord = gsr_pin.get_access_coord();
      double grid_x = access_coord.get_x();
      double grid_y = access_coord.get_y();
      PlanarCoord real_coord = RTUTIL.getRealRectByGCell(access_coord, gcell_axis).getMidPoint();
      double real_x = real_coord.get_x() / 1.0 / micron_dbu;
      double real_y = real_coord.get_y() / 1.0 / micron_dbu;
      int32_t layer_idx = access_coord.get_layer_idx();
      if (layer_idx < 0 || static_cast<int32_t>(routing_layer_list.size()) <= layer_idx) {
        continue;
      }
      std::string layer = routing_layer_list[layer_idx].get_layer_name();
      std::string connect = "load";
      int32_t pin_idx = gsr_pin.get_pin_idx();
      if (origin_net != nullptr && 0 <= pin_idx && pin_idx < static_cast<int32_t>(origin_net->get_pin_list().size())
          && origin_net->get_pin_list()[pin_idx].get_is_driven()) {
        connect = "driven";
      }
      RTUTIL.pushStream(guide_file_stream, "pin ", grid_x, " ", grid_y, " ", real_x, " ", real_y, " ", layer, " ", connect, " ",
                        gsr_pin.get_pin_name(), "\n");
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
        if (first_layer_idx < 0 || static_cast<int32_t>(routing_layer_list.size()) <= first_layer_idx || second_layer_idx < 0
            || static_cast<int32_t>(routing_layer_list.size()) <= second_layer_idx) {
          continue;
        }
        std::string layer1 = routing_layer_list[first_layer_idx].get_layer_name();
        std::string layer2 = routing_layer_list[second_layer_idx].get_layer_name();
        RTUTIL.pushStream(guide_file_stream, "via ", grid1_x, " ", grid1_y, " ", real1_x, " ", real1_y, " ", layer1, " ", layer2, "\n");
      } else {
        if (first_layer_idx < 0 || static_cast<int32_t>(routing_layer_list.size()) <= first_layer_idx) {
          continue;
        }
        std::string layer = routing_layer_list[first_layer_idx].get_layer_name();
        RTUTIL.pushStream(guide_file_stream, "wire ", grid1_x, " ", grid1_y, " ", grid2_x, " ", grid2_y, " ", real1_x, " ", real1_y,
                          " ", real2_x, " ", real2_y, " ", layer, "\n");
      }
    }
  }
  RTUTIL.closeFileStream(guide_file_stream);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void GlobalSpatialRouter::outputSummaryCSV(GSRModel& gsr_model, GSRRouteStats& route_stats)
{
  if (!RTDM.getConfig().output_inter_result) {
    return;
  }

  std::string& gsr_temp_directory_path = RTDM.getConfig().gsr_temp_directory_path;
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  GSRGridGraph& gsr_grid_graph = gsr_model.get_gsr_grid_graph();
  std::map<int32_t, GSRLayerUsage> layer_usage_map = getLayerUsageMap(routing_layer_list, gsr_grid_graph, route_stats);

  std::ofstream* summary_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(gsr_temp_directory_path, "summary.csv"));
  RTUTIL.pushStream(summary_csv_file, "metric,value\n");
  RTUTIL.pushStream(summary_csv_file, "total_net_num,", route_stats.total_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "task_net_num,", route_stats.task_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "routed_net_num,", route_stats.routed_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "skipped_net_num,", route_stats.skipped_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "invalid_access_point_num,", route_stats.invalid_access_point_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "cleared_global_segment_num,", route_stats.cleared_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "uploaded_global_segment_num,", route_stats.uploaded_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "same_layer_segment_num,", route_stats.same_layer_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "via_segment_num,", route_stats.via_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "invalid_segment_num,", route_stats.invalid_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "fallback_topology_net_num,", route_stats.fallback_topology_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "net_without_ta_visible_segment_num,", route_stats.net_without_ta_visible_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "reroute_iter_num,", route_stats.reroute_iter_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "reroute_task_num,", route_stats.reroute_task_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "reroute_accept_num,", route_stats.reroute_accept_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "reroute_reject_num,", route_stats.reroute_reject_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "maze_route_num,", route_stats.maze_route_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "maze_fail_num,", route_stats.maze_fail_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage2_detour_route_num,", route_stats.stage2_detour_route_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage2_detour_accept_num,", route_stats.stage2_detour_accept_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage3_sparse_maze_route_num,", route_stats.stage3_sparse_maze_route_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage3_sparse_maze_success_num,", route_stats.stage3_sparse_maze_success_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "detour_candidate_num,", route_stats.detour_candidate_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "pattern_refine_success_num,", route_stats.pattern_refine_success_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "canonical_tree_num,", route_stats.canonical_tree_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "canonical_removed_edge_num,", route_stats.canonical_removed_edge_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "canonical_removed_node_num,", route_stats.canonical_removed_node_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "canonical_cycle_break_num,", route_stats.canonical_cycle_break_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "canonical_degree2_merge_num,", route_stats.canonical_degree2_merge_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "tree_layer_dp_route_num,", route_stats.tree_layer_dp_route_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "tree_layer_dp_success_num,", route_stats.tree_layer_dp_success_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "tree_layer_dp_fail_num,", route_stats.tree_layer_dp_fail_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "tree_layer_dp_state_num,", route_stats.tree_layer_dp_state_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "tree_layer_dp_edge_candidate_num,", route_stats.tree_layer_dp_edge_candidate_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "total_overflow_accept_num,", route_stats.total_overflow_accept_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "touched_overflow_accept_num,", route_stats.touched_overflow_accept_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "congestion_risk_accept_num,", route_stats.congestion_risk_accept_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "route_cost_accept_num,", route_stats.route_cost_accept_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "route_sanitize_num,", route_stats.route_sanitize_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "route_sanitize_changed_num,", route_stats.route_sanitize_changed_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "route_sanitize_fail_num,", route_stats.route_sanitize_fail_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "shape_guard_reject_num,", route_stats.shape_guard_reject_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_cycle_check_num,", route_stats.planar_cycle_check_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_cycle_detect_num,", route_stats.planar_cycle_detect_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_cycle_repair_num,", route_stats.planar_cycle_repair_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_cycle_repair_fail_num,", route_stats.planar_cycle_repair_fail_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_overlap_check_num,", route_stats.planar_overlap_check_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_overlap_detect_num,", route_stats.planar_overlap_detect_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_overlap_repair_num,", route_stats.planar_overlap_repair_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "planar_overlap_repair_fail_num,", route_stats.planar_overlap_repair_fail_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "sparse_hotspot_line_num,", route_stats.sparse_hotspot_line_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "sparse_offset_line_num,", route_stats.sparse_offset_line_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "ta_visible_net_num,", route_stats.ta_visible_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "via_only_net_num,", route_stats.via_only_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "same_layer_guide_segment_num,", route_stats.same_layer_guide_segment_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "guide_endpoint_on_track_num,", route_stats.guide_endpoint_on_track_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "guide_endpoint_off_track_num,", route_stats.guide_endpoint_off_track_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "non_preferred_reject_num,", route_stats.non_preferred_reject_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "wire_dirty_h_edge_num,", route_stats.wire_dirty_h_edge_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "wire_dirty_v_edge_num,", route_stats.wire_dirty_v_edge_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "wire_dirty_h_row_num,", route_stats.wire_dirty_h_row_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "wire_dirty_v_col_num,", route_stats.wire_dirty_v_col_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "overflow_net_num,", route_stats.overflow_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "selected_overflow_net_num,", route_stats.selected_overflow_net_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "selected_hotspot_num,", route_stats.selected_hotspot_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "skipped_max_routed_times_num,", route_stats.skipped_max_routed_times_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage2_coverage_cell_num,", route_stats.stage2_coverage_cell_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage3_coverage_cell_num,", route_stats.stage3_coverage_cell_num, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage2_coverage_ratio,", route_stats.stage2_coverage_ratio, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage3_coverage_ratio,", route_stats.stage3_coverage_ratio, "\n");
  RTUTIL.pushStream(summary_csv_file, "congestion_view_h_risk_sum,", route_stats.congestion_view_h_risk_sum, "\n");
  RTUTIL.pushStream(summary_csv_file, "congestion_view_v_risk_sum,", route_stats.congestion_view_v_risk_sum, "\n");
  RTUTIL.pushStream(summary_csv_file, "congestion_view_hotspot_sum,", route_stats.congestion_view_hotspot_sum, "\n");
  RTUTIL.pushStream(summary_csv_file, "stage1_total_overflow,", gsr_model.get_stage1_total_overflow(), "\n");
  RTUTIL.pushStream(summary_csv_file, "stage2_total_overflow,", gsr_model.get_stage2_total_overflow(), "\n");
  RTUTIL.pushStream(summary_csv_file, "stage3_total_overflow,", gsr_model.get_stage3_total_overflow(), "\n");
  RTUTIL.pushStream(summary_csv_file, "best_total_overflow,", gsr_model.get_best_total_overflow(), "\n");
  RTUTIL.pushStream(summary_csv_file, "best_total_congestion_risk,", gsr_model.get_best_total_congestion_risk(), "\n");
  for (RoutingLayer& routing_layer : routing_layer_list) {
    int32_t layer_idx = routing_layer.get_layer_idx();
    if (!gsr_grid_graph.isLegalLayer(layer_idx)) {
      continue;
    }
    RTUTIL.pushStream(summary_csv_file, "layer_", routing_layer.get_layer_name(), "_wire_grid_length,", layer_usage_map[layer_idx].wire_grid_length, "\n");
    RTUTIL.pushStream(summary_csv_file, "layer_", routing_layer.get_layer_name(), "_same_layer_segment_num,",
                      layer_usage_map[layer_idx].same_layer_segment_num, "\n");
    RTUTIL.pushStream(summary_csv_file, "layer_", routing_layer.get_layer_name(), "_via_touch_num,", layer_usage_map[layer_idx].via_touch_num, "\n");
  }
  RTUTIL.closeFileStream(summary_csv_file);

  std::ofstream* layer_usage_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(gsr_temp_directory_path, "layer_usage.csv"));
  RTUTIL.pushStream(layer_usage_csv_file,
                    "layer_idx,layer_name,same_layer_segment_num,wire_grid_length,via_touch_num,total_demand,total_supply,total_overflow,max_usage_ratio\n");
  for (RoutingLayer& routing_layer : routing_layer_list) {
    int32_t layer_idx = routing_layer.get_layer_idx();
    if (!gsr_grid_graph.isLegalLayer(layer_idx)) {
      continue;
    }
    double total_demand = 0;
    double total_supply = 0;
    double total_overflow = 0;
    double max_usage_ratio = 0;
    GridMap<GSRNode>& gsr_node_map = gsr_grid_graph.get_layer_node_map()[layer_idx];
    for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
      for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
        GSRNode& gsr_node = gsr_node_map[x][y];
        total_overflow += gsr_node.getOverflow();
        max_usage_ratio = std::max(max_usage_ratio, gsr_node.getMaxUsageRatio());
        for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
          total_demand += gsr_node.getBoundaryDemand(orient);
          total_supply += gsr_node.getSupply(orient);
        }
        total_demand += gsr_node.getInternalDemand();
        total_supply += gsr_node.getInternalSupply();
      }
    }
    GSRLayerUsage& layer_usage = layer_usage_map[layer_idx];
    RTUTIL.pushStream(layer_usage_csv_file, layer_idx, ",", routing_layer.get_layer_name(), ",", layer_usage.same_layer_segment_num, ",",
                      layer_usage.wire_grid_length, ",", layer_usage.via_touch_num, ",", total_demand, ",", total_supply, ",", total_overflow, ",",
                      max_usage_ratio, "\n");
  }
  RTUTIL.closeFileStream(layer_usage_csv_file);

  std::ofstream* hotspot_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(gsr_temp_directory_path, "congestion_hotspot_GSR.csv"));
  RTUTIL.pushStream(hotspot_csv_file, "layer_idx,layer_name,x,y,orient,demand,supply,overflow,usage_ratio,net_count,net_list\n");
  for (RoutingLayer& routing_layer : routing_layer_list) {
    int32_t layer_idx = routing_layer.get_layer_idx();
    if (!gsr_grid_graph.isLegalLayer(layer_idx)) {
      continue;
    }
    GridMap<GSRNode>& gsr_node_map = gsr_grid_graph.get_layer_node_map()[layer_idx];
    for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
      for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
        GSRNode& gsr_node = gsr_node_map[x][y];
        for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
          double demand = gsr_node.getBoundaryDemand(orient);
          double supply = gsr_node.getSupply(orient);
          double overflow = std::max(0.0, demand - supply);
          double usage_ratio = supply > 0 ? demand / supply : (demand > 0 ? 1.0e6 : 0);
          if (overflow <= 0) {
            continue;
          }
          std::set<int32_t> net_set;
          if (RTUTIL.exist(gsr_node.get_orient_net_map(), orient)) {
            net_set = gsr_node.get_orient_net_map()[orient];
          }
          RTUTIL.pushStream(hotspot_csv_file, layer_idx, ",", routing_layer.get_layer_name(), ",", x, ",", y, ",", GetOrientationName()(orient), ",",
                            demand, ",", supply, ",", overflow, ",", usage_ratio, ",", net_set.size(), ",", joinNetSet(net_set), "\n");
        }
      }
    }
  }
  RTUTIL.closeFileStream(hotspot_csv_file);
}

bool GlobalSpatialRouter::isValidSegment(GSRModel& gsr_model, Segment<LayerCoord>& segment)
{
  LayerCoord& first_coord = segment.get_first();
  LayerCoord& second_coord = segment.get_second();
  if (!gsr_model.get_gsr_grid_graph().isInside(first_coord) || !gsr_model.get_gsr_grid_graph().isInside(second_coord)) {
    return false;
  }

  if (first_coord.get_layer_idx() == second_coord.get_layer_idx()) {
    if (first_coord.get_planar_coord() == second_coord.get_planar_coord()) {
      return false;
    }
    Direction segment_direction = getPlanarSegmentDirection(segment);
    if (segment_direction == Direction::kHorizontal || segment_direction == Direction::kVertical) {
      std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
      return routing_layer_list[first_coord.get_layer_idx()].get_prefer_direction() == segment_direction;
    }
    return false;
  }

  if (first_coord.get_x() != second_coord.get_x() || first_coord.get_y() != second_coord.get_y()) {
    return false;
  }
  return std::abs(first_coord.get_layer_idx() - second_coord.get_layer_idx()) == 1;
}

}  // namespace irt
