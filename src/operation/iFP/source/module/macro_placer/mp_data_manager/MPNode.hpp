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

class MPNode
{
 public:
  MPNode() = default;
  ~MPNode() = default;
  // getter
  std::string& get_name() { return _name; }
  int32_t get_x() const { return _x; }
  int32_t get_y() const { return _y; }
  int32_t get_width() const { return _width; }
  int32_t get_height() const { return _height; }
  bool get_fixed() const { return _fixed; }
  bool get_placed() const { return _placed; }
  // const getter
  const std::string& get_name() const { return _name; }

  // setter
  void set_name(const std::string& name) { _name = name; }
  void set_x(int32_t x) { _x = x; }
  void set_y(int32_t y) { _y = y; }
  void set_coord(int32_t x, int32_t y)
  {
    _x = x;
    _y = y;
  }
  void set_width(int32_t width) { _width = width; }
  void set_height(int32_t height) { _height = height; }
  void set_fixed(bool fixed) { _fixed = fixed; }
  void set_placed(bool placed) { _placed = placed; }
  // function

 private:
  std::string _name;
  int32_t _x = 0;
  int32_t _y = 0;
  int32_t _width = 0;
  int32_t _height = 0;
  bool _fixed = false;
  bool _placed = false;
};

}  // namespace ifp
