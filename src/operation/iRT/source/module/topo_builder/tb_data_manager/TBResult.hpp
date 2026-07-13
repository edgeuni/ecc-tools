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

#include "PlanarCoord.hpp"
#include "RTHeader.hpp"
#include "Segment.hpp"

namespace irt {

struct TBSteinerRepairStat
{
  int32_t raw_steiner_in_macro = 0;
  int32_t fixed_steiner_in_macro = 0;
  int32_t failed_steiner_legalize_num = 0;
};

class TBResult
{
 public:
  TBResult() = default;
  ~TBResult() = default;
  // getter
  std::vector<Segment<PlanarCoord>>& get_planar_topo_list() { return _planar_topo_list; }
  TBSteinerRepairStat& get_steiner_repair_stat() { return _steiner_repair_stat; }
  // const getter
  const std::vector<Segment<PlanarCoord>>& get_planar_topo_list() const { return _planar_topo_list; }
  const TBSteinerRepairStat& get_steiner_repair_stat() const { return _steiner_repair_stat; }
  // setter
  void set_planar_topo_list(std::vector<Segment<PlanarCoord>> planar_topo_list) { _planar_topo_list = std::move(planar_topo_list); }
  void set_steiner_repair_stat(const TBSteinerRepairStat& steiner_repair_stat) { _steiner_repair_stat = steiner_repair_stat; }

 private:
  std::vector<Segment<PlanarCoord>> _planar_topo_list;
  TBSteinerRepairStat _steiner_repair_stat;
};

}  // namespace irt
