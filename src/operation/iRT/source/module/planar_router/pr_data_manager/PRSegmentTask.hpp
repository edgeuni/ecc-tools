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

#include <map>

#include "ConnectType.hpp"
#include "LayerCoord.hpp"
#include "Segment.hpp"

namespace irt {

class PRSegmentTask
{
 public:
  PRSegmentTask() = default;
  ~PRSegmentTask() = default;
  // getter
  int32_t get_net_idx() const { return _net_idx; }
  ConnectType get_connect_type() const { return _connect_type; }
  Segment<LayerCoord>* get_origin_segment() const { return _origin_segment; }
  Segment<PlanarCoord>& get_planar_segment() { return _planar_segment; }
  int32_t get_routed_times() const { return _routed_times; }
  double get_overflow() const { return _overflow; }
  double get_congestion_risk() const { return _congestion_risk; }
  double get_high_usage() const { return _high_usage; }
  double get_max_usage_ratio() const { return _max_usage_ratio; }
  double get_wire_length() const { return _wire_length; }
  std::map<PlanarCoord, double, CmpPlanarCoordByXASC>& get_origin_overflow_penalty_map() { return _origin_overflow_penalty_map; }
  const std::map<PlanarCoord, double, CmpPlanarCoordByXASC>& get_origin_overflow_penalty_map() const
  {
    return _origin_overflow_penalty_map;
  }
  std::map<PlanarCoord, double, CmpPlanarCoordByXASC>& get_origin_high_usage_penalty_map() { return _origin_high_usage_penalty_map; }
  const std::map<PlanarCoord, double, CmpPlanarCoordByXASC>& get_origin_high_usage_penalty_map() const
  {
    return _origin_high_usage_penalty_map;
  }
  // setter
  void set_net_idx(const int32_t net_idx) { _net_idx = net_idx; }
  void set_connect_type(const ConnectType connect_type) { _connect_type = connect_type; }
  void set_origin_segment(Segment<LayerCoord>* origin_segment) { _origin_segment = origin_segment; }
  void set_planar_segment(const Segment<PlanarCoord>& planar_segment) { _planar_segment = planar_segment; }
  void set_routed_times(const int32_t routed_times) { _routed_times = routed_times; }
  void set_overflow(const double overflow) { _overflow = overflow; }
  void set_congestion_risk(const double congestion_risk) { _congestion_risk = congestion_risk; }
  void set_high_usage(const double high_usage) { _high_usage = high_usage; }
  void set_max_usage_ratio(const double max_usage_ratio) { _max_usage_ratio = max_usage_ratio; }
  void set_wire_length(const double wire_length) { _wire_length = wire_length; }
  void set_origin_overflow_penalty_map(const std::map<PlanarCoord, double, CmpPlanarCoordByXASC>& origin_overflow_penalty_map)
  {
    _origin_overflow_penalty_map = origin_overflow_penalty_map;
  }
  void set_origin_high_usage_penalty_map(const std::map<PlanarCoord, double, CmpPlanarCoordByXASC>& origin_high_usage_penalty_map)
  {
    _origin_high_usage_penalty_map = origin_high_usage_penalty_map;
  }
  // function
  void addRoutedTimes() { ++_routed_times; }

 private:
  int32_t _net_idx = -1;
  ConnectType _connect_type = ConnectType::kNone;
  Segment<LayerCoord>* _origin_segment = nullptr;
  Segment<PlanarCoord> _planar_segment;
  int32_t _routed_times = 0;
  double _overflow = 0.0;
  double _congestion_risk = 0.0;
  double _high_usage = 0.0;
  double _max_usage_ratio = 0.0;
  double _wire_length = 0.0;
  std::map<PlanarCoord, double, CmpPlanarCoordByXASC> _origin_overflow_penalty_map;
  std::map<PlanarCoord, double, CmpPlanarCoordByXASC> _origin_high_usage_penalty_map;
};

}  // namespace irt
