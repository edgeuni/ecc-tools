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
#include "TimingPathPoint.hpp"

namespace ista {

class TimingPath
{
 public:
  TimingPath() = default;
  ~TimingPath() = default;
  // getter
  std::string& get_start_point() { return _start_point; }
  std::string& get_end_point() { return _end_point; }
  double get_path_delay() const { return _path_delay; }
  double get_required_time() const { return _required_time; }
  double get_slack() const { return _slack; }
  double get_cell_delay() const { return _cell_delay; }
  double get_net_delay() const { return _net_delay; }
  double get_launch_time() const { return _launch_time; }
  double get_capture_time() const { return _capture_time; }
  double get_setup_time() const { return _setup_time; }
  int32_t get_level() const { return _level; }
  std::string& get_clock_name() { return _clock_name; }
  std::string& get_capture_clock_pin() { return _capture_clock_pin; }
  std::vector<TimingPathPoint>& get_point_list() { return _point_list; }
  // setter
  void set_start_point(const std::string& start_point) { _start_point = start_point; }
  void set_end_point(const std::string& end_point) { _end_point = end_point; }
  void set_path_delay(const double path_delay) { _path_delay = path_delay; }
  void set_required_time(const double required_time) { _required_time = required_time; }
  void set_slack(const double slack) { _slack = slack; }
  void set_cell_delay(const double cell_delay) { _cell_delay = cell_delay; }
  void set_net_delay(const double net_delay) { _net_delay = net_delay; }
  void set_launch_time(const double launch_time) { _launch_time = launch_time; }
  void set_capture_time(const double capture_time) { _capture_time = capture_time; }
  void set_setup_time(const double setup_time) { _setup_time = setup_time; }
  void set_level(const int32_t level) { _level = level; }
  void set_clock_name(const std::string& clock_name) { _clock_name = clock_name; }
  void set_capture_clock_pin(const std::string& capture_clock_pin) { _capture_clock_pin = capture_clock_pin; }
  void set_point_list(const std::vector<TimingPathPoint>& point_list) { _point_list = point_list; }
  // function

 private:
  std::string _start_point;
  std::string _end_point;
  double _path_delay = 0.0;
  double _required_time = 0.0;
  double _slack = 0.0;
  double _cell_delay = 0.0;
  double _net_delay = 0.0;
  double _launch_time = 0.0;
  double _capture_time = 0.0;
  double _setup_time = 0.0;
  int32_t _level = 0;
  std::string _clock_name;
  std::string _capture_clock_pin;
  std::vector<TimingPathPoint> _point_list;
};

}  // namespace ista
