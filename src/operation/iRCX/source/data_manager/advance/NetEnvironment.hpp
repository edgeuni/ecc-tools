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

#include "EdgeEnvironmentInterval.hpp"
#include "GroupPool.hpp"

namespace ircx {

class NetEnvironment
{
 public:
  NetEnvironment() = default;
  ~NetEnvironment() = default;
  // getter
  GroupPool<EdgeEnvironmentInterval>& get_edge_interval_pool() { return _edge_interval_pool; }
  // setter
  void set_edge_interval_pool(const GroupPool<EdgeEnvironmentInterval>& edge_interval_pool) { _edge_interval_pool = edge_interval_pool; }
  // function
  void append_edge_interval_list(std::vector<EdgeEnvironmentInterval> edge_interval_list)
  {
    _edge_interval_pool.append(std::move(edge_interval_list));
  }
  std::span<EdgeEnvironmentInterval> get_edge_interval_list(Size edge_idx) { return _edge_interval_pool.get_group_item_list(edge_idx); }
  std::span<const EdgeEnvironmentInterval> get_edge_interval_list(Size edge_idx) const
  {
    return _edge_interval_pool.get_group_item_list(edge_idx);
  }

 private:
  GroupPool<EdgeEnvironmentInterval> _edge_interval_pool;
};

}  // namespace ircx
