// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
//
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
// ***************************************************************************************
#pragma once

#include "Orientation.hpp"
#include "RTHeader.hpp"

namespace irt {

class RoutingEdge
{
 public:
  RoutingEdge() = default;
  ~RoutingEdge() = default;
  // getter
  int32_t get_supply() const { return _supply; }
  std::map<Orientation, int32_t>& get_orient_demand_map() { return _orient_demand_map; }
  std::set<int32_t>& get_ignore_net_set() { return _ignore_net_set; }
  double get_congestion_cost() const { return _congestion_cost; }
  int32_t get_usage() const
  {
    int32_t usage = 0;
    for (auto& [orient, demand] : _orient_demand_map) {
      usage = std::max(usage, demand);
    }
    return usage;
  }
  int32_t get_overflow() const { return std::max(0, get_usage() - _supply); }
  // setter
  void set_supply(const int32_t supply) { _supply = supply; }
  void set_orient_demand_map(const std::map<Orientation, int32_t>& orient_demand_map) { _orient_demand_map = orient_demand_map; }
  void set_ignore_net_set(const std::set<int32_t>& ignore_net_set) { _ignore_net_set = ignore_net_set; }
  void set_congestion_cost(const double congestion_cost) { _congestion_cost = congestion_cost; }

 private:
  int32_t _supply = 0;
  std::map<Orientation, int32_t> _orient_demand_map;
  std::set<int32_t> _ignore_net_set;
  double _congestion_cost = 0;
};

}  // namespace irt
