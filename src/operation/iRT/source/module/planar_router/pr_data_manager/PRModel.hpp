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
#include "PRComParam.hpp"
#include "PRNet.hpp"
#include "PRNode.hpp"

namespace irt {

struct PRMacroRegion
{
  std::string inst_name;
  PlanarRect body_grid_rect;
};

class PRModel
{
 public:
  PRModel() = default;
  ~PRModel() = default;
  // getter
  std::vector<PRNet>& get_pr_net_list() { return _pr_net_list; }
  PRComParam& get_pr_com_param() { return _pr_com_param; }
  std::vector<PRNet*>& get_pr_task_list() { return _pr_task_list; }
  GridMap<PRNode>& get_pr_node_map() { return _pr_node_map; }
  std::vector<PRMacroRegion>& get_pr_macro_region_list() { return _pr_macro_region_list; }
  GridMap<bool>& get_macro_body_forbidden_map() { return _macro_body_forbidden_map; }
  const std::vector<PlanarRect>& get_macro_body_obs_list() const { return _macro_body_obs_list; }
  // setter
  void set_pr_net_list(const std::vector<PRNet>& pr_net_list) { _pr_net_list = pr_net_list; }
  void set_pr_com_param(const PRComParam& pr_com_param) { _pr_com_param = pr_com_param; }
  void set_pr_task_list(const std::vector<PRNet*>& pr_task_list) { _pr_task_list = pr_task_list; }
  void set_pr_node_map(const GridMap<PRNode>& pr_node_map) { _pr_node_map = pr_node_map; }
  void set_pr_macro_region_list(const std::vector<PRMacroRegion>& pr_macro_region_list) { _pr_macro_region_list = pr_macro_region_list; }
  void set_macro_body_forbidden_map(const GridMap<bool>& macro_body_forbidden_map) { _macro_body_forbidden_map = macro_body_forbidden_map; }
  void set_macro_body_obs_list(std::vector<PlanarRect> macro_body_obs_list) { _macro_body_obs_list = std::move(macro_body_obs_list); }
#if 1
  // single task
  PRNet* get_curr_pr_task() { return _curr_pr_task; }
  void set_curr_pr_task(PRNet* curr_pr_task) { _curr_pr_task = curr_pr_task; }
#endif

 private:
  std::vector<PRNet> _pr_net_list;
  PRComParam _pr_com_param;
  std::vector<PRNet*> _pr_task_list;
  GridMap<PRNode> _pr_node_map;
  std::vector<PRMacroRegion> _pr_macro_region_list;
  GridMap<bool> _macro_body_forbidden_map;
  std::vector<PlanarRect> _macro_body_obs_list;
#if 1
  // single task
  PRNet* _curr_pr_task = nullptr;
#endif
};

}  // namespace irt
