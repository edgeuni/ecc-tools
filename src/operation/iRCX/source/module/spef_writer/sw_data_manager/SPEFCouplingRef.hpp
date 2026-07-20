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
#include "RCXType.hpp"

namespace ircx {

class SPEFCouplingRef
{
 public:
  SPEFCouplingRef() = default;
  SPEFCouplingRef(size_t self_edge_idx, size_t other_edge_idx, F64 capacitance)
  {
    _self_edge_idx = self_edge_idx;
    _other_edge_idx = other_edge_idx;
    _capacitance = capacitance;
  }
  ~SPEFCouplingRef() = default;
  // getter
  size_t get_self_edge_idx() const { return _self_edge_idx; }
  size_t get_other_edge_idx() const { return _other_edge_idx; }
  F64 get_capacitance() const { return _capacitance; }
  // setter
  void set_self_edge_idx(size_t self_edge_idx) { _self_edge_idx = self_edge_idx; }
  void set_other_edge_idx(size_t other_edge_idx) { _other_edge_idx = other_edge_idx; }
  void set_capacitance(F64 capacitance) { _capacitance = capacitance; }
  // function

 private:
  size_t _self_edge_idx = kMaxSize;
  size_t _other_edge_idx = kMaxSize;
  F64 _capacitance = 0.0;
};

}  // namespace ircx
