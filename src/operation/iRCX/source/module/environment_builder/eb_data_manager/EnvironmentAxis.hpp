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

#include "RCXHeader.hpp"

namespace ircx {

class EnvironmentAxis
{
 public:
  EnvironmentAxis() = default;
  EnvironmentAxis(Dbu origin, Dbu count, Dbu step) : _origin(origin), _count(count), _step(step) {}
  ~EnvironmentAxis() = default;
  // getter
  Dbu get_origin() const { return _origin; }
  Dbu get_count() const { return _count; }
  Dbu get_step() const { return _step; }
  // setter
  void set_origin(Dbu origin) { _origin = origin; }
  void set_count(Dbu count) { _count = count; }
  void set_step(Dbu step) { _step = step; }
  // function

 private:
  Dbu _origin = 0;
  Dbu _count = 0;
  Dbu _step = 0;
};

}  // namespace ircx
