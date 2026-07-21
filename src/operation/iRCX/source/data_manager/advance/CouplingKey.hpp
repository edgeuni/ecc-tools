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

#include "RCXHeader.hpp"

namespace ircx {

class CouplingKey
{
 public:
  CouplingKey() = default;
  CouplingKey(size_t first_edge_idx, size_t second_edge_idx)
  {
    _first_edge_idx = std::min(first_edge_idx, second_edge_idx);
    _second_edge_idx = std::max(first_edge_idx, second_edge_idx);
  }
  ~CouplingKey() = default;
  bool operator==(const CouplingKey& other) const
  {
    return _first_edge_idx == other._first_edge_idx && _second_edge_idx == other._second_edge_idx;
  }
  // getter
  size_t get_first_edge_idx() const { return _first_edge_idx; }
  size_t get_second_edge_idx() const { return _second_edge_idx; }
  // setter
  void set_first_edge_idx(size_t first_edge_idx) { _first_edge_idx = first_edge_idx; }
  void set_second_edge_idx(size_t second_edge_idx) { _second_edge_idx = second_edge_idx; }
  // function

 private:
  size_t _first_edge_idx = SIZE_MAX;
  size_t _second_edge_idx = SIZE_MAX;
};

}  // namespace ircx
