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

class CouplingKey
{
 public:
  CouplingKey() = default;
  CouplingKey(Size first_edge_idx, Size second_edge_idx)
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
  Size get_first_edge_idx() const { return _first_edge_idx; }
  Size get_second_edge_idx() const { return _second_edge_idx; }
  // setter
  void set_first_edge_idx(Size first_edge_idx) { _first_edge_idx = first_edge_idx; }
  void set_second_edge_idx(Size second_edge_idx) { _second_edge_idx = second_edge_idx; }
  // function

 private:
  Size _first_edge_idx = kMaxSize;
  Size _second_edge_idx = kMaxSize;
};

class CouplingKeyHash
{
 public:
  CouplingKeyHash() = default;
  ~CouplingKeyHash() = default;
  // getter
  // setter
  // function
  Size operator()(const CouplingKey& coupling_key) const;
};

inline Size CouplingKeyHash::operator()(const CouplingKey& coupling_key) const
{
  Size seed = std::hash<Size>()(coupling_key.get_first_edge_idx());
  Size value = std::hash<Size>()(coupling_key.get_second_edge_idx());
  Size magic = sizeof(Size) == 8 ? static_cast<Size>(0x9e3779b97f4a7c15ull) : static_cast<Size>(0x9e3779b9ul);
  return seed ^ (value + magic + (seed << 6) + (seed >> 2));
}

}  // namespace ircx
