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

#include "RTHeader.hpp"

namespace irt {

class GSRComParam
{
 public:
  GSRComParam() = default;
  GSRComParam(const int32_t bottom_routing_layer_idx, const int32_t top_routing_layer_idx, const int32_t horizontal_layer_idx,
             const int32_t vertical_layer_idx)
      : _bottom_routing_layer_idx(bottom_routing_layer_idx),
        _top_routing_layer_idx(top_routing_layer_idx),
        _horizontal_layer_idx(horizontal_layer_idx),
        _vertical_layer_idx(vertical_layer_idx)
  {
  }
  ~GSRComParam() = default;
  // getter
  int32_t get_bottom_routing_layer_idx() const { return _bottom_routing_layer_idx; }
  int32_t get_top_routing_layer_idx() const { return _top_routing_layer_idx; }
  int32_t get_horizontal_layer_idx() const { return _horizontal_layer_idx; }
  int32_t get_vertical_layer_idx() const { return _vertical_layer_idx; }
  double get_unit_wire_cost() const { return _unit_wire_cost; }
  double get_unit_short_cost() const { return _unit_short_cost; }
  double get_unit_via_cost() const { return _unit_via_cost; }
  double get_cost_logistic_slope() const { return _cost_logistic_slope; }
  double get_maze_logistic_slope() const { return _maze_logistic_slope; }
  double get_via_min_area_demand_unit() const { return _via_min_area_demand_unit; }
  double get_via_multiplier() const { return _via_multiplier; }
  double get_history_risk_decay() const { return _history_risk_decay; }
  int32_t get_max_reroute_iter() const { return _max_reroute_iter; }
  int32_t get_max_routed_times() const { return _max_routed_times; }
  int32_t get_congestion_risk_radius() const { return _congestion_risk_radius; }
  int32_t get_maze_window_size() const { return _maze_window_size; }
  int32_t get_maze_window_max_expand_times() const { return _maze_window_max_expand_times; }
  int32_t get_min_reroute_task_num() const { return _min_reroute_task_num; }
  int32_t get_max_reroute_task_num() const { return _max_reroute_task_num; }
  double get_reroute_task_growth_ratio() const { return _reroute_task_growth_ratio; }
  double get_reroute_coverage_target() const { return _reroute_coverage_target; }
  double get_max_detour_ratio() const { return _max_detour_ratio; }
  int32_t get_target_detour_count() const { return _target_detour_count; }
  int32_t get_sparse_grid_interval() const { return _sparse_grid_interval; }
  double get_detour_congestion_threshold() const { return _detour_congestion_threshold; }
  double get_congestion_risk_threshold() const { return _congestion_risk_threshold; }
  double get_near_full_usage_ratio() const { return _near_full_usage_ratio; }
  int32_t get_congestion_view_radius() const { return _congestion_view_radius; }
  double get_congestion_overflow_weight() const { return _congestion_overflow_weight; }
  double get_congestion_near_full_weight() const { return _congestion_near_full_weight; }
  double get_congestion_internal_weight() const { return _congestion_internal_weight; }
  double get_congestion_via_weight() const { return _congestion_via_weight; }
  double get_congestion_history_weight() const { return _congestion_history_weight; }
  // setter
  void set_bottom_routing_layer_idx(const int32_t bottom_routing_layer_idx) { _bottom_routing_layer_idx = bottom_routing_layer_idx; }
  void set_top_routing_layer_idx(const int32_t top_routing_layer_idx) { _top_routing_layer_idx = top_routing_layer_idx; }
  void set_horizontal_layer_idx(const int32_t horizontal_layer_idx) { _horizontal_layer_idx = horizontal_layer_idx; }
  void set_vertical_layer_idx(const int32_t vertical_layer_idx) { _vertical_layer_idx = vertical_layer_idx; }
  void set_unit_wire_cost(const double unit_wire_cost) { _unit_wire_cost = unit_wire_cost; }
  void set_unit_short_cost(const double unit_short_cost) { _unit_short_cost = unit_short_cost; }
  void set_unit_via_cost(const double unit_via_cost) { _unit_via_cost = unit_via_cost; }
  void set_cost_logistic_slope(const double cost_logistic_slope) { _cost_logistic_slope = cost_logistic_slope; }
  void set_maze_logistic_slope(const double maze_logistic_slope) { _maze_logistic_slope = maze_logistic_slope; }
  void set_via_min_area_demand_unit(const double via_min_area_demand_unit) { _via_min_area_demand_unit = via_min_area_demand_unit; }
  void set_via_multiplier(const double via_multiplier) { _via_multiplier = via_multiplier; }
  void set_history_risk_decay(const double history_risk_decay) { _history_risk_decay = history_risk_decay; }
  void set_max_reroute_iter(const int32_t max_reroute_iter) { _max_reroute_iter = max_reroute_iter; }
  void set_max_routed_times(const int32_t max_routed_times) { _max_routed_times = max_routed_times; }
  void set_congestion_risk_radius(const int32_t congestion_risk_radius) { _congestion_risk_radius = congestion_risk_radius; }
  void set_maze_window_size(const int32_t maze_window_size) { _maze_window_size = maze_window_size; }
  void set_maze_window_max_expand_times(const int32_t maze_window_max_expand_times)
  {
    _maze_window_max_expand_times = maze_window_max_expand_times;
  }
  void set_min_reroute_task_num(const int32_t min_reroute_task_num) { _min_reroute_task_num = min_reroute_task_num; }
  void set_max_reroute_task_num(const int32_t max_reroute_task_num) { _max_reroute_task_num = max_reroute_task_num; }
  void set_reroute_task_growth_ratio(const double reroute_task_growth_ratio) { _reroute_task_growth_ratio = reroute_task_growth_ratio; }
  void set_reroute_coverage_target(const double reroute_coverage_target) { _reroute_coverage_target = reroute_coverage_target; }
  void set_max_detour_ratio(const double max_detour_ratio) { _max_detour_ratio = max_detour_ratio; }
  void set_target_detour_count(const int32_t target_detour_count) { _target_detour_count = target_detour_count; }
  void set_sparse_grid_interval(const int32_t sparse_grid_interval) { _sparse_grid_interval = sparse_grid_interval; }
  void set_detour_congestion_threshold(const double detour_congestion_threshold)
  {
    _detour_congestion_threshold = detour_congestion_threshold;
  }
  void set_congestion_risk_threshold(const double congestion_risk_threshold) { _congestion_risk_threshold = congestion_risk_threshold; }
  void set_near_full_usage_ratio(const double near_full_usage_ratio) { _near_full_usage_ratio = near_full_usage_ratio; }
  void set_congestion_view_radius(const int32_t congestion_view_radius) { _congestion_view_radius = congestion_view_radius; }
  void set_congestion_overflow_weight(const double congestion_overflow_weight)
  {
    _congestion_overflow_weight = congestion_overflow_weight;
  }
  void set_congestion_near_full_weight(const double congestion_near_full_weight)
  {
    _congestion_near_full_weight = congestion_near_full_weight;
  }
  void set_congestion_internal_weight(const double congestion_internal_weight)
  {
    _congestion_internal_weight = congestion_internal_weight;
  }
  void set_congestion_via_weight(const double congestion_via_weight) { _congestion_via_weight = congestion_via_weight; }
  void set_congestion_history_weight(const double congestion_history_weight)
  {
    _congestion_history_weight = congestion_history_weight;
  }

