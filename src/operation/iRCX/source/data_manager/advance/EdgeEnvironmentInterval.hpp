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

#include "CrossOverlapSub.hpp"
#include "DiagCoupSub.hpp"
#include "TopoEdge.hpp"

namespace ircx {

class EdgeEnvironmentInterval
{
 public:
  EdgeEnvironmentInterval() = default;
  ~EdgeEnvironmentInterval() = default;
  // getter
  int32_t get_start_coordinate() const { return _start_coordinate; }
  int32_t get_end_coordinate() const { return _end_coordinate; }
  int32_t get_lower_spacing() const { return _lower_spacing; }
  int32_t get_upper_spacing() const { return _upper_spacing; }
  TopoEdge* get_lower_adjacent_edge() const { return _lower_adjacent_edge; }
  TopoEdge* get_upper_adjacent_edge() const { return _upper_adjacent_edge; }
  std::vector<CrossOverlapSub>& get_cross_overlap_sub_list() { return _cross_overlap_sub_list; }
  std::vector<DiagCoupSub>& get_diag_coup_sub_list() { return _diag_coup_sub_list; }
  // setter
  void set_start_coordinate(int32_t start_coordinate) { _start_coordinate = start_coordinate; }
  void set_end_coordinate(int32_t end_coordinate) { _end_coordinate = end_coordinate; }
  void set_lower_spacing(int32_t lower_spacing) { _lower_spacing = lower_spacing; }
  void set_upper_spacing(int32_t upper_spacing) { _upper_spacing = upper_spacing; }
  void set_lower_adjacent_edge(TopoEdge* lower_adjacent_edge) { _lower_adjacent_edge = lower_adjacent_edge; }
  void set_upper_adjacent_edge(TopoEdge* upper_adjacent_edge) { _upper_adjacent_edge = upper_adjacent_edge; }
  void set_cross_overlap_sub_list(const std::vector<CrossOverlapSub>& cross_overlap_sub_list)
  {
    _cross_overlap_sub_list = cross_overlap_sub_list;
  }
  void set_diag_coup_sub_list(const std::vector<DiagCoupSub>& diag_coup_sub_list) { _diag_coup_sub_list = diag_coup_sub_list; }
  // function

 private:
  int32_t _start_coordinate = 0;
  int32_t _end_coordinate = 0;
  int32_t _lower_spacing = kMaxDbu;
  int32_t _upper_spacing = kMaxDbu;
  TopoEdge* _lower_adjacent_edge = nullptr;
  TopoEdge* _upper_adjacent_edge = nullptr;
  std::vector<CrossOverlapSub> _cross_overlap_sub_list;
  std::vector<DiagCoupSub> _diag_coup_sub_list;
};

}  // namespace ircx
