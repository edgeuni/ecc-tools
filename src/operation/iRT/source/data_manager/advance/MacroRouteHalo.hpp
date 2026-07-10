// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of Mulan PSL v2.
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

#include "PlanarRect.hpp"
#include "RTHeader.hpp"

namespace irt {

class MacroRouteHalo
{
 public:
  MacroRouteHalo() = default;
  ~MacroRouteHalo() = default;
  // getter
  std::string& get_inst_name() { return _inst_name; }
  PlanarRect& get_body_rect() { return _body_rect; }
  PlanarRect& get_halo_rect() { return _halo_rect; }
  std::vector<int32_t>& get_layer_idx_list() { return _layer_idx_list; }
  // setter
  void set_inst_name(const std::string& inst_name) { _inst_name = inst_name; }
  void set_body_rect(const PlanarRect& body_rect) { _body_rect = body_rect; }
  void set_halo_rect(const PlanarRect& halo_rect) { _halo_rect = halo_rect; }
  void set_layer_idx_list(const std::vector<int32_t>& layer_idx_list) { _layer_idx_list = layer_idx_list; }
  // function

 private:
  std::string _inst_name;
  PlanarRect _body_rect;
  PlanarRect _halo_rect;
  std::vector<int32_t> _layer_idx_list;
};

}  // namespace irt
