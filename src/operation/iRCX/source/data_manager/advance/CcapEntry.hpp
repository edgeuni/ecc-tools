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
  CcapEntry(Size first_edge_idx, Size second_edge_idx, Size corner_idx, F32 capacitance)
  {
    _first_edge_idx = first_edge_idx;
    _second_edge_idx = second_edge_idx;
    _corner_idx = corner_idx;
    _capacitance = capacitance;
  }
  ~CcapEntry() = default;
  // getter
  Size get_first_edge_idx() const { return _first_edge_idx; }
  Size get_second_edge_idx() const { return _second_edge_idx; }
  Size get_corner_idx() const { return _corner_idx; }
  F32 get_capacitance() const { return _capacitance; }
  // setter
  void set_first_edge_idx(Size first_edge_idx) { _first_edge_idx = first_edge_idx; }
  void set_second_edge_idx(Size second_edge_idx) { _second_edge_idx = second_edge_idx; }
  void set_corner_idx(Size corner_idx) { _corner_idx = corner_idx; }
  void set_capacitance(F32 capacitance) { _capacitance = capacitance; }
  // function

 private:
  Size _first_edge_idx = kMaxSize;
  Size _second_edge_idx = kMaxSize;
  Size _corner_idx = kMaxSize;
  F32 _capacitance = 0.0F;
};

}  // namespace ircx
