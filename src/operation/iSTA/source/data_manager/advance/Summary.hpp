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

namespace ista {

class Summary
{
 public:
  Summary() = default;
  ~Summary() = default;
  // getter
  std::size_t get_instance_num() const { return _instance_num; }
  std::size_t get_port_num() const { return _port_num; }
  std::size_t get_pin_num() const { return _pin_num; }
  std::size_t get_net_num() const { return _net_num; }
  std::size_t get_arc_num() const { return _arc_num; }
  std::size_t get_startpoint_num() const { return _startpoint_num; }
  std::size_t get_endpoint_num() const { return _endpoint_num; }
  std::size_t get_loop_vertex_num() const { return _loop_vertex_num; }
  double get_required_time() const { return _required_time; }
  double get_worst_slack() const { return _worst_slack; }
  std::string& get_worst_endpoint() { return _worst_endpoint; }
  std::vector<std::string>& get_timing_order() { return _timing_order; }
  // setter
  void set_instance_num(const std::size_t instance_num) { _instance_num = instance_num; }
  void set_port_num(const std::size_t port_num) { _port_num = port_num; }
  void set_pin_num(const std::size_t pin_num) { _pin_num = pin_num; }
  void set_net_num(const std::size_t net_num) { _net_num = net_num; }
  void set_arc_num(const std::size_t arc_num) { _arc_num = arc_num; }
  void set_startpoint_num(const std::size_t startpoint_num) { _startpoint_num = startpoint_num; }
  void set_endpoint_num(const std::size_t endpoint_num) { _endpoint_num = endpoint_num; }
  void set_loop_vertex_num(const std::size_t loop_vertex_num) { _loop_vertex_num = loop_vertex_num; }
  void set_required_time(const double required_time) { _required_time = required_time; }
  void set_worst_slack(const double worst_slack) { _worst_slack = worst_slack; }
  void set_worst_endpoint(const std::string& worst_endpoint) { _worst_endpoint = worst_endpoint; }
  void set_timing_order(const std::vector<std::string>& timing_order) { _timing_order = timing_order; }
  // function

 private:
  std::size_t _instance_num = 0;
  std::size_t _port_num = 0;
  std::size_t _pin_num = 0;
  std::size_t _net_num = 0;
  std::size_t _arc_num = 0;
  std::size_t _startpoint_num = 0;
  std::size_t _endpoint_num = 0;
  std::size_t _loop_vertex_num = 0;
  double _required_time = 0.0;
  double _worst_slack = 0.0;
  std::string _worst_endpoint;
  std::vector<std::string> _timing_order;
};

}  // namespace ista
