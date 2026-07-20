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

class CrossOverlapSub
{
 public:
  CrossOverlapSub() = default;
  ~CrossOverlapSub() = default;
  // getter
  int32_t get_start_coordinate() const { return _start_coordinate; }
  int32_t get_end_coordinate() const { return _end_coordinate; }
  size_t get_above_layer_id() const { return _above_layer_id; }
  size_t get_below_layer_id() const { return _below_layer_id; }
  // setter
  void set_start_coordinate(int32_t start_coordinate) { _start_coordinate = start_coordinate; }
  void set_end_coordinate(int32_t end_coordinate) { _end_coordinate = end_coordinate; }
  void set_above_layer_id(size_t above_layer_id) { _above_layer_id = above_layer_id; }
  void set_below_layer_id(size_t below_layer_id) { _below_layer_id = below_layer_id; }
  // function

 private:
  int32_t _start_coordinate = 0;
  int32_t _end_coordinate = 0;
  size_t _above_layer_id = 0;
  size_t _below_layer_id = 0;
};

}  // namespace ircx
