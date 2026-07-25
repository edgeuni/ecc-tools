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

#include "PlanarRect.hpp"

namespace ifp {

class Row : public PlanarRect
{
 public:
  Row() = default;
  ~Row() = default;
  // getter
  std::string& get_name() { return _name; }
  std::string& get_site_name() { return _site_name; }
  int32_t get_y() const { return _y; }
  std::string& get_orient_name() { return _orient_name; }

  // const getter
  const std::string& get_name() const { return _name; }
  const std::string& get_site_name() const { return _site_name; }
  const std::string& get_orient_name() const { return _orient_name; }

  // setter
  void set_name(std::string name) { _name = name; }
  void set_site_name(std::string site_name) { _site_name = site_name; }
  void set_y(int32_t y) { _y = y; }
  void set_orient_name(std::string orient_name) { _orient_name = orient_name; }

  // function

 private:
  std::string _name;
  std::string _site_name;
  int32_t _y = -1;
  std::string _orient_name;
};

}  // namespace ifp
