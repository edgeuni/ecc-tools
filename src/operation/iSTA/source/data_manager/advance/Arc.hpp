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

#include "ArcType.hpp"
#include "STAHeader.hpp"

namespace ista {

class Arc
{
 public:
  Arc() = default;
  ~Arc() = default;
  // getter
  std::string& get_arc_name() { return _arc_name; }
  std::string& get_source_pin() { return _source_pin; }
  std::string& get_sink_pin() { return _sink_pin; }
  std::string& get_owner_name() { return _owner_name; }
  ArcType get_type() const { return _type; }
  double get_delay() const { return _delay; }
  // setter
  void set_arc_name(const std::string& arc_name) { _arc_name = arc_name; }
  void set_source_pin(const std::string& source_pin) { _source_pin = source_pin; }
  void set_sink_pin(const std::string& sink_pin) { _sink_pin = sink_pin; }
  void set_owner_name(const std::string& owner_name) { _owner_name = owner_name; }
  void set_type(const ArcType& type) { _type = type; }
  void set_delay(const double delay) { _delay = delay; }
  // function

 private:
  std::string _arc_name;
  std::string _source_pin;
  std::string _sink_pin;
  std::string _owner_name;
  ArcType _type = ArcType::kNone;
  double _delay = 1.0;
};

}  // namespace ista
