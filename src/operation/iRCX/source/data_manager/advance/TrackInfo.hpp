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

class TrackInfo
{
 public:
  TrackInfo() = default;
  ~TrackInfo() = default;
  // getter
  Dbu get_x_start() const { return _x_start; }
  Dbu get_y_start() const { return _y_start; }
  Dbu get_x_step() const { return _x_step; }
  Dbu get_y_step() const { return _y_step; }
  Size get_x_count() const { return _x_count; }
  Size get_y_count() const { return _y_count; }
  // setter
  void set_x_start(Dbu x_start) { _x_start = x_start; }
  void set_y_start(Dbu y_start) { _y_start = y_start; }
  void set_x_step(Dbu x_step) { _x_step = x_step; }
  void set_y_step(Dbu y_step) { _y_step = y_step; }
  void set_x_count(Size x_count) { _x_count = x_count; }
  void set_y_count(Size y_count) { _y_count = y_count; }
  // function

 private:
  Dbu _x_start = 0;
  Dbu _y_start = 0;
  Dbu _x_step = 0;
  Dbu _y_step = 0;
  Size _x_count = 0;
  Size _y_count = 0;
};

}  // namespace ircx
