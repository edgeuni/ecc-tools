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

class TopoNode
{
 public:
  explicit TopoNode(Size net_id) : _net_id(net_id) {}
  TopoNode() = delete;
  ~TopoNode() = default;
  // getter
  Size get_node_id() const { return _node_id; }
  Size get_net_id() const { return _net_id; }
  Size get_layer_id() const { return _layer_id; }
  GtlPointI& get_point() { return _point; }
  GtlRectI& get_shape() { return _shape; }
  std::string& get_pin_name() { return _pin_name; }
  // setter
  void set_layer_id(Size layer_id) { _layer_id = layer_id; }
  void set_point(const GtlPointI& point) { _point = point; }
  void set_shape(const GtlRectI& shape) { _shape = shape; }
  void set_pin_name(const std::string& pin_name) { _pin_name = pin_name; }
  // function
  bool get_is_pin_node() const { return !_pin_name.empty(); }

 private:
  friend class TopoPool;

  void set_node_id(Size node_id) { _node_id = node_id; }

  Size _node_id = kMaxSize;
  Size _net_id = kMaxSize;
  Size _layer_id = kMaxSize;
  GtlPointI _point;
  GtlRectI _shape;
  std::string _pin_name;
};

}  // namespace ircx
