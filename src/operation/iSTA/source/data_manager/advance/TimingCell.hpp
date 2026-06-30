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

#include "STAHeader.hpp"
#include "TimingCellArc.hpp"
#include "TimingCellPort.hpp"
#include "TimingCheckArc.hpp"

namespace ista {

class TimingCell
{
 public:
  TimingCell() = default;
  ~TimingCell() = default;
  // getter
  std::string& get_cell_name() { return _cell_name; }
  double get_area() const { return _area; }
  std::map<std::string, TimingCellPort>& get_port_map() { return _port_map; }
  std::vector<TimingCellArc>& get_cell_arc_list() { return _cell_arc_list; }
  std::vector<TimingCheckArc>& get_setup_arc_list() { return _setup_arc_list; }
  std::vector<TimingCheckArc>& get_check_arc_list() { return _check_arc_list; }
  bool get_is_sequential() const { return _is_sequential; }
  bool get_is_clock_gating() const { return _is_clock_gating; }
  bool get_is_macro() const { return _is_macro; }
  bool get_has_clear_arc() const { return _has_clear_arc; }
  bool get_has_preset_arc() const { return _has_preset_arc; }
  // setter
  void set_cell_name(const std::string& cell_name) { _cell_name = cell_name; }
  void set_area(const double area) { _area = area; }
  void set_port_map(const std::map<std::string, TimingCellPort>& port_map) { _port_map = port_map; }
  void set_cell_arc_list(const std::vector<TimingCellArc>& cell_arc_list) { _cell_arc_list = cell_arc_list; }
  void set_setup_arc_list(const std::vector<TimingCheckArc>& setup_arc_list) { _setup_arc_list = setup_arc_list; }
  void set_check_arc_list(const std::vector<TimingCheckArc>& check_arc_list) { _check_arc_list = check_arc_list; }
  void set_is_sequential(const bool is_sequential) { _is_sequential = is_sequential; }
  void set_is_clock_gating(const bool is_clock_gating) { _is_clock_gating = is_clock_gating; }
  void set_is_macro(const bool is_macro) { _is_macro = is_macro; }
  void set_has_clear_arc(const bool has_clear_arc) { _has_clear_arc = has_clear_arc; }
  void set_has_preset_arc(const bool has_preset_arc) { _has_preset_arc = has_preset_arc; }
  // function

 private:
  std::string _cell_name;
  double _area = 0.0;
  std::map<std::string, TimingCellPort> _port_map;
  std::vector<TimingCellArc> _cell_arc_list;
  std::vector<TimingCheckArc> _setup_arc_list;
  std::vector<TimingCheckArc> _check_arc_list;
  bool _is_sequential = false;
  bool _is_clock_gating = false;
  bool _is_macro = false;
  bool _has_clear_arc = false;
  bool _has_preset_arc = false;
};

}  // namespace ista
