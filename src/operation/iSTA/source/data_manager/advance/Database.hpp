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

#include "Arc.hpp"
#include "Instance.hpp"
#include "Net.hpp"
#include "Pin.hpp"
#include "STAHeader.hpp"
#include "Summary.hpp"
#include "TimingPoint.hpp"

namespace ista {

class Database
{
 public:
  Database() = default;
  ~Database() = default;
  // getter
  std::string& get_design_name() { return _design_name; }
  std::unordered_map<std::string, Instance>& get_instance_map() { return _instance_map; }
  std::unordered_map<std::string, Pin>& get_pin_map() { return _pin_map; }
  std::unordered_map<std::string, Net>& get_net_map() { return _net_map; }
  std::vector<Arc>& get_arc_list() { return _arc_list; }
  std::unordered_map<std::string, std::vector<std::size_t>>& get_outgoing_arc_list_map() { return _outgoing_arc_list_map; }
  std::unordered_map<std::string, std::vector<std::size_t>>& get_incoming_arc_list_map() { return _incoming_arc_list_map; }
  std::vector<std::string>& get_startpoint_list() { return _startpoint_list; }
  std::vector<std::string>& get_endpoint_list() { return _endpoint_list; }
  std::unordered_map<std::string, TimingPoint>& get_timing_point_map() { return _timing_point_map; }
  Summary& get_summary() { return _summary; }
  // setter
  void set_design_name(const std::string& design_name) { _design_name = design_name; }
  void set_instance_map(const std::unordered_map<std::string, Instance>& instance_map) { _instance_map = instance_map; }
  void set_pin_map(const std::unordered_map<std::string, Pin>& pin_map) { _pin_map = pin_map; }
  void set_net_map(const std::unordered_map<std::string, Net>& net_map) { _net_map = net_map; }
  void set_arc_list(const std::vector<Arc>& arc_list) { _arc_list = arc_list; }
  void set_outgoing_arc_list_map(const std::unordered_map<std::string, std::vector<std::size_t>>& outgoing_arc_list_map)
  {
    _outgoing_arc_list_map = outgoing_arc_list_map;
  }
  void set_incoming_arc_list_map(const std::unordered_map<std::string, std::vector<std::size_t>>& incoming_arc_list_map)
  {
    _incoming_arc_list_map = incoming_arc_list_map;
  }
  void set_startpoint_list(const std::vector<std::string>& startpoint_list) { _startpoint_list = startpoint_list; }
  void set_endpoint_list(const std::vector<std::string>& endpoint_list) { _endpoint_list = endpoint_list; }
  void set_timing_point_map(const std::unordered_map<std::string, TimingPoint>& timing_point_map)
  {
    _timing_point_map = timing_point_map;
  }
  void set_summary(const Summary& summary) { _summary = summary; }
  // function

 private:
  std::string _design_name;
  std::unordered_map<std::string, Instance> _instance_map;
  std::unordered_map<std::string, Pin> _pin_map;
  std::unordered_map<std::string, Net> _net_map;
  std::vector<Arc> _arc_list;
  std::unordered_map<std::string, std::vector<std::size_t>> _outgoing_arc_list_map;
  std::unordered_map<std::string, std::vector<std::size_t>> _incoming_arc_list_map;
  std::vector<std::string> _startpoint_list;
  std::vector<std::string> _endpoint_list;
  std::unordered_map<std::string, TimingPoint> _timing_point_map;
  Summary _summary;
};

}  // namespace ista
