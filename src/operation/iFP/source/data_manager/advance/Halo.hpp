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
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "FPHeader.hpp"
#include "HaloType.hpp"

namespace ifp {

class Halo
{
 public:
  Halo() = default;
  ~Halo() = default;
  // getter
  HaloType get_type() const { return _type; }
  std::string& get_instance_name() { return _instance_name; }
  std::vector<std::string>& get_layer_name_list() { return _layer_name_list; }
  int32_t get_top() const { return _top; }
  int32_t get_bottom() const { return _bottom; }
  int32_t get_left() const { return _left; }
  int32_t get_right() const { return _right; }
  bool get_except_pg_net() const { return _except_pg_net; }

  // const getter
  const std::string& get_instance_name() const { return _instance_name; }
  const std::vector<std::string>& get_layer_name_list() const { return _layer_name_list; }

  // setter
  void set_type(HaloType type) { _type = type; }
  void set_instance_name(std::string instance_name) { _instance_name = instance_name; }
  void set_layer_name_list(std::vector<std::string> layer_name_list) { _layer_name_list = layer_name_list; }
  void set_top(int32_t top) { _top = top; }
  void set_bottom(int32_t bottom) { _bottom = bottom; }
  void set_left(int32_t left) { _left = left; }
  void set_right(int32_t right) { _right = right; }
  void set_except_pg_net(bool except_pg_net) { _except_pg_net = except_pg_net; }

  // function

 private:
  HaloType _type = HaloType::kNone;
  std::string _instance_name;
  std::vector<std::string> _layer_name_list;
  int32_t _top = -1;
  int32_t _bottom = -1;
  int32_t _left = -1;
  int32_t _right = -1;
  bool _except_pg_net = false;
};

}  // namespace ifp
