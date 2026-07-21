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

namespace ircx {

class EdgeEtchInterval
{
 public:
  EdgeEtchInterval() = default;
  ~EdgeEtchInterval() = default;
  // getter
  double get_start_coordinate() const { return _start_coordinate; }
  double get_end_coordinate() const { return _end_coordinate; }
  double get_center() const { return _center; }
  double get_width() const { return _width; }
  double get_lower_spacing() const { return _lower_spacing; }
  double get_upper_spacing() const { return _upper_spacing; }
  double get_thickness() const { return _thickness; }
  double get_height() const { return _height; }
  double get_res_center() const { return _res_center; }
  double get_res_width() const { return _res_width; }
  double get_res_lower_spacing() const { return _res_lower_spacing; }
  double get_res_upper_spacing() const { return _res_upper_spacing; }
  double get_res_thickness() const { return _res_thickness; }
  double get_cap_center() const { return _cap_center; }
  double get_cap_width() const { return _cap_width; }
  double get_cap_lower_spacing() const { return _cap_lower_spacing; }
  double get_cap_upper_spacing() const { return _cap_upper_spacing; }
  double get_cap_thickness() const { return _cap_thickness; }
  // setter
  void set_start_coordinate(double start_coordinate) { _start_coordinate = start_coordinate; }
  void set_end_coordinate(double end_coordinate) { _end_coordinate = end_coordinate; }
  void set_center(double center)
  {
    _center = center;
    _res_center = center;
    _cap_center = center;
  }
  void set_width(double width)
  {
    _width = width;
    _res_width = width;
    _cap_width = width;
  }
  void set_lower_spacing(double lower_spacing)
  {
    _lower_spacing = lower_spacing;
    _res_lower_spacing = lower_spacing;
    _cap_lower_spacing = lower_spacing;
  }
  void set_upper_spacing(double upper_spacing)
  {
    _upper_spacing = upper_spacing;
    _res_upper_spacing = upper_spacing;
    _cap_upper_spacing = upper_spacing;
  }
  void set_thickness(double thickness)
  {
    _thickness = thickness;
    _res_thickness = thickness;
    _cap_thickness = thickness;
  }
  void set_height(double height) { _height = height; }
  void set_res_center(double res_center) { _res_center = res_center; }
  void set_res_width(double res_width) { _res_width = res_width; }
  void set_res_lower_spacing(double res_lower_spacing) { _res_lower_spacing = res_lower_spacing; }
  void set_res_upper_spacing(double res_upper_spacing) { _res_upper_spacing = res_upper_spacing; }
  void set_res_thickness(double res_thickness) { _res_thickness = res_thickness; }
  void set_cap_center(double cap_center) { _cap_center = cap_center; }
  void set_cap_width(double cap_width) { _cap_width = cap_width; }
  void set_cap_lower_spacing(double cap_lower_spacing) { _cap_lower_spacing = cap_lower_spacing; }
  void set_cap_upper_spacing(double cap_upper_spacing) { _cap_upper_spacing = cap_upper_spacing; }
  void set_cap_thickness(double cap_thickness) { _cap_thickness = cap_thickness; }
  // function

 private:
  double _start_coordinate = 0.0;
  double _end_coordinate = 0.0;
  double _center = 0.0;
  double _width = 0.0;
  double _lower_spacing = DBL_MAX;
  double _upper_spacing = DBL_MAX;
  double _thickness = 0.0;
  double _height = 0.0;
  double _res_center = 0.0;
  double _res_width = 0.0;
  double _res_lower_spacing = DBL_MAX;
  double _res_upper_spacing = DBL_MAX;
  double _res_thickness = 0.0;
  double _cap_center = 0.0;
  double _cap_width = 0.0;
  double _cap_lower_spacing = DBL_MAX;
  double _cap_upper_spacing = DBL_MAX;
  double _cap_thickness = 0.0;
};

}  // namespace ircx
