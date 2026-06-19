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

class Instance
{
 public:
  Instance() = default;
  ~Instance() = default;
  // getter
  std::string& get_name() { return _name; }
  std::string& get_cell_name() { return _cell_name; }
  std::vector<std::string>& get_pin_name_list() { return _pin_name_list; }
  // setter
  void set_name(const std::string& name) { _name = name; }
  void set_cell_name(const std::string& cell_name) { _cell_name = cell_name; }
  void set_pin_name_list(const std::vector<std::string>& pin_name_list) { _pin_name_list = pin_name_list; }
  // function

 private:
  std::string _name;
  std::string _cell_name;
  std::vector<std::string> _pin_name_list;
};

}  // namespace ista
