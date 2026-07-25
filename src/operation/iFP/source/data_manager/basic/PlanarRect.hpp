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

class PlanarRect
{
 public:
  PlanarRect() = default;
  PlanarRect(int32_t ll_x, int32_t ll_y, int32_t ur_x, int32_t ur_y)
  {
    _ll_x = ll_x;
    _ll_y = ll_y;
    _ur_x = ur_x;
    _ur_y = ur_y;
  }
  ~PlanarRect() = default;
  // getter
  int32_t get_ll_x() const { return _ll_x; }
  int32_t get_ll_y() const { return _ll_y; }
  int32_t get_ur_x() const { return _ur_x; }
  int32_t get_ur_y() const { return _ur_y; }
  int32_t get_width() const { return _ur_x - _ll_x; }
  int32_t get_height() const { return _ur_y - _ll_y; }
  // setter
  void set_ll_x(int32_t ll_x) { _ll_x = ll_x; }
  void set_ll_y(int32_t ll_y) { _ll_y = ll_y; }
  void set_ur_x(int32_t ur_x) { _ur_x = ur_x; }
  void set_ur_y(int32_t ur_y) { _ur_y = ur_y; }
  void set_ll(int32_t ll_x, int32_t ll_y)
  {
    _ll_x = ll_x;
    _ll_y = ll_y;
  }
  void set_ur(int32_t ur_x, int32_t ur_y)
  {
    _ur_x = ur_x;
    _ur_y = ur_y;
  }
  void set_rect(int32_t ll_x, int32_t ll_y, int32_t ur_x, int32_t ur_y)
  {
    set_ll(ll_x, ll_y);
    set_ur(ur_x, ur_y);
  }
  // function

 private:
  int32_t _ll_x = -1;
  int32_t _ll_y = -1;
  int32_t _ur_x = -1;
  int32_t _ur_y = -1;
};

}  // namespace ifp
