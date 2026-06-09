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

#include "Net.hpp"
#include "RTHeader.hpp"
#include "Segment.hpp"
#include "GSRPin.hpp"
#include "GSRTree.hpp"

namespace irt {

class GSRNet
{
 public:
  GSRNet() = default;
  ~GSRNet() = default;
  // getter
  Net* get_origin_net() { return _origin_net; }
  int32_t get_net_idx() const { return _net_idx; }
  std::string& get_net_name() { return _net_name; }
  std::vector<GSRPin>& get_gsr_pin_list() { return _gsr_pin_list; }
  GSRTree& get_routing_tree() { return _routing_tree; }
  GSRTree& get_best_routing_tree() { return _best_routing_tree; }
  std::vector<Segment<LayerCoord>>& get_routing_segment_list() { return _routing_segment_list; }
  std::vector<Segment<LayerCoord>>& get_best_routing_segment_list() { return _best_routing_segment_list; }
  int32_t get_routed_times() const { return _routed_times; }
  double get_route_overflow() const { return _route_overflow; }
  double get_route_congestion_risk() const { return _route_congestion_risk; }
  // const getter
  const std::string& get_net_name() const { return _net_name; }
  const Net* get_origin_net() const { return _origin_net; }
  const std::vector<GSRPin>& get_gsr_pin_list() const { return _gsr_pin_list; }
  const GSRTree& get_routing_tree() const { return _routing_tree; }
  const GSRTree& get_best_routing_tree() const { return _best_routing_tree; }
  const std::vector<Segment<LayerCoord>>& get_routing_segment_list() const { return _routing_segment_list; }
  const std::vector<Segment<LayerCoord>>& get_best_routing_segment_list() const { return _best_routing_segment_list; }
  // setter
  void set_origin_net(Net* origin_net) { _origin_net = origin_net; }
  void set_net_idx(const int32_t net_idx) { _net_idx = net_idx; }
  void set_net_name(const std::string& net_name) { _net_name = net_name; }
  void set_gsr_pin_list(const std::vector<GSRPin>& gsr_pin_list) { _gsr_pin_list = gsr_pin_list; }
  void set_routing_tree(const GSRTree& routing_tree) { _routing_tree = routing_tree; }
  void set_best_routing_tree(const GSRTree& best_routing_tree) { _best_routing_tree = best_routing_tree; }
  void set_routing_segment_list(const std::vector<Segment<LayerCoord>>& routing_segment_list) { _routing_segment_list = routing_segment_list; }
  void set_best_routing_segment_list(const std::vector<Segment<LayerCoord>>& best_routing_segment_list)
  {
    _best_routing_segment_list = best_routing_segment_list;
  }
  void set_routed_times(const int32_t routed_times) { _routed_times = routed_times; }
  void set_route_overflow(const double route_overflow) { _route_overflow = route_overflow; }
  void set_route_congestion_risk(const double route_congestion_risk) { _route_congestion_risk = route_congestion_risk; }
  // function
  void addRoutedTimes() { _routed_times++; }

 private:
  Net* _origin_net = nullptr;
  int32_t _net_idx = -1;
  std::string _net_name;
  std::vector<GSRPin> _gsr_pin_list;
  GSRTree _routing_tree;
  GSRTree _best_routing_tree;
  std::vector<Segment<LayerCoord>> _routing_segment_list;
  std::vector<Segment<LayerCoord>> _best_routing_segment_list;
  int32_t _routed_times = 0;
  double _route_overflow = 0;
  double _route_congestion_risk = 0;
};

}  // namespace irt
