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

#include "RTHeader.hpp"
#include "Segment.hpp"
#include "GSRComParam.hpp"
#include "GSRGridGraph.hpp"
#include "GSRNet.hpp"

namespace irt {

class GSRModel
{
 public:
  GSRModel() = default;
  ~GSRModel() = default;
  // getter
  std::vector<GSRNet>& get_gsr_net_list() { return _gsr_net_list; }
  GSRComParam& get_gsr_com_param() { return _gsr_com_param; }
  GSRGridGraph& get_gsr_grid_graph() { return _gsr_grid_graph; }
  std::map<int32_t, int32_t>& get_net_idx_to_gsr_net_idx_map() { return _net_idx_to_gsr_net_idx_map; }
  std::vector<GridMap<double>>& get_layer_congestion_risk_map() { return _layer_congestion_risk_map; }
  std::map<int32_t, std::vector<Segment<LayerCoord>>>& get_best_net_result_map() { return _best_net_result_map; }
  std::map<int32_t, GSRTree>& get_best_net_tree_map() { return _best_net_tree_map; }
  double get_total_overflow() const { return _total_overflow; }
  double get_total_congestion_risk() const { return _total_congestion_risk; }
  double get_best_total_overflow() const { return _best_total_overflow; }
  double get_best_total_congestion_risk() const { return _best_total_congestion_risk; }
  double get_stage1_total_overflow() const { return _stage1_total_overflow; }
  double get_stage2_total_overflow() const { return _stage2_total_overflow; }
  double get_stage3_total_overflow() const { return _stage3_total_overflow; }
  // const getter
  const std::vector<GSRNet>& get_gsr_net_list() const { return _gsr_net_list; }
  const GSRComParam& get_gsr_com_param() const { return _gsr_com_param; }
  const GSRGridGraph& get_gsr_grid_graph() const { return _gsr_grid_graph; }
  const std::map<int32_t, int32_t>& get_net_idx_to_gsr_net_idx_map() const { return _net_idx_to_gsr_net_idx_map; }
  const std::vector<GridMap<double>>& get_layer_congestion_risk_map() const { return _layer_congestion_risk_map; }
  const std::map<int32_t, std::vector<Segment<LayerCoord>>>& get_best_net_result_map() const { return _best_net_result_map; }
  const std::map<int32_t, GSRTree>& get_best_net_tree_map() const { return _best_net_tree_map; }
  // setter
  void set_gsr_net_list(const std::vector<GSRNet>& gsr_net_list) { _gsr_net_list = gsr_net_list; }
  void set_gsr_com_param(const GSRComParam& gsr_com_param) { _gsr_com_param = gsr_com_param; }
  void set_gsr_grid_graph(const GSRGridGraph& gsr_grid_graph) { _gsr_grid_graph = gsr_grid_graph; }
  void set_net_idx_to_gsr_net_idx_map(const std::map<int32_t, int32_t>& net_idx_to_gsr_net_idx_map)
  {
    _net_idx_to_gsr_net_idx_map = net_idx_to_gsr_net_idx_map;
  }
  void set_layer_congestion_risk_map(const std::vector<GridMap<double>>& layer_congestion_risk_map)
  {
    _layer_congestion_risk_map = layer_congestion_risk_map;
  }
  void set_best_net_result_map(const std::map<int32_t, std::vector<Segment<LayerCoord>>>& best_net_result_map)
  {
    _best_net_result_map = best_net_result_map;
  }
  void set_best_net_tree_map(const std::map<int32_t, GSRTree>& best_net_tree_map) { _best_net_tree_map = best_net_tree_map; }
  void set_total_overflow(const double total_overflow) { _total_overflow = total_overflow; }
  void set_total_congestion_risk(const double total_congestion_risk) { _total_congestion_risk = total_congestion_risk; }
  void set_best_total_overflow(const double best_total_overflow) { _best_total_overflow = best_total_overflow; }
  void set_best_total_congestion_risk(const double best_total_congestion_risk) { _best_total_congestion_risk = best_total_congestion_risk; }
  void set_stage1_total_overflow(const double stage1_total_overflow) { _stage1_total_overflow = stage1_total_overflow; }
  void set_stage2_total_overflow(const double stage2_total_overflow) { _stage2_total_overflow = stage2_total_overflow; }
  void set_stage3_total_overflow(const double stage3_total_overflow) { _stage3_total_overflow = stage3_total_overflow; }

 private:
  std::vector<GSRNet> _gsr_net_list;
  GSRComParam _gsr_com_param;
  GSRGridGraph _gsr_grid_graph;
  std::map<int32_t, int32_t> _net_idx_to_gsr_net_idx_map;
  std::vector<GridMap<double>> _layer_congestion_risk_map;
  std::map<int32_t, std::vector<Segment<LayerCoord>>> _best_net_result_map;
  std::map<int32_t, GSRTree> _best_net_tree_map;
  double _total_overflow = 0;
  double _total_congestion_risk = 0;
  double _best_total_overflow = std::numeric_limits<double>::max();
  double _best_total_congestion_risk = std::numeric_limits<double>::max();
  double _stage1_total_overflow = 0;
  double _stage2_total_overflow = 0;
  double _stage3_total_overflow = 0;
};

}  // namespace irt
