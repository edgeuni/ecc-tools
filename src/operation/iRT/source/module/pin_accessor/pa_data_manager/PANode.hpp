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
#include "Utility.hpp"
#include "ViaMasterIdx.hpp"

namespace irt {

#if 1  // astar
enum class PANodeState
{
  kNone = 0,
  kOpen = 1,
  kClose = 2
};
#endif

class PANode : public LayerCoord
{
 public:
  PANode() = default;
  ~PANode() = default;
  // getter
  std::map<Orientation, std::set<int32_t>>& get_orient_fixed_rect_map() { return _orient_fixed_rect_map; }
  std::map<Orientation, std::set<int32_t>>& get_orient_routed_rect_map() { return _orient_routed_rect_map; }
  std::map<Orientation, int32_t>& get_orient_violation_number_map() { return _orient_violation_number_map; }
  // setter
  void set_neighbor_node_map(const std::map<Orientation, PANode*>& neighbor_node_map)
  {
    clearNeighborNodeList();
    for (auto& [orientation, neighbor_node] : neighbor_node_map) {
      setNeighborNode(orientation, neighbor_node);
    }
  }
  void set_orient_fixed_rect_map(const std::map<Orientation, std::set<int32_t>>& orient_fixed_rect_map) { _orient_fixed_rect_map = orient_fixed_rect_map; }
  void set_orient_routed_rect_map(const std::map<Orientation, std::set<int32_t>>& orient_routed_rect_map) { _orient_routed_rect_map = orient_routed_rect_map; }
  void set_orient_violation_number_map(const std::map<Orientation, int32_t>& orient_violation_number_map)
  {
    _orient_violation_number_map = orient_violation_number_map;
  }
  // function
  void setNeighborNode(Orientation orientation, PANode* neighbor_node)
  {
    if (!isNeighborOrientation(orientation)) {
      RTLOG.error(Loc::current(), "The neighbor orientation is invalid!");
      return;
    }
    PANode*& exist_neighbor_node = _neighbor_node_list[getNeighborOrientationIdx(orientation)];
    if (exist_neighbor_node == nullptr && neighbor_node != nullptr) {
      _neighbor_node_num++;
    } else if (exist_neighbor_node != nullptr && neighbor_node == nullptr) {
      _neighbor_node_num--;
    }
    exist_neighbor_node = neighbor_node;
  }
  PANode* getNeighborNode(Orientation orientation) const
  {
    if (!isNeighborOrientation(orientation)) {
      return nullptr;
    }
    return _neighbor_node_list[getNeighborOrientationIdx(orientation)];
  }
  int32_t get_neighbor_node_num() const { return _neighbor_node_num; }
  bool hasNeighborNode(Orientation orientation) const { return getNeighborNode(orientation) != nullptr; }
  bool hasAnyNeighborNode() const { return _neighbor_node_num > 0; }
  template <typename Func>
  void forEachNeighborNode(Func func) const
  {
    for (int32_t orient_idx = static_cast<int32_t>(Orientation::kEast); orient_idx <= static_cast<int32_t>(Orientation::kBelow); orient_idx++) {
      Orientation orientation = static_cast<Orientation>(orient_idx);
      PANode* neighbor_node = _neighbor_node_list[getNeighborOrientationIdx(orientation)];
      if (neighbor_node != nullptr) {
        func(orientation, neighbor_node);
      }
    }
  }
  double getFixedRectCost(int32_t net_idx, Orientation orientation, double fixed_rect_unit)
  {
    int32_t fixed_rect_num = 0;
    if (RTUTIL.exist(_orient_fixed_rect_map, orientation)) {
      std::set<int32_t>& net_set = _orient_fixed_rect_map[orientation];
      fixed_rect_num = static_cast<int32_t>(net_set.size());
      if (RTUTIL.exist(net_set, net_idx)) {
        fixed_rect_num--;
      }
      if (fixed_rect_num < 0) {
        RTLOG.error(Loc::current(), "The fixed_rect_num < 0!");
      }
    }
    double cost = 0;
    if (fixed_rect_num > 0) {
      cost = fixed_rect_unit;
    }
    return cost;
  }
  double getRoutedRectCost(int32_t net_idx, Orientation orientation, double routed_rect_unit)
  {
    int32_t routed_rect_num = 0;
    if (RTUTIL.exist(_orient_routed_rect_map, orientation)) {
      std::set<int32_t>& net_set = _orient_routed_rect_map[orientation];
      routed_rect_num = static_cast<int32_t>(net_set.size());
      if (RTUTIL.exist(net_set, net_idx)) {
        routed_rect_num--;
      }
      if (routed_rect_num < 0) {
        RTLOG.error(Loc::current(), "The routed_rect_num < 0!");
      }
    }
    double cost = 0;
    if (routed_rect_num > 0) {
      cost = routed_rect_unit;
    }
    return cost;
  }
  double getViolationCost(Orientation orientation, double violation_unit)
  {
    int32_t violation_num = 0;
    if (RTUTIL.exist(_orient_violation_number_map, orientation)) {
      violation_num = _orient_violation_number_map[orientation];
    }
    double cost = 0;
    if (violation_num > 0) {
      cost = violation_unit;
    }
    return cost;
  }
#if 1  // astar
  // single path
  PANodeState& get_state() { return _state; }
  PANode* get_parent_node() const { return _parent_node; }
  ViaMasterIdx& get_parent_via_master_idx() { return _parent_via_master_idx; }
  double get_known_cost() const { return _known_cost; }
  double get_estimated_cost() const { return _estimated_cost; }
  void set_state(PANodeState state) { _state = state; }
  void set_parent_node(PANode* parent_node) { _parent_node = parent_node; }
  void set_parent_via_master_idx(const ViaMasterIdx& parent_via_master_idx) { _parent_via_master_idx = parent_via_master_idx; }
  void set_known_cost(const double known_cost) { _known_cost = known_cost; }
  void set_estimated_cost(const double estimated_cost) { _estimated_cost = estimated_cost; }
  // function
  bool isNone() { return _state == PANodeState::kNone; }
  bool isOpen() { return _state == PANodeState::kOpen; }
  bool isClose() { return _state == PANodeState::kClose; }
  double getTotalCost() { return (_known_cost + _estimated_cost); }
#endif

