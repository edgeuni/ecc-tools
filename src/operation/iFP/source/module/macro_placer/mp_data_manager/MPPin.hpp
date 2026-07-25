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
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "FPHeader.hpp"

namespace ifp {

class MPPin
{
 public:
  MPPin() = default;
  ~MPPin() = default;
  // getter
  int32_t get_node_idx() const { return _node_idx; }
  int32_t get_offset_x() const { return _offset_x; }
  int32_t get_offset_y() const { return _offset_y; }
  int32_t get_x() const { return _x; }
  int32_t get_y() const { return _y; }
  bool get_io() const { return _io; }
  // const getter

  // setter
  void set_node_idx(int32_t node_idx) { _node_idx = node_idx; }
  void set_offset_x(int32_t offset_x) { _offset_x = offset_x; }
  void set_offset_y(int32_t offset_y) { _offset_y = offset_y; }
  void set_x(int32_t x) { _x = x; }
  void set_y(int32_t y) { _y = y; }
  void set_coord(int32_t x, int32_t y)
  {
    _x = x;
    _y = y;
  }
  void set_io(bool io) { _io = io; }
  // function
  bool is_node_pin() const { return _node_idx >= 0; }

 private:
  int32_t _node_idx = -1;
  int32_t _offset_x = 0;
  int32_t _offset_y = 0;
  int32_t _x = 0;
  int32_t _y = 0;
  bool _io = false;
};

}  // namespace ifp
