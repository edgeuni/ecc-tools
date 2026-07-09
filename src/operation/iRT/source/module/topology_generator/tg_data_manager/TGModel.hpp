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

#include "RTHeader.hpp"
#include "TGComParam.hpp"
#include "TGNet.hpp"
#include "TGNode.hpp"

namespace irt {

struct TGMacroRegion
{
  std::string inst_name;
  PlanarRect body_grid_rect;
  PlanarRect halo_grid_rect;
};

struct TGMacroRepairStat
{
  int32_t raw_steiner_in_macro = 0;
  int32_t fixed_steiner_in_macro = 0;
  int32_t failed_steiner_legalize_num = 0;
  int32_t filtered_macro_cross_candidate_num = 0;
  int32_t astar_candidate_num = 0;
  int32_t failed_astar_num = 0;
  int32_t pattern_astar_macro_cross_edge_num = 0;
  std::set<int32_t> pattern_astar_macro_cross_net_set;
};

class TGModel
{
 public:
  TGModel() = default;
  ~TGModel() = default;
  // getter
  std::vector<TGNet>& get_tg_net_list() { return _tg_net_list; }
  TGComParam& get_tg_com_param() { return _tg_com_param; }
  std::vector<TGNet*>& get_tg_task_list() { return _tg_task_list; }
  GridMap<TGNode>& get_tg_node_map() { return _tg_node_map; }
  std::vector<TGMacroRegion>& get_tg_macro_region_list() { return _tg_macro_region_list; }
  GridMap<bool>& get_macro_body_forbidden_map() { return _macro_body_forbidden_map; }
  TGMacroRepairStat& get_tg_macro_repair_stat() { return _tg_macro_repair_stat; }
  GridMap<uint8_t>& get_shadow_orient_mask_map() { return _shadow_orient_mask_map; }
  GridMap<int32_t>& get_shadow_stamp_map() { return _shadow_stamp_map; }
  int32_t get_shadow_stamp() const { return _shadow_stamp; }
  // setter
  void set_tg_net_list(const std::vector<TGNet>& tg_net_list) { _tg_net_list = tg_net_list; }
  void set_tg_com_param(const TGComParam& tg_com_param) { _tg_com_param = tg_com_param; }
  void set_tg_task_list(const std::vector<TGNet*>& tg_task_list) { _tg_task_list = tg_task_list; }
  void set_tg_node_map(const GridMap<TGNode>& tg_node_map) { _tg_node_map = tg_node_map; }
  void set_tg_macro_region_list(const std::vector<TGMacroRegion>& tg_macro_region_list) { _tg_macro_region_list = tg_macro_region_list; }
  void set_macro_body_forbidden_map(const GridMap<bool>& macro_body_forbidden_map) { _macro_body_forbidden_map = macro_body_forbidden_map; }
  void set_tg_macro_repair_stat(const TGMacroRepairStat& tg_macro_repair_stat) { _tg_macro_repair_stat = tg_macro_repair_stat; }
  void set_shadow_stamp(const int32_t shadow_stamp) { _shadow_stamp = shadow_stamp; }
#if 1
  // single task
  TGNet* get_curr_tg_task() { return _curr_tg_task; }
  void set_curr_tg_task(TGNet* curr_tg_task) { _curr_tg_task = curr_tg_task; }
#endif

 private:
  std::vector<TGNet> _tg_net_list;
  TGComParam _tg_com_param;
  std::vector<TGNet*> _tg_task_list;
  GridMap<TGNode> _tg_node_map;
  std::vector<TGMacroRegion> _tg_macro_region_list;
  GridMap<bool> _macro_body_forbidden_map;
  TGMacroRepairStat _tg_macro_repair_stat;
  GridMap<uint8_t> _shadow_orient_mask_map;
  GridMap<int32_t> _shadow_stamp_map;
  int32_t _shadow_stamp = 0;
#if 1
  // single task
  TGNet* _curr_tg_task = nullptr;
#endif
};

}  // namespace irt