 private:
  static constexpr int32_t kNeighborOrientationNum = static_cast<int32_t>(Orientation::kBelow) - static_cast<int32_t>(Orientation::kEast) + 1;
  static constexpr int32_t getNeighborOrientationIdx(Orientation orientation)
  {
    return static_cast<int32_t>(orientation) - static_cast<int32_t>(Orientation::kEast);
  }
  static constexpr bool isNeighborOrientation(Orientation orientation)
  {
    return (static_cast<int32_t>(Orientation::kEast) <= static_cast<int32_t>(orientation)
            && static_cast<int32_t>(orientation) <= static_cast<int32_t>(Orientation::kBelow));
  }
  void clearNeighborNodeList()
  {
    _neighbor_node_list.fill(nullptr);
    _neighbor_node_num = 0;
  }

  std::array<PANode*, kNeighborOrientationNum> _neighbor_node_list = {};
  int32_t _neighbor_node_num = 0;
  // obstacle & pin_shape
  std::map<Orientation, std::set<int32_t>> _orient_fixed_rect_map;
  // net_result
  std::map<Orientation, std::set<int32_t>> _orient_routed_rect_map;
  // violation
  std::map<Orientation, int32_t> _orient_violation_number_map;
#if 1  // astar
  // single path
  PANodeState _state = PANodeState::kNone;
  PANode* _parent_node = nullptr;
  ViaMasterIdx _parent_via_master_idx;
  double _known_cost = 0.0;  // include curr
  double _estimated_cost = 0.0;
#endif
};

#if 1  // astar
struct CmpPANodeCost
{
  bool operator()(PANode* a, PANode* b)
  {
    if (RTUTIL.equalDoubleByError(a->getTotalCost(), b->getTotalCost(), RT_ERROR)) {
      if (RTUTIL.equalDoubleByError(a->get_estimated_cost(), b->get_estimated_cost(), RT_ERROR)) {
        return a->get_neighbor_node_num() < b->get_neighbor_node_num();
      } else {
        return a->get_estimated_cost() > b->get_estimated_cost();
      }
    } else {
      return a->getTotalCost() > b->getTotalCost();
    }
  }
};
#endif

}  // namespace irt
