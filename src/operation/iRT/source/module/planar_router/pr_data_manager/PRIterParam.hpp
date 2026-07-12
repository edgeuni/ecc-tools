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

#include "RTHeader.hpp"

namespace irt {

class PRIterParam
{
 public:
  PRIterParam() = default;
  PRIterParam(int32_t max_iter_num, int32_t max_routed_times, int32_t max_task_num, double wire_unit, double corner_unit,
              double overflow_unit, double congestion_risk_unit, double high_usage_unit, double high_usage_ratio_threshold,
              int32_t congestion_risk_radius, int32_t route_window_base_expand, int32_t route_window_max_expand_times,
              double route_window_expand_ratio, bool enable_full_die_fallback)
  {
    _max_iter_num = max_iter_num;
    _max_routed_times = max_routed_times;
    _max_task_num = max_task_num;
    _wire_unit = wire_unit;
    _corner_unit = corner_unit;
    _overflow_unit = overflow_unit;
    _congestion_risk_unit = congestion_risk_unit;
    _high_usage_unit = high_usage_unit;
    _high_usage_ratio_threshold = high_usage_ratio_threshold;
    _congestion_risk_radius = congestion_risk_radius;
    _route_window_base_expand = route_window_base_expand;
    _route_window_max_expand_times = route_window_max_expand_times;
    _route_window_expand_ratio = route_window_expand_ratio;
    _enable_full_die_fallback = enable_full_die_fallback;
  }
  ~PRIterParam() = default;
  // getter
  int32_t get_max_iter_num() const { return _max_iter_num; }
  int32_t get_max_routed_times() const { return _max_routed_times; }
  int32_t get_max_task_num() const { return _max_task_num; }
  double get_wire_unit() const { return _wire_unit; }
  double get_corner_unit() const { return _corner_unit; }
  double get_overflow_unit() const { return _overflow_unit; }
  double get_congestion_risk_unit() const { return _congestion_risk_unit; }
  double get_high_usage_unit() const { return _high_usage_unit; }
  double get_high_usage_ratio_threshold() const { return _high_usage_ratio_threshold; }
  int32_t get_congestion_risk_radius() const { return _congestion_risk_radius; }
  int32_t get_route_window_base_expand() const { return _route_window_base_expand; }
  int32_t get_route_window_max_expand_times() const { return _route_window_max_expand_times; }
  double get_route_window_expand_ratio() const { return _route_window_expand_ratio; }
  bool get_enable_full_die_fallback() const { return _enable_full_die_fallback; }
  // setter
  void set_max_iter_num(const int32_t max_iter_num) { _max_iter_num = max_iter_num; }
  void set_max_routed_times(const int32_t max_routed_times) { _max_routed_times = max_routed_times; }
  void set_max_task_num(const int32_t max_task_num) { _max_task_num = max_task_num; }
  void set_wire_unit(const double wire_unit) { _wire_unit = wire_unit; }
  void set_corner_unit(const double corner_unit) { _corner_unit = corner_unit; }
  void set_overflow_unit(const double overflow_unit) { _overflow_unit = overflow_unit; }
  void set_congestion_risk_unit(const double congestion_risk_unit) { _congestion_risk_unit = congestion_risk_unit; }
  void set_high_usage_unit(const double high_usage_unit) { _high_usage_unit = high_usage_unit; }
  void set_high_usage_ratio_threshold(const double high_usage_ratio_threshold) { _high_usage_ratio_threshold = high_usage_ratio_threshold; }
  void set_congestion_risk_radius(const int32_t congestion_risk_radius) { _congestion_risk_radius = congestion_risk_radius; }
  void set_route_window_base_expand(const int32_t route_window_base_expand) { _route_window_base_expand = route_window_base_expand; }
  void set_route_window_max_expand_times(const int32_t route_window_max_expand_times) { _route_window_max_expand_times = route_window_max_expand_times; }
  void set_route_window_expand_ratio(const double route_window_expand_ratio) { _route_window_expand_ratio = route_window_expand_ratio; }
  void set_enable_full_die_fallback(const bool enable_full_die_fallback) { _enable_full_die_fallback = enable_full_die_fallback; }

 private:
  int32_t _max_iter_num = 0;
  int32_t _max_routed_times = 0;
  int32_t _max_task_num = 0;
  double _wire_unit = 0.0;
  double _corner_unit = 0.0;
  double _overflow_unit = 0.0;
  double _congestion_risk_unit = 0.0;
  double _high_usage_unit = 0.0;
  double _high_usage_ratio_threshold = 0.0;
  int32_t _congestion_risk_radius = 0;
  int32_t _route_window_base_expand = 0;
  int32_t _route_window_max_expand_times = 0;
  double _route_window_expand_ratio = 0.0;
  bool _enable_full_die_fallback = false;
};

}  // namespace irt
