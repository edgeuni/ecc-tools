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
#include "MPPin.hpp"

namespace ifp {

class MPNet
{
 public:
  MPNet() = default;
  ~MPNet() = default;
  // getter
  std::string& get_name() { return _name; }
  std::vector<MPPin>& get_mp_pin_list() { return _mp_pin_list; }
  // const getter
  const std::string& get_name() const { return _name; }
  const std::vector<MPPin>& get_mp_pin_list() const { return _mp_pin_list; }

  // setter
  void set_name(const std::string& name) { _name = name; }
  void set_mp_pin_list(const std::vector<MPPin>& mp_pin_list) { _mp_pin_list = mp_pin_list; }
  // function

 private:
  std::string _name;
  std::vector<MPPin> _mp_pin_list;
};

}  // namespace ifp
