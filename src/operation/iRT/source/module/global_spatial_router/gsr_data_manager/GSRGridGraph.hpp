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

#include "GCell.hpp"
#include "LayerCoord.hpp"
#include "PlanarCoord.hpp"
#include "RTHeader.hpp"
#include "Segment.hpp"
#include "GSRComParam.hpp"
#include "GSRNet.hpp"
#include "GSRNode.hpp"
#include "Utility.hpp"

namespace irt {

class GSRGridGraph
{
 public:
  GSRGridGraph() = default;
  GSRGridGraph(const int32_t x_size, const int32_t y_size, const GSRComParam& gsr_com_param)
      : _x_size(x_size), _y_size(y_size), _gsr_com_param(gsr_com_param)
  {
  }
  ~GSRGridGraph() = default;
  // getter
  int32_t get_x_size() const { return _x_size; }
  int32_t get_y_size() const { return _y_size; }
  GSRComParam& get_gsr_com_param() { return _gsr_com_param; }
  const GSRComParam& get_gsr_com_param() const { return _gsr_com_param; }
  std::vector<GridMap<GSRNode>>& get_layer_node_map() { return _layer_node_map; }
  const std::vector<GridMap<GSRNode>>& get_layer_node_map() const { return _layer_node_map; }
  // setter
  void set_x_size(const int32_t x_size) { _x_size = x_size; }
  void set_y_size(const int32_t y_size) { _y_size = y_size; }
  void set_gsr_com_param(const GSRComParam& gsr_com_param) { _gsr_com_param = gsr_com_param; }
  void set_layer_node_map(const std::vector<GridMap<GSRNode>>& layer_node_map) { _layer_node_map = layer_node_map; }
  // function
  bool isInside(const PlanarCoord& coord) const { return 0 <= coord.get_x() && coord.get_x() < _x_size && 0 <= coord.get_y() && coord.get_y() < _y_size; }
  bool isInside(const LayerCoord& coord) const { return isInside(coord.get_planar_coord()) && isLegalLayer(coord.get_layer_idx()); }
  bool isLegalLayer(const int32_t layer_idx) const
  {
    return _gsr_com_param.get_bottom_routing_layer_idx() <= layer_idx && layer_idx <= _gsr_com_param.get_top_routing_layer_idx();
  }
  void initFromGCellMap(GridMap<GCell>& gcell_map)
  {
    _layer_node_map.clear();
    _layer_node_map.resize(_gsr_com_param.get_top_routing_layer_idx() + 1);
    for (int32_t layer_idx = _gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= _gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
      _layer_node_map[layer_idx].init(_x_size, _y_size);
      for (int32_t x = 0; x < _x_size; x++) {
        for (int32_t y = 0; y < _y_size; y++) {
          GCell& gcell = gcell_map[x][y];
          GSRNode& gsr_node = _layer_node_map[layer_idx][x][y];
          gsr_node.set_coord(x, y);
          gsr_node.set_layer_idx(layer_idx);
          gsr_node.set_boundary_wire_unit(gcell.get_boundary_wire_unit());
          gsr_node.set_internal_wire_unit(gcell.get_internal_wire_unit());
          gsr_node.set_internal_via_unit(gcell.get_internal_via_unit());
          if (RTUTIL.exist(gcell.get_routing_orient_supply_map(), layer_idx)) {
            gsr_node.set_orient_supply_map(gcell.get_routing_orient_supply_map()[layer_idx]);
          }
        }
      }
    }
  }
  double getRouteCost(const int32_t net_idx, const std::vector<Segment<LayerCoord>>& segment_list, const GSRComParam& gsr_com_param) const
  {
    std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> usage_map;
    double cost = 0;
    for (const Segment<LayerCoord>& segment : segment_list) {
      cost += getBaseSegmentCost(segment, gsr_com_param);
      if (isViaSegment(segment)) {
        cost += getViaMinAreaCost(segment.get_first(), segment.get_second(), gsr_com_param, gsr_com_param.get_cost_logistic_slope());
      }
      std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> segment_usage_map = getUsageMap(segment);
      for (auto& [usage_coord, orient_set] : segment_usage_map) {
        usage_map[usage_coord].insert(orient_set.begin(), orient_set.end());
      }
    }
    for (auto& [usage_coord, orient_set] : usage_map) {
      if (!isInside(usage_coord)) {
        cost += 1.0e12;
        continue;
      }
      const GSRNode& gsr_node = _layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()];
      cost += getProjectedLogisticCost(gsr_node, net_idx, orient_set, gsr_com_param, gsr_com_param.get_cost_logistic_slope());
    }
    return cost;
  }
  void updateDemandToGraph(const ChangeType change_type, const int32_t net_idx, const std::vector<Segment<LayerCoord>>& segment_list)
  {
    std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> usage_map;
    for (const Segment<LayerCoord>& segment : segment_list) {
      std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> segment_usage_map = getUsageMap(segment);
      for (auto& [usage_coord, orient_set] : segment_usage_map) {
        usage_map[usage_coord].insert(orient_set.begin(), orient_set.end());
      }
    }
    for (auto& [usage_coord, orient_set] : usage_map) {
      if (isInside(usage_coord)) {
        _layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()].updateDemand(net_idx, orient_set, change_type);
      }
    }
  }
  std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> getRouteUsageMap(const std::vector<Segment<LayerCoord>>& segment_list) const
  {
    std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> usage_map;
    for (const Segment<LayerCoord>& segment : segment_list) {
      std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> segment_usage_map = getUsageMap(segment);
      for (auto& [usage_coord, orient_set] : segment_usage_map) {
        usage_map[usage_coord].insert(orient_set.begin(), orient_set.end());
      }
    }
    return usage_map;
  }
  double getRouteOverflow(const std::vector<Segment<LayerCoord>>& segment_list) const
  {
    double route_overflow = 0;
    for (auto& [usage_coord, orient_set] : getRouteUsageMap(segment_list)) {
      (void) orient_set;
      if (isInside(usage_coord)) {
        route_overflow += _layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()].getOverflow();
      }
    }
    return route_overflow;
  }
  double getRouteCongestionRisk(const std::vector<Segment<LayerCoord>>& segment_list) const
  {
    double route_congestion_risk = 0;
    for (auto& [usage_coord, orient_set] : getRouteUsageMap(segment_list)) {
      (void) orient_set;
      if (isInside(usage_coord)) {
        route_congestion_risk += _layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()].get_congestion_risk();
      }
    }
    return route_congestion_risk;
  }
  double getTotalOverflow() const
  {
    double total_overflow = 0;
    for (int32_t layer_idx = _gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= _gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
      const GridMap<GSRNode>& gsr_node_map = _layer_node_map[layer_idx];
      for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
        for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
          total_overflow += gsr_node_map[x][y].getOverflow();
        }
      }
    }
    return total_overflow;
  }
  std::vector<std::set<int32_t>> getOverflowNetSetList() const
  {
    std::vector<std::set<int32_t>> overflow_net_set_list;
    for (int32_t layer_idx = _gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= _gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
      const GridMap<GSRNode>& gsr_node_map = _layer_node_map[layer_idx];
      for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
        for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
          std::set<int32_t> overflow_net_set = gsr_node_map[x][y].getOverflowNetSet();
          if (!overflow_net_set.empty()) {
            overflow_net_set_list.push_back(overflow_net_set);
          }
        }
      }
    }
    return overflow_net_set_list;
  }
  void updateCongestionRisk(std::vector<GridMap<double>>& layer_congestion_risk_map, const int32_t risk_radius, const double history_risk_decay)
  {
    if (layer_congestion_risk_map.size() != _layer_node_map.size()) {
      layer_congestion_risk_map.clear();
      layer_congestion_risk_map.resize(_layer_node_map.size());
    }
    for (int32_t layer_idx = _gsr_com_param.get_bottom_routing_layer_idx(); layer_idx <= _gsr_com_param.get_top_routing_layer_idx(); layer_idx++) {
      GridMap<GSRNode>& gsr_node_map = _layer_node_map[layer_idx];
      GridMap<double> history_congestion_risk_map = layer_congestion_risk_map[layer_idx];
      GridMap<double>& congestion_risk_map = layer_congestion_risk_map[layer_idx];
      congestion_risk_map.init(_x_size, _y_size, 0.0);
      for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
        for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
          double overflow = gsr_node_map[x][y].getOverflow();
          if (overflow <= 0) {
            continue;
          }
          for (int32_t risk_x = x - risk_radius; risk_x <= x + risk_radius; risk_x++) {
            for (int32_t risk_y = y - risk_radius; risk_y <= y + risk_radius; risk_y++) {
              if (!congestion_risk_map.isInside(risk_x, risk_y)) {
                continue;
              }
              int32_t distance = std::abs(risk_x - x) + std::abs(risk_y - y);
              if (risk_radius < distance) {
                continue;
              }
              double decay = risk_radius == 0 ? 1.0 : static_cast<double>(risk_radius - distance + 1) / static_cast<double>(risk_radius + 1);
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
      for (int32_t x = 0; x < gsr_node_map.get_x_size(); x++) {
        for (int32_t y = 0; y < gsr_node_map.get_y_size(); y++) {
          gsr_node_map[x][y].set_congestion_risk(congestion_risk_map[x][y]);
        }
      }
    }
  }
  double getTotalCongestionRisk(const std::vector<GSRNet>& gsr_net_list) const
  {
    double total_congestion_risk = 0;
    for (const GSRNet& gsr_net : gsr_net_list) {
      total_congestion_risk += getRouteCongestionRisk(gsr_net.get_routing_segment_list());
    }
    return total_congestion_risk;
  }
  double getProjectedViaCost(const int32_t net_idx, const PlanarCoord& coord, const int32_t first_layer_idx, const int32_t second_layer_idx,
                             const GSRComParam& gsr_com_param, const double slope) const
  {
    if (!isInside(coord)) {
      return 1.0e12;
    }
    if (first_layer_idx == second_layer_idx) {
      return 0;
    }
    int32_t lower_layer_idx = std::min(first_layer_idx, second_layer_idx);
    int32_t upper_layer_idx = std::max(first_layer_idx, second_layer_idx);
    double cost = 0;
    for (int32_t layer_idx = lower_layer_idx; layer_idx < upper_layer_idx; layer_idx++) {
      LayerCoord first_coord(coord, layer_idx);
      LayerCoord second_coord(coord, layer_idx + 1);
      Segment<LayerCoord> segment(first_coord, second_coord);
      cost += getBaseSegmentCost(segment, gsr_com_param);
      cost += getViaMinAreaCost(first_coord, second_coord, gsr_com_param, slope);
      std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> usage_map = getUsageMap(segment);
      for (auto& [usage_coord, orient_set] : usage_map) {
        if (!isInside(usage_coord)) {
          cost += 1.0e12;
          continue;
        }
        const GSRNode& gsr_node = _layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()];
        cost += getProjectedLogisticCost(gsr_node, net_idx, orient_set, gsr_com_param, slope);
      }
    }
    return cost;
  }
  double getBestProjectedSwitchCost(const int32_t net_idx, const PlanarCoord& coord, const std::vector<int32_t>& first_layer_list,
                                    const std::vector<int32_t>& second_layer_list, const GSRComParam& gsr_com_param, const double slope) const
  {
    double best_cost = std::numeric_limits<double>::max();
    for (int32_t first_layer_idx : first_layer_list) {
      for (int32_t second_layer_idx : second_layer_list) {
        best_cost = std::min(best_cost, getProjectedViaCost(net_idx, coord, first_layer_idx, second_layer_idx, gsr_com_param, slope));
      }
    }
    return best_cost == std::numeric_limits<double>::max() ? 1.0e12 : best_cost;
  }
  double getBestProjectedPlanarEdgeCost(const int32_t net_idx, const Direction direction, const PlanarCoord& first_coord,
                                        const PlanarCoord& second_coord, const std::vector<int32_t>& layer_idx_list,
                                        const GSRComParam& gsr_com_param, const double slope) const
  {
    if (!isInside(first_coord) || !isInside(second_coord) || RTUTIL.getDirection(first_coord, second_coord) != direction) {
      return 1.0e12;
    }
    double best_cost = std::numeric_limits<double>::max();
    for (int32_t layer_idx : layer_idx_list) {
      LayerCoord first_layer_coord(first_coord, layer_idx);
      LayerCoord second_layer_coord(second_coord, layer_idx);
      Segment<LayerCoord> segment(first_layer_coord, second_layer_coord);
      double cost = getBaseSegmentCost(segment, gsr_com_param);
      std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> usage_map = getUsageMap(segment);
      for (auto& [usage_coord, orient_set] : usage_map) {
        if (!isInside(usage_coord)) {
          cost += 1.0e12;
          continue;
        }
        const GSRNode& gsr_node = _layer_node_map[usage_coord.get_layer_idx()][usage_coord.get_x()][usage_coord.get_y()];
        cost += getProjectedLogisticCost(gsr_node, net_idx, orient_set, gsr_com_param, slope);
        cost += gsr_com_param.get_unit_short_cost() * gsr_node.get_congestion_risk();
      }
      best_cost = std::min(best_cost, cost);
    }
    return best_cost == std::numeric_limits<double>::max() ? 1.0e12 : best_cost;
  }

 private:
  int32_t _x_size = 0;
  int32_t _y_size = 0;
  GSRComParam _gsr_com_param;
  std::vector<GridMap<GSRNode>> _layer_node_map;

  static bool isPlanarOrientation(const Orientation orient)
  {
    return orient == Orientation::kEast || orient == Orientation::kWest || orient == Orientation::kSouth || orient == Orientation::kNorth;
  }
  static bool isViaOrientation(const Orientation orient) { return orient == Orientation::kAbove || orient == Orientation::kBelow; }
  static bool isViaSegment(const Segment<LayerCoord>& segment)
  {
    return segment.get_first().get_layer_idx() != segment.get_second().get_layer_idx();
  }
  static bool isHorizontalOrientation(const Orientation orient) { return orient == Orientation::kEast || orient == Orientation::kWest; }
  static bool isVerticalOrientation(const Orientation orient) { return orient == Orientation::kSouth || orient == Orientation::kNorth; }
  static double logistic(const double remaining, const double slope)
  {
    double x = std::max(-50.0, std::min(50.0, slope * remaining));
    return 1.0 / (1.0 + std::exp(x));
  }
  static double getDemandSupplyLogisticCost(const double demand, const double supply, const GSRComParam& gsr_com_param, const double slope)
  {
    if (demand <= 0) {
      return 0.0;
    }
    if (supply <= 0) {
      return gsr_com_param.get_unit_short_cost();
    }
    return gsr_com_param.get_unit_short_cost() * logistic(supply - demand, slope);
  }
  static double getProjectedLogisticCost(const GSRNode& gsr_node, const int32_t net_idx, const std::set<Orientation>& orient_set,
                                         const GSRComParam& gsr_com_param, const double slope)
  {
    double cost = 0;
    for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
      bool add_extra = RTUTIL.exist(orient_set, orient);
      double demand = gsr_node.getBoundaryDemand(orient, net_idx, add_extra);
      double supply = gsr_node.getSupply(orient);
      cost += getDemandSupplyLogisticCost(demand, supply, gsr_com_param, slope);
    }
    double internal_demand = gsr_node.getInternalDemand(net_idx, &orient_set);
    double internal_supply = gsr_node.getInternalSupply();
    cost += getDemandSupplyLogisticCost(internal_demand, internal_supply, gsr_com_param, slope);
    return cost;
  }
  double getProjectedViaMinAreaCost(const LayerCoord& usage_coord, const std::set<Orientation>& orient_set, const GSRComParam& gsr_com_param,
                                    const double slope) const
  {
    if (!RTUTIL.exist(orient_set, Orientation::kAbove) && !RTUTIL.exist(orient_set, Orientation::kBelow)) {
      return 0;
    }
    double cost = 0;
    if (RTUTIL.exist(orient_set, Orientation::kAbove)) {
      LayerCoord above_coord(usage_coord.get_x(), usage_coord.get_y(), usage_coord.get_layer_idx() + 1);
      cost += getViaMinAreaCost(usage_coord, above_coord, gsr_com_param, slope);
    }
    if (RTUTIL.exist(orient_set, Orientation::kBelow)) {
      LayerCoord below_coord(usage_coord.get_x(), usage_coord.get_y(), usage_coord.get_layer_idx() - 1);
      cost += getViaMinAreaCost(usage_coord, below_coord, gsr_com_param, slope);
    }
    return cost;
  }
  double getViaMinAreaCost(const LayerCoord& first_coord, const LayerCoord& second_coord, const GSRComParam& gsr_com_param, const double slope) const
  {
    if (first_coord.get_x() != second_coord.get_x() || first_coord.get_y() != second_coord.get_y()) {
      return 1.0e12;
    }
    double cost = 0;
    int32_t lower_layer_idx = std::min(first_coord.get_layer_idx(), second_coord.get_layer_idx());
    int32_t upper_layer_idx = std::max(first_coord.get_layer_idx(), second_coord.get_layer_idx());
    for (int32_t layer_idx : {lower_layer_idx, upper_layer_idx}) {
      if (!isLegalLayer(layer_idx)) {
        cost += 1.0e12;
        continue;
      }
      cost += getLayerViaSideCost(LayerCoord(first_coord.get_x(), first_coord.get_y(), layer_idx), gsr_com_param, slope);
    }
    return cost;
  }
  double getLayerViaSideCost(const LayerCoord& via_coord, const GSRComParam& gsr_com_param, const double slope) const
  {
    const GridMap<GSRNode>& gsr_node_map = _layer_node_map[via_coord.get_layer_idx()];
    const GSRNode& via_node = gsr_node_map[via_coord.get_x()][via_coord.get_y()];
    double demand_unit = gsr_com_param.get_via_min_area_demand_unit() * gsr_com_param.get_via_multiplier();
    double cost = 0;
    std::vector<Orientation> side_orient_list;
    double east_west_supply = via_node.getSupply(Orientation::kEast) + via_node.getSupply(Orientation::kWest);
    double north_south_supply = via_node.getSupply(Orientation::kNorth) + via_node.getSupply(Orientation::kSouth);
    if (east_west_supply >= north_south_supply) {
      side_orient_list = {Orientation::kEast, Orientation::kWest};
    } else {
      side_orient_list = {Orientation::kNorth, Orientation::kSouth};
    }
    for (Orientation orient : side_orient_list) {
      double demand = via_node.getBoundaryDemand(orient) + demand_unit;
      double supply = via_node.getSupply(orient);
      cost += 0.5 * getDemandSupplyLogisticCost(demand, supply, gsr_com_param, slope);
    }
    return cost;
  }
  static std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> getUsageMap(const Segment<LayerCoord>& segment)
  {
    std::map<LayerCoord, std::set<Orientation>, CmpLayerCoordByXASC> usage_map;
    const LayerCoord& first_coord = segment.get_first();
    const LayerCoord& second_coord = segment.get_second();

    Orientation orientation = RTUTIL.getOrientation(first_coord, second_coord);
    if (orientation == Orientation::kNone || orientation == Orientation::kOblique) {
      return usage_map;
    }
    Orientation opposite_orientation = RTUTIL.getOppositeOrientation(orientation);

    int32_t first_x = first_coord.get_x();
    int32_t first_y = first_coord.get_y();
    int32_t first_layer_idx = first_coord.get_layer_idx();
    int32_t second_x = second_coord.get_x();
    int32_t second_y = second_coord.get_y();
    int32_t second_layer_idx = second_coord.get_layer_idx();
    RTUTIL.swapByASC(first_x, second_x);
    RTUTIL.swapByASC(first_y, second_y);
    RTUTIL.swapByASC(first_layer_idx, second_layer_idx);

    for (int32_t x = first_x; x <= second_x; x++) {
      for (int32_t y = first_y; y <= second_y; y++) {
        for (int32_t layer_idx = first_layer_idx; layer_idx <= second_layer_idx; layer_idx++) {
          LayerCoord coord(x, y, layer_idx);
          if (coord != first_coord) {
            usage_map[coord].insert(opposite_orientation);
          }
          if (coord != second_coord) {
            usage_map[coord].insert(orientation);
          }
        }
      }
    }
    return usage_map;
  }
  static double getBaseSegmentCost(const Segment<LayerCoord>& segment, const GSRComParam& gsr_com_param)
  {
    const LayerCoord& first_coord = segment.get_first();
    const LayerCoord& second_coord = segment.get_second();
    Orientation orientation = RTUTIL.getOrientation(first_coord, second_coord);
    int32_t planar_length = std::abs(first_coord.get_x() - second_coord.get_x()) + std::abs(first_coord.get_y() - second_coord.get_y());
    int32_t via_length = std::abs(first_coord.get_layer_idx() - second_coord.get_layer_idx());
    if (orientation == Orientation::kAbove || orientation == Orientation::kBelow) {
      return gsr_com_param.get_unit_via_cost() * via_length;
    }
    if (isPlanarOrientation(orientation)) {
      return gsr_com_param.get_unit_wire_cost() * std::max(planar_length, 1);
    }
    return 1.0e12;
  }
};

}  // namespace irt
