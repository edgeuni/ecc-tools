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
#include "liberty/Lib.hh"

namespace ista {

class TimingCellArc
{
 public:
  TimingCellArc() = default;
  ~TimingCellArc() = default;
  // getter
  std::string& get_source_port() { return _source_port; }
  std::string& get_sink_port() { return _sink_port; }
  double get_delay() const { return _delay; }
  double get_delay_max() const { return _delay_max; }
  double get_delay_min() const { return _delay_min; }
  idb::LibArcSet* get_lib_arc_set() { return _lib_arc_set; }
  bool get_is_clock_arc() const { return _is_clock_arc; }
  // setter
  void set_source_port(const std::string& source_port) { _source_port = source_port; }
  void set_sink_port(const std::string& sink_port) { _sink_port = sink_port; }
  void set_delay(const double delay) { _delay = delay; }
  void set_delay_max(const double delay_max) { _delay_max = delay_max; }
  void set_delay_min(const double delay_min) { _delay_min = delay_min; }
  void set_lib_arc_set(idb::LibArcSet* lib_arc_set) { _lib_arc_set = lib_arc_set; }
  void set_is_clock_arc(const bool is_clock_arc) { _is_clock_arc = is_clock_arc; }
  // function

 private:
  std::string _source_port;
  std::string _sink_port;
  double _delay = 0.0;
  double _delay_max = 0.0;
  double _delay_min = 0.0;
  idb::LibArcSet* _lib_arc_set = nullptr;
  bool _is_clock_arc = false;
};

}  // namespace ista
