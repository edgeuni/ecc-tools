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
#include "TopoEdge.hpp"

namespace ircx {

class EnvTrackOverlap
{
 public:
  EnvTrackOverlap() = default;
  EnvTrackOverlap(int32_t start_coordinate, int32_t end_coordinate, int32_t spacing, TopoEdge* edge)
      : _start_coordinate(start_coordinate), _end_coordinate(end_coordinate), _spacing(spacing), _edge(edge)
  {
  }
  ~EnvTrackOverlap() = default;
  // getter
  int32_t get_start_coordinate() const { return _start_coordinate; }
  int32_t get_end_coordinate() const { return _end_coordinate; }
  int32_t get_spacing() const { return _spacing; }
  TopoEdge* get_edge() const { return _edge; }
  // setter
  void set_start_coordinate(int32_t start_coordinate) { _start_coordinate = start_coordinate; }
  void set_end_coordinate(int32_t end_coordinate) { _end_coordinate = end_coordinate; }
  void set_spacing(int32_t spacing) { _spacing = spacing; }
  void set_edge(TopoEdge* edge) { _edge = edge; }
  // function

 private:
  int32_t _start_coordinate = 0;
  int32_t _end_coordinate = 0;
  int32_t _spacing = INT32_MAX;
  TopoEdge* _edge = nullptr;
};

}  // namespace ircx
