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

class Segment
{
 public:
  Segment() = default;
  ~Segment() = default;
  // getter
  Size get_layer_id() const { return _layer_id; }
  GtlRectI& get_shape() { return _shape; }
  GtlPointI& get_start_point() { return _start_point; }
  GtlPointI& get_end_point() { return _end_point; }
  // setter
  void set_layer_id(Size layer_id) { _layer_id = layer_id; }
  void set_shape(const GtlRectI& shape) { _shape = shape; }
  void set_start_point(const GtlPointI& start_point) { _start_point = start_point; }
  void set_end_point(const GtlPointI& end_point) { _end_point = end_point; }
  // function

 private:
  Size _layer_id = kMaxSize;
  GtlRectI _shape;
  GtlPointI _start_point;
  GtlPointI _end_point;
};

}  // namespace ircx
