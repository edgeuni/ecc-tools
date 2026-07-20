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

class CornerNetId
{
 public:
  CornerNetId() = default;
  CornerNetId(Size corner_idx, Size net_idx)
  {
    _corner_idx = corner_idx;
    _net_idx = net_idx;
  }
  ~CornerNetId() = default;
  // getter
  Size get_corner_idx() const { return _corner_idx; }
  Size get_net_idx() const { return _net_idx; }
  // setter
  void set_corner_idx(Size corner_idx) { _corner_idx = corner_idx; }
  void set_net_idx(Size net_idx) { _net_idx = net_idx; }
  // function

 private:
  Size _corner_idx = kMaxSize;
  Size _net_idx = kMaxSize;
};

}  // namespace ircx
