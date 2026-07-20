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

#include "TopoEdge.hpp"
#include "TopoNode.hpp"

namespace ircx {

class TopoPool
{
 public:
  TopoPool() = default;
  ~TopoPool() = default;
  // getter
  std::vector<TopoNode>& get_node_pool() { return _node_pool; }
  std::vector<TopoEdge>& get_edge_pool() { return _edge_pool; }
  std::vector<TopoEdge>& get_special_edge_pool() { return _special_edge_pool; }
  TopoNode& get_node(Size node_idx) { return _node_pool.at(node_idx); }
  TopoEdge& get_edge(Size edge_idx) { return _edge_pool.at(edge_idx); }
  std::span<TopoNode> get_net_node_list(Size net_id);
  std::span<TopoEdge> get_net_edge_list(Size net_id);
  std::pair<Size, Size> get_net_node_range(Size net_id) { return _net_node_range_list.at(net_id); }
  std::pair<Size, Size> get_net_edge_range(Size net_id) { return _net_edge_range_list.at(net_id); }
  // setter
  // function
  Size get_node_idx(TopoNode& topo_node) { return static_cast<Size>(&topo_node - _node_pool.data()); }
  Size get_edge_idx(TopoEdge& topo_edge) { return static_cast<Size>(&topo_edge - _edge_pool.data()); }
  Size get_node_idx(Size net_id, Size local_node_idx) { return _net_node_range_list.at(net_id).first + local_node_idx; }
  Size get_edge_idx(Size net_id, Size local_edge_idx) { return _net_edge_range_list.at(net_id).first + local_edge_idx; }
  void reserve(Size net_count, Size node_count, Size edge_count);
  void add_net(std::vector<TopoNode> node_list, std::vector<TopoEdge> edge_list);
  void add_special_edge_list(std::vector<TopoEdge> edge_list);

 private:
  void assign_node_id(std::vector<TopoNode>& node_list);
  void assign_edge_id(std::vector<TopoEdge>& edge_list);

  std::vector<TopoNode> _node_pool;
  std::vector<std::pair<Size, Size>> _net_node_range_list;
  std::vector<TopoEdge> _edge_pool;
  std::vector<std::pair<Size, Size>> _net_edge_range_list;
  std::vector<TopoEdge> _special_edge_pool;
};

inline std::span<TopoNode> TopoPool::get_net_node_list(Size net_id)
{
  std::pair<Size, Size> net_node_range = _net_node_range_list.at(net_id);
  return std::span<TopoNode>(_node_pool.data() + net_node_range.first, net_node_range.second);
}

inline std::span<TopoEdge> TopoPool::get_net_edge_list(Size net_id)
{
  std::pair<Size, Size> net_edge_range = _net_edge_range_list.at(net_id);
  return std::span<TopoEdge>(_edge_pool.data() + net_edge_range.first, net_edge_range.second);
}

inline void TopoPool::reserve(Size net_count, Size node_count, Size edge_count)
{
  _net_node_range_list.reserve(net_count);
  _net_edge_range_list.reserve(net_count);
  _node_pool.reserve(node_count);
  _edge_pool.reserve(edge_count);
}

inline void TopoPool::add_net(std::vector<TopoNode> node_list, std::vector<TopoEdge> edge_list)
{
  Size node_offset = _node_pool.size();
  Size node_count = node_list.size();
  Size edge_offset = _edge_pool.size();
  Size edge_count = edge_list.size();

  assign_node_id(node_list);
  assign_edge_id(edge_list);
  _net_node_range_list.emplace_back(node_offset, node_count);
  _net_edge_range_list.emplace_back(edge_offset, edge_count);
  _node_pool.insert(_node_pool.end(), std::make_move_iterator(node_list.begin()), std::make_move_iterator(node_list.end()));
  _edge_pool.insert(_edge_pool.end(), std::make_move_iterator(edge_list.begin()), std::make_move_iterator(edge_list.end()));
}

inline void TopoPool::add_special_edge_list(std::vector<TopoEdge> edge_list)
{
  assign_edge_id(edge_list);
  _special_edge_pool.insert(_special_edge_pool.end(), std::make_move_iterator(edge_list.begin()), std::make_move_iterator(edge_list.end()));
}

inline void TopoPool::assign_node_id(std::vector<TopoNode>& node_list)
{
  for (Size node_idx = 0; node_idx < node_list.size(); ++node_idx) {
    node_list[node_idx].set_node_id(node_idx);
  }
}

inline void TopoPool::assign_edge_id(std::vector<TopoEdge>& edge_list)
{
  for (Size edge_idx = 0; edge_idx < edge_list.size(); ++edge_idx) {
    edge_list[edge_idx].set_edge_id(edge_idx);
  }
}

}  // namespace ircx