 private:
  int32_t _bottom_routing_layer_idx = -1;
  int32_t _top_routing_layer_idx = -1;
  int32_t _horizontal_layer_idx = -1;
  int32_t _vertical_layer_idx = -1;
  double _unit_wire_cost = 1.0;
  double _unit_short_cost = 10.0;
  double _unit_via_cost = 5.0;
  double _cost_logistic_slope = 1.0;
  double _maze_logistic_slope = 1.0;
  double _via_min_area_demand_unit = 0.5;
  double _via_multiplier = 1.0;
  double _history_risk_decay = 0.8;
  int32_t _max_reroute_iter = 3;
  int32_t _max_routed_times = 4;
  int32_t _congestion_risk_radius = 2;
  int32_t _maze_window_size = 8;
  int32_t _maze_window_max_expand_times = 3;
  int32_t _min_reroute_task_num = 2048;
  int32_t _max_reroute_task_num = 8192;
  double _reroute_task_growth_ratio = 1.5;
  double _reroute_coverage_target = 0.8;
  double _max_detour_ratio = 0.5;
  int32_t _target_detour_count = 4;
  int32_t _sparse_grid_interval = 10;
  double _detour_congestion_threshold = 0.0;
  double _congestion_risk_threshold = 0.1;
  double _near_full_usage_ratio = 0.85;
  int32_t _congestion_view_radius = 3;
  double _congestion_overflow_weight = 1.0;
  double _congestion_near_full_weight = 0.25;
  double _congestion_internal_weight = 0.5;
  double _congestion_via_weight = 0.5;
  double _congestion_history_weight = 0.5;
};

}  // namespace irt
