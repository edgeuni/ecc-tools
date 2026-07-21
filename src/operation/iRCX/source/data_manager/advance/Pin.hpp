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

#include "LayerShape.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class Pin
{
 public:
  Pin() = default;
  ~Pin() = default;
  // getter
  std::string& get_pin_name() { return _pin_name; }
  bool get_is_driver() const { return _is_driver; }
  bool get_is_input() const { return _is_input; }
  bool get_is_output() const { return _is_output; }
  std::vector<LayerShape>& get_layer_shape_list() { return _layer_shape_list; }
  // setter
  void set_pin_name(const std::string& pin_name) { _pin_name = pin_name; }
  void set_is_driver(bool is_driver) { _is_driver = is_driver; }
  void set_is_input(bool is_input) { _is_input = is_input; }
  void set_is_output(bool is_output) { _is_output = is_output; }
  void set_layer_shape_list(const std::vector<LayerShape>& layer_shape_list) { _layer_shape_list = layer_shape_list; }
  // function
  bool get_is_port() const { return _pin_name.find(':') == std::string::npos; }
  std::string get_instance_name() const;
  std::string get_instance_pin_name() const;
  std::string get_port_name() const { return _pin_name; }

 private:
  std::string _pin_name;
  bool _is_driver = false;
  bool _is_input = false;
  bool _is_output = false;
  std::vector<LayerShape> _layer_shape_list;
};

inline std::string Pin::get_instance_name() const
{
  size_t delimiter_pos = _pin_name.find(':');
  return _pin_name.substr(0, delimiter_pos);
}

inline std::string Pin::get_instance_pin_name() const
{
  size_t delimiter_pos = _pin_name.find(':');
  return _pin_name.substr(delimiter_pos + 1);
}

}  // namespace ircx
