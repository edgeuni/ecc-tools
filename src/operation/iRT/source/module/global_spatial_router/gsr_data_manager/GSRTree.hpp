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

#include "LayerCoord.hpp"
#include "PlanarCoord.hpp"
#include "RTHeader.hpp"
#include "Segment.hpp"

namespace irt {

struct GSRTreeNode
{
  PlanarCoord coord;
  std::pair<int32_t, int32_t> fixed_layer_interval = {-1, -1};
  std::vector<int32_t> child_idx_list;

  GSRTreeNode() = default;
  explicit GSRTreeNode(const PlanarCoord& coord) : coord(coord) {}
};

class GSRTree
{
 public:
  GSRTree() = default;
  ~GSRTree() = default;
  // getter
  int32_t get_root_idx() const { return _root_idx; }
  std::vector<GSRTreeNode>& get_node_list() { return _node_list; }
  std::vector<Segment<LayerCoord>>& get_segment_list() { return _segment_list; }
  const std::vector<GSRTreeNode>& get_node_list() const { return _node_list; }
  const std::vector<Segment<LayerCoord>>& get_segment_list() const { return _segment_list; }
  // setter
  void set_root_idx(const int32_t root_idx) { _root_idx = root_idx; }
  void set_node_list(const std::vector<GSRTreeNode>& node_list) { _node_list = node_list; }
  void set_segment_list(const std::vector<Segment<LayerCoord>>& segment_list) { _segment_list = segment_list; }
  // function
  bool empty() const { return _node_list.empty() && _segment_list.empty(); }

 private:
  int32_t _root_idx = -1;
  std::vector<GSRTreeNode> _node_list;
  std::vector<Segment<LayerCoord>> _segment_list;
};

}  // namespace irt
