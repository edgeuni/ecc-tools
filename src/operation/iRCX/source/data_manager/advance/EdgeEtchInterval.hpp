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

class EdgeEtchInterval
{
 public:
  EdgeEtchInterval() = default;
  ~EdgeEtchInterval() = default;
  // getter
  Micron get_start_coordinate() const { return _start_coordinate; }
  Micron get_end_coordinate() const { return _end_coordinate; }
  Micron get_center() const { return _center; }
  Micron get_width() const { return _width; }
  Micron get_lower_spacing() const { return _lower_spacing; }
  Micron get_upper_spacing() const { return _upper_spacing; }
  Micron get_thickness() const { return _thickness; }
  Micron get_height() const { return _height; }
  Micron get_resistance_center() const { return _resistance_center; }
  Micron get_resistance_width() const { return _resistance_width; }
  Micron get_resistance_lower_spacing() const { return _resistance_lower_spacing; }
  Micron get_resistance_upper_spacing() const { return _resistance_upper_spacing; }
  Micron get_resistance_thickness() const { return _resistance_thickness; }
  Micron get_capacitance_center() const { return _capacitance_center; }
  Micron get_capacitance_width() const { return _capacitance_width; }
  Micron get_capacitance_lower_spacing() const { return _capacitance_lower_spacing; }
  Micron get_capacitance_upper_spacing() const { return _capacitance_upper_spacing; }
  Micron get_capacitance_thickness() const { return _capacitance_thickness; }
  // setter
  void set_start_coordinate(Micron start_coordinate) { _start_coordinate = start_coordinate; }
  void set_end_coordinate(Micron end_coordinate) { _end_coordinate = end_coordinate; }
  void set_center(Micron center)
  {
    _center = center;
    _resistance_center = center;
    _capacitance_center = center;
  }
  void set_width(Micron width)
  {
    _width = width;
    _resistance_width = width;
    _capacitance_width = width;
  }
  void set_lower_spacing(Micron lower_spacing)
  {
    _lower_spacing = lower_spacing;
    _resistance_lower_spacing = lower_spacing;
    _capacitance_lower_spacing = lower_spacing;
  }
  void set_upper_spacing(Micron upper_spacing)
  {
    _upper_spacing = upper_spacing;
    _resistance_upper_spacing = upper_spacing;
    _capacitance_upper_spacing = upper_spacing;
  }
  void set_thickness(Micron thickness)
  {
    _thickness = thickness;
    _resistance_thickness = thickness;
    _capacitance_thickness = thickness;
  }
  void set_height(Micron height) { _height = height; }
  void set_resistance_center(Micron resistance_center) { _resistance_center = resistance_center; }
  void set_resistance_width(Micron resistance_width) { _resistance_width = resistance_width; }
  void set_resistance_lower_spacing(Micron resistance_lower_spacing) { _resistance_lower_spacing = resistance_lower_spacing; }
  void set_resistance_upper_spacing(Micron resistance_upper_spacing) { _resistance_upper_spacing = resistance_upper_spacing; }
  void set_resistance_thickness(Micron resistance_thickness) { _resistance_thickness = resistance_thickness; }
  void set_capacitance_center(Micron capacitance_center) { _capacitance_center = capacitance_center; }
  void set_capacitance_width(Micron capacitance_width) { _capacitance_width = capacitance_width; }
  void set_capacitance_lower_spacing(Micron capacitance_lower_spacing) { _capacitance_lower_spacing = capacitance_lower_spacing; }
  void set_capacitance_upper_spacing(Micron capacitance_upper_spacing) { _capacitance_upper_spacing = capacitance_upper_spacing; }
  void set_capacitance_thickness(Micron capacitance_thickness) { _capacitance_thickness = capacitance_thickness; }
  // function

 private:
  Micron _start_coordinate = 0.0;
  Micron _end_coordinate = 0.0;
  Micron _center = 0.0;
  Micron _width = 0.0;
  Micron _lower_spacing = kMaxMicron;
  Micron _upper_spacing = kMaxMicron;
  Micron _thickness = 0.0;
  Micron _height = 0.0;
  Micron _resistance_center = 0.0;
  Micron _resistance_width = 0.0;
  Micron _resistance_lower_spacing = kMaxMicron;
  Micron _resistance_upper_spacing = kMaxMicron;
  Micron _resistance_thickness = 0.0;
  Micron _capacitance_center = 0.0;
  Micron _capacitance_width = 0.0;
  Micron _capacitance_lower_spacing = kMaxMicron;
  Micron _capacitance_upper_spacing = kMaxMicron;
  Micron _capacitance_thickness = 0.0;
};

}  // namespace ircx
