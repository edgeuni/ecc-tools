// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of the Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "STAHeader.hpp"

namespace ista {

class PPModel
{
 public:
  PPModel() = default;
  ~PPModel() = default;
  // getter
  std::vector<std::string>& get_seed_pin_list() { return _seed_pin_list; }
  // setter
  void set_seed_pin_list(const std::vector<std::string>& seed_pin_list) { _seed_pin_list = seed_pin_list; }
  // function

 private:
  std::vector<std::string> _seed_pin_list;
};

}  // namespace ista
