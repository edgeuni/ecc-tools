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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "FPHeader.hpp"

namespace ifp {

class MPComParam
{
 public:
  MPComParam() = default;
  MPComParam(double wirelength_weight, double overlap_weight, double out_of_bound_weight, double periphery_weight, double blockage_weight,
             double io_weight, int32_t max_iter, double cool_rate, double initial_temperature)
  {
    _wirelength_weight = wirelength_weight;
    _overlap_weight = overlap_weight;
    _out_of_bound_weight = out_of_bound_weight;
    _periphery_weight = periphery_weight;
    _blockage_weight = blockage_weight;
    _io_weight = io_weight;
    _max_iter = max_iter;
    _cool_rate = cool_rate;
    _initial_temperature = initial_temperature;
  }
  ~MPComParam() = default;
  // getter
  double get_wirelength_weight() const { return _wirelength_weight; }
  double get_overlap_weight() const { return _overlap_weight; }
  double get_out_of_bound_weight() const { return _out_of_bound_weight; }
  double get_periphery_weight() const { return _periphery_weight; }
  double get_blockage_weight() const { return _blockage_weight; }
  double get_io_weight() const { return _io_weight; }
  int32_t get_max_iter() const { return _max_iter; }
  double get_cool_rate() const { return _cool_rate; }
  double get_initial_temperature() const { return _initial_temperature; }
  // setter
  void set_wirelength_weight(double wirelength_weight) { _wirelength_weight = wirelength_weight; }
  void set_overlap_weight(double overlap_weight) { _overlap_weight = overlap_weight; }
  void set_out_of_bound_weight(double out_of_bound_weight) { _out_of_bound_weight = out_of_bound_weight; }
  void set_periphery_weight(double periphery_weight) { _periphery_weight = periphery_weight; }
  void set_blockage_weight(double blockage_weight) { _blockage_weight = blockage_weight; }
  void set_io_weight(double io_weight) { _io_weight = io_weight; }
  void set_max_iter(int32_t max_iter) { _max_iter = max_iter; }
  void set_cool_rate(double cool_rate) { _cool_rate = cool_rate; }
  void set_initial_temperature(double initial_temperature) { _initial_temperature = initial_temperature; }

 private:
  double _wirelength_weight = 0;
  double _overlap_weight = 0;
  double _out_of_bound_weight = 0;
  double _periphery_weight = 0;
  double _blockage_weight = 0;
  double _io_weight = 0;
  int32_t _max_iter = -1;
  double _cool_rate = 0;
  double _initial_temperature = 0;
};

}  // namespace ifp
