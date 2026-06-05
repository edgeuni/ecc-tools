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
// INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "ChangeType.hpp"
#include "LayerCoord.hpp"
#include "Orientation.hpp"
#include "RTHeader.hpp"
#include "Utility.hpp"

namespace irt {

class GSRNode : public LayerCoord
{
 public:
  GSRNode() = default;
  ~GSRNode() = default;
  // getter
  double get_boundary_wire_unit() const { return _boundary_wire_unit; }
  double get_internal_wire_unit() const { return _internal_wire_unit; }
  double get_internal_via_unit() const { return _internal_via_unit; }
  std::map<Orientation, int32_t>& get_orient_supply_map() { return _orient_supply_map; }
  std::map<Orientation, std::set<int32_t>>& get_orient_net_map() { return _orient_net_map; }
  std::map<int32_t, std::set<Orientation>>& get_net_orient_map() { return _net_orient_map; }
  double get_congestion_risk() const { return _congestion_risk; }
  // const getter
  const std::map<Orientation, int32_t>& get_orient_supply_map() const { return _orient_supply_map; }
  const std::map<Orientation, std::set<int32_t>>& get_orient_net_map() const { return _orient_net_map; }
  const std::map<int32_t, std::set<Orientation>>& get_net_orient_map() const { return _net_orient_map; }
  // setter
  void set_boundary_wire_unit(const double boundary_wire_unit) { _boundary_wire_unit = boundary_wire_unit; }
  void set_internal_wire_unit(const double internal_wire_unit) { _internal_wire_unit = internal_wire_unit; }
  void set_internal_via_unit(const double internal_via_unit) { _internal_via_unit = internal_via_unit; }
  void set_orient_supply_map(const std::map<Orientation, int32_t>& orient_supply_map) { _orient_supply_map = orient_supply_map; }
  void set_congestion_risk(const double congestion_risk) { _congestion_risk = congestion_risk; }
  // function
  void updateDemand(const int32_t net_idx, const std::set<Orientation>& orient_set, const ChangeType change_type)
  {
    for (const Orientation& orient : orient_set) {
      if (change_type == ChangeType::kAdd) {
        _orient_net_map[orient].insert(net_idx);
        _net_orient_map[net_idx].insert(orient);
      } else {
        if (RTUTIL.exist(_orient_net_map, orient)) {
          _orient_net_map[orient].erase(net_idx);
          if (_orient_net_map[orient].empty()) {
            _orient_net_map.erase(orient);
          }
        }
        if (RTUTIL.exist(_net_orient_map, net_idx)) {
          _net_orient_map[net_idx].erase(orient);
          if (_net_orient_map[net_idx].empty()) {
            _net_orient_map.erase(net_idx);
          }
        }
      }
    }
  }
  double getSupply(const Orientation orient) const
  {
    if (RTUTIL.exist(_orient_supply_map, orient)) {
      return static_cast<double>(_orient_supply_map.at(orient));
    }
    return 0.0;
  }
  double getInternalSupply() const
  {
    double supply = 0;
    for (auto& [orient, orient_supply] : _orient_supply_map) {
      if (isPlanarOrientation(orient)) {
        supply += orient_supply;
      }
    }
    return supply;
  }
  double getBoundaryDemand(const Orientation orient, const int32_t extra_net_idx = -1, const bool add_extra = false) const
  {
    int32_t net_num = 0;
    auto iter = _orient_net_map.find(orient);
    if (iter != _orient_net_map.end()) {
      net_num = static_cast<int32_t>(iter->second.size());
      if (add_extra && extra_net_idx >= 0 && iter->second.find(extra_net_idx) == iter->second.end()) {
        net_num++;
      }
    } else if (add_extra && extra_net_idx >= 0) {
      net_num = 1;
    }
    return static_cast<double>(net_num) * _boundary_wire_unit;
  }
  double getInternalDemand(const int32_t extra_net_idx = -1, const std::set<Orientation>* extra_orient_set = nullptr) const
  {
    double demand = 0;
    for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
      int32_t net_num = 0;
      auto iter = _orient_net_map.find(orient);
      if (iter != _orient_net_map.end()) {
        net_num = static_cast<int32_t>(iter->second.size());
        if (extra_net_idx >= 0 && extra_orient_set != nullptr && RTUTIL.exist(*extra_orient_set, orient)
            && iter->second.find(extra_net_idx) == iter->second.end()) {
          net_num++;
        }
      } else if (extra_net_idx >= 0 && extra_orient_set != nullptr && RTUTIL.exist(*extra_orient_set, orient)) {
        net_num = 1;
      }
      demand += static_cast<double>(net_num) * _internal_wire_unit;
    }
    for (auto& [net_idx, orient_set] : _net_orient_map) {
      (void) net_idx;
      if (hasPlanarOrientation(orient_set)) {
        continue;
      }
      if (hasViaOrientation(orient_set)) {
        demand += _internal_via_unit;
      }
    }
    if (extra_net_idx >= 0 && extra_orient_set != nullptr) {
      auto iter = _net_orient_map.find(extra_net_idx);
      bool old_via_only = false;
      bool new_has_planar = hasPlanarOrientation(*extra_orient_set);
      bool new_has_via = hasViaOrientation(*extra_orient_set);
      if (iter != _net_orient_map.end()) {
        old_via_only = !hasPlanarOrientation(iter->second) && hasViaOrientation(iter->second);
        new_has_planar = new_has_planar || hasPlanarOrientation(iter->second);
        new_has_via = new_has_via || hasViaOrientation(iter->second);
      }
      bool new_via_only = !new_has_planar && new_has_via;
      if (old_via_only && !new_via_only) {
        demand -= _internal_via_unit;
      } else if (!old_via_only && new_via_only) {
        demand += _internal_via_unit;
      }
    }
    return demand;
  }
  double getOverflow() const
  {
    double overflow = 0;
    for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
      overflow += std::max(0.0, getBoundaryDemand(orient) - getSupply(orient));
    }
    overflow += std::max(0.0, getInternalDemand() - getInternalSupply());
    return overflow;
  }
  std::set<int32_t> getOverflowNetSet() const
  {
    std::set<int32_t> overflow_net_set;
    for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
      if (getBoundaryDemand(orient) <= getSupply(orient)) {
        continue;
      }
      if (RTUTIL.exist(_orient_net_map, orient)) {
        overflow_net_set.insert(_orient_net_map.at(orient).begin(), _orient_net_map.at(orient).end());
      }
    }
    if (getInternalDemand() > getInternalSupply()) {
      for (auto& [net_idx, orient_set] : _net_orient_map) {
        (void) orient_set;
        overflow_net_set.insert(net_idx);
      }
    }
    return overflow_net_set;
  }
  double getMaxUsageRatio() const
  {
    double max_usage_ratio = 0;
    for (Orientation orient : {Orientation::kEast, Orientation::kWest, Orientation::kSouth, Orientation::kNorth}) {
      max_usage_ratio = std::max(max_usage_ratio, calcUsageRatio(getBoundaryDemand(orient), getSupply(orient)));
    }
    max_usage_ratio = std::max(max_usage_ratio, calcUsageRatio(getInternalDemand(), getInternalSupply()));
    return max_usage_ratio;
  }
 private:
  double _boundary_wire_unit = -1;
  double _internal_wire_unit = -1;
  double _internal_via_unit = -1;
  std::map<Orientation, int32_t> _orient_supply_map;
  std::map<Orientation, std::set<int32_t>> _orient_net_map;
  std::map<int32_t, std::set<Orientation>> _net_orient_map;
  double _congestion_risk = 0;

  static bool isPlanarOrientation(const Orientation orient)
  {
    return orient == Orientation::kEast || orient == Orientation::kWest || orient == Orientation::kSouth || orient == Orientation::kNorth;
  }
  static bool isViaOrientation(const Orientation orient) { return orient == Orientation::kAbove || orient == Orientation::kBelow; }
  static bool hasPlanarOrientation(const std::set<Orientation>& orient_set)
  {
    for (Orientation orient : orient_set) {
      if (isPlanarOrientation(orient)) {
        return true;
      }
    }
    return false;
  }
  static bool hasViaOrientation(const std::set<Orientation>& orient_set)
  {
    for (Orientation orient : orient_set) {
      if (isViaOrientation(orient)) {
        return true;
      }
    }
    return false;
  }
  static double calcUsageRatio(const double demand, const double supply)
  {
    if (supply <= 0) {
      return demand > 0 ? 1.0e6 : 0.0;
    }
    return demand / supply;
  }
};

}  // namespace irt
