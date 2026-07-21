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

#include "RCXType.hpp"

namespace ircx {

class CcapEntry
{
 public:
  CcapEntry() = default;
  CcapEntry(size_t first_edge_idx, size_t second_edge_idx, size_t corner_idx, F32 cap)
  {
    _first_edge_idx = first_edge_idx;
    _second_edge_idx = second_edge_idx;
    _corner_idx = corner_idx;
    _cap = cap;
  }
  ~CcapEntry() = default;
  // getter
  size_t get_first_edge_idx() const { return _first_edge_idx; }
  size_t get_second_edge_idx() const { return _second_edge_idx; }
  size_t get_corner_idx() const { return _corner_idx; }
  F32 get_cap() const { return _cap; }
  // setter
  void set_first_edge_idx(size_t first_edge_idx) { _first_edge_idx = first_edge_idx; }
  void set_second_edge_idx(size_t second_edge_idx) { _second_edge_idx = second_edge_idx; }
  void set_corner_idx(size_t corner_idx) { _corner_idx = corner_idx; }
  void set_cap(F32 cap) { _cap = cap; }
  // function

 private:
  size_t _first_edge_idx = kMaxSize;
  size_t _second_edge_idx = kMaxSize;
  size_t _corner_idx = kMaxSize;
  F32 _cap = 0.0F;
};

}  // namespace ircx
