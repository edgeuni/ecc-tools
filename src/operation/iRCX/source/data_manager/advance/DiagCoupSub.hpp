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
#include "TopoEdge.hpp"

namespace ircx {

class DiagCoupSub
{
 public:
  DiagCoupSub() = default;
  ~DiagCoupSub() = default;
  // getter
  Dbu get_start_coordinate() const { return _start_coordinate; }
  Dbu get_end_coordinate() const { return _end_coordinate; }
  TopoEdge* get_neighbor_edge() const { return _neighbor_edge; }
  Dbu get_distance() const { return _distance; }
  I16 get_layer_delta() const { return _layer_delta; }
  // setter
  void set_start_coordinate(Dbu start_coordinate) { _start_coordinate = start_coordinate; }
  void set_end_coordinate(Dbu end_coordinate) { _end_coordinate = end_coordinate; }
  void set_neighbor_edge(TopoEdge* neighbor_edge) { _neighbor_edge = neighbor_edge; }
  void set_distance(Dbu distance) { _distance = distance; }
  void set_layer_delta(I16 layer_delta) { _layer_delta = layer_delta; }
  // function

 private:
  Dbu _start_coordinate = 0;
  Dbu _end_coordinate = 0;
  TopoEdge* _neighbor_edge = nullptr;
  Dbu _distance = 0;
  I16 _layer_delta = 0;
};

}  // namespace ircx
