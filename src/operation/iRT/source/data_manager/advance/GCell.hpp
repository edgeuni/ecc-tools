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

#include "AccessPoint.hpp"

namespace irt {

class GCell : public PlanarRect
{
 public:
  GCell() = default;
  ~GCell() = default;
  // getter
  std::map<int32_t, std::vector<AccessPoint*>>& get_net_access_point_map() { return _net_access_point_map; }
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& get_net_global_result_map() { return _net_global_result_map; }
  // setter
  void set_net_access_point_map(const std::map<int32_t, std::vector<AccessPoint*>>& net_access_point_map)
  {
    _net_access_point_map = net_access_point_map;
  }
  void set_net_global_result_map(const std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map)
  {
    _net_global_result_map = net_global_result_map;
  }
  // function

 private:
  // access point
  std::map<int32_t, std::vector<AccessPoint*>> _net_access_point_map;
  // global routing result
  std::map<int32_t, std::set<Segment<LayerCoord>*>> _net_global_result_map;
};

}  // namespace irt
