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

class MPRect
{
 public:
  MPRect() = default;
  MPRect(int32_t lx, int32_t ly, int32_t ux, int32_t uy) : _lx(lx), _ly(ly), _ux(ux), _uy(uy) {}
  ~MPRect() = default;
  // getter
  int32_t get_lx() const { return _lx; }
  int32_t get_ly() const { return _ly; }
  int32_t get_ux() const { return _ux; }
  int32_t get_uy() const { return _uy; }
  int32_t get_width() const { return _ux - _lx; }
  int32_t get_height() const { return _uy - _ly; }
  // const getter

  // setter
  void set_lx(int32_t lx) { _lx = lx; }
  void set_ly(int32_t ly) { _ly = ly; }
  void set_ux(int32_t ux) { _ux = ux; }
  void set_uy(int32_t uy) { _uy = uy; }
  // function
  bool is_overlap(const MPRect& rect) const
  {
    return _lx < rect.get_ux() && rect.get_lx() < _ux && _ly < rect.get_uy() && rect.get_ly() < _uy;
  }
  int64_t get_overlap_area(const MPRect& rect) const
  {
    if (!is_overlap(rect)) {
      return 0;
    }
    return static_cast<int64_t>(std::min(_ux, rect.get_ux()) - std::max(_lx, rect.get_lx()))
           * static_cast<int64_t>(std::min(_uy, rect.get_uy()) - std::max(_ly, rect.get_ly()));
  }

 private:
  int32_t _lx = 0;
  int32_t _ly = 0;
  int32_t _ux = 0;
  int32_t _uy = 0;
};

}  // namespace ifp
