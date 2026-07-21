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

class TrackInfo
{
 public:
  TrackInfo() = default;
  ~TrackInfo() = default;
  // getter
  int32_t get_x_start() const { return _x_start; }
  int32_t get_y_start() const { return _y_start; }
  int32_t get_x_step() const { return _x_step; }
  int32_t get_y_step() const { return _y_step; }
  size_t get_x_count() const { return _x_count; }
  size_t get_y_count() const { return _y_count; }
  // setter
  void set_x_start(int32_t x_start) { _x_start = x_start; }
  void set_y_start(int32_t y_start) { _y_start = y_start; }
  void set_x_step(int32_t x_step) { _x_step = x_step; }
  void set_y_step(int32_t y_step) { _y_step = y_step; }
  void set_x_count(size_t x_count) { _x_count = x_count; }
  void set_y_count(size_t y_count) { _y_count = y_count; }
  // function

 private:
  int32_t _x_start = 0;
  int32_t _y_start = 0;
  int32_t _x_step = 0;
  int32_t _y_step = 0;
  size_t _x_count = 0;
  size_t _y_count = 0;
};

}  // namespace ircx
