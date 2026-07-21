// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of the License at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "RCXHeader.hpp"

namespace ircx {

class CapTableEntry
{
 public:
  CapTableEntry() = default;
  ~CapTableEntry() = default;
  // getter
  double get_distance() const { return _distance; }
  double get_coupling_cap() const { return _coupling_cap; }
  double get_ground_cap() const { return _ground_cap; }
  // setter
  void set_distance(double distance) { _distance = distance; }
  void set_coupling_cap(double coupling_cap) { _coupling_cap = coupling_cap; }
  void set_ground_cap(double ground_cap) { _ground_cap = ground_cap; }
  // function

 private:
  double _distance = DBL_MAX;
  double _coupling_cap = DBL_MAX;
  double _ground_cap = DBL_MAX;
};

}  // namespace ircx
