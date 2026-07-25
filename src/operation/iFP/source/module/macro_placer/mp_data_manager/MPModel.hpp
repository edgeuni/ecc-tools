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

#include "FPHeader.hpp"
#include "MPNet.hpp"
#include "MPNode.hpp"
#include "MPRect.hpp"

namespace ifp {

class MPModel
{
 public:
  MPModel() = default;
  ~MPModel() = default;
  // getter
  MPRect& get_core_rect() { return _core_rect; }
  std::vector<MPNode>& get_mp_node_list() { return _mp_node_list; }
  std::vector<int32_t>& get_movable_node_idx_list() { return _movable_node_idx_list; }
  std::vector<MPNet>& get_mp_net_list() { return _mp_net_list; }
  std::vector<MPRect>& get_blockage_rect_list() { return _blockage_rect_list; }

  // const getter
  const MPRect& get_core_rect() const { return _core_rect; }
  const std::vector<MPNode>& get_mp_node_list() const { return _mp_node_list; }
  const std::vector<int32_t>& get_movable_node_idx_list() const { return _movable_node_idx_list; }
  const std::vector<MPNet>& get_mp_net_list() const { return _mp_net_list; }
  const std::vector<MPRect>& get_blockage_rect_list() const { return _blockage_rect_list; }

  // setter
  void set_core_rect(const MPRect& core_rect) { _core_rect = core_rect; }
  void set_mp_node_list(const std::vector<MPNode>& mp_node_list) { _mp_node_list = mp_node_list; }
  void set_movable_node_idx_list(const std::vector<int32_t>& movable_node_idx_list) { _movable_node_idx_list = movable_node_idx_list; }
  void set_mp_net_list(const std::vector<MPNet>& mp_net_list) { _mp_net_list = mp_net_list; }
  void set_blockage_rect_list(const std::vector<MPRect>& blockage_rect_list) { _blockage_rect_list = blockage_rect_list; }

  // function
  void clear()
  {
    _core_rect = MPRect();
    _mp_node_list.clear();
    _movable_node_idx_list.clear();
    _mp_net_list.clear();
    _blockage_rect_list.clear();
  }

 private:
  MPRect _core_rect;
  std::vector<MPNode> _mp_node_list;
  std::vector<int32_t> _movable_node_idx_list;
  std::vector<MPNet> _mp_net_list;
  std::vector<MPRect> _blockage_rect_list;
};

}  // namespace ifp
