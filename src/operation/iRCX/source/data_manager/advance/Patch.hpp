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

class Patch
{
 public:
  Patch() = default;
  ~Patch() = default;
  // getter
  size_t get_layer_id() const { return _layer_id; }
  GtlRectI& get_shape() { return _shape; }
  // setter
  void set_layer_id(size_t layer_id) { _layer_id = layer_id; }
  void set_shape(const GtlRectI& shape) { _shape = shape; }
  // function

 private:
  size_t _layer_id = kMaxSize;
  GtlRectI _shape;
};

}  // namespace ircx
