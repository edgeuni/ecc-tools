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
// INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "LayerCoord.hpp"
#include "RTHeader.hpp"

namespace irt {

class GSRPin
{
 public:
  GSRPin() = default;
  GSRPin(const int32_t pin_idx, const std::string& pin_name, const LayerCoord& access_coord)
      : _pin_idx(pin_idx), _pin_name(pin_name), _access_coord(access_coord)
  {
  }
  ~GSRPin() = default;
  // getter
  int32_t get_pin_idx() const { return _pin_idx; }
  std::string& get_pin_name() { return _pin_name; }
  LayerCoord& get_access_coord() { return _access_coord; }
  // const getter
  const std::string& get_pin_name() const { return _pin_name; }
  const LayerCoord& get_access_coord() const { return _access_coord; }
  // setter
  void set_pin_idx(const int32_t pin_idx) { _pin_idx = pin_idx; }
  void set_pin_name(const std::string& pin_name) { _pin_name = pin_name; }
  void set_access_coord(const LayerCoord& access_coord) { _access_coord = access_coord; }

 private:
  int32_t _pin_idx = -1;
  std::string _pin_name;
  LayerCoord _access_coord;
};

}  // namespace irt
