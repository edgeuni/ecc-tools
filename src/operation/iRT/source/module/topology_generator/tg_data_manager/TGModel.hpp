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
#include "TGIterParam.hpp"
#include "TGNet.hpp"
#include "TGNode.hpp"

namespace irt {

class TGModel
{
 public:
  TGModel() = default;
  ~TGModel() = default;
  // getter
  std::vector<TGNet>& get_tg_net_list() { return _tg_net_list; }
  TGComParam& get_tg_com_param() { return _tg_com_param; }
  TGIterParam& get_tg_iter_param() { return _tg_iter_param; }
  std::vector<TGNet*>& get_tg_task_list() { return _tg_task_list; }
  GridMap<TGNode>& get_tg_node_map() { return _tg_node_map; }
  GridMap<double>& get_congestion_risk_map() { return _congestion_risk_map; }
  GridMap<uint8_t>& get_shadow_orient_mask_map() { return _shadow_orient_mask_map; }
  GridMap<int32_t>& get_shadow_stamp_map() { return _shadow_stamp_map; }
  int32_t get_shadow_stamp() const { return _shadow_stamp; }
  std::map<int32_t, std::set<Segment<LayerCoord>*>>& get_net_global_result_map() { return _net_global_result_map; }
  std::map<int32_t, std::vector<Segment<LayerCoord>>>& get_best_net_global_result_map() { return _best_net_global_result_map; }
  std::set<int32_t>& get_changed_net_set() { return _changed_net_set; }
  double get_curr_overflow() const { return _curr_overflow; }
  double get_curr_high_usage() const { return _curr_high_usage; }
  double get_curr_congestion_risk() const { return _curr_congestion_risk; }
  double get_curr_wire_length() const { return _curr_wire_length; }
  bool get_metric_valid() const { return _metric_valid; }
  double get_best_overflow() const { return _best_overflow; }
  double get_best_high_usage() const { return _best_high_usage; }
  double get_best_congestion_risk() const { return _best_congestion_risk; }
  double get_best_wire_length() const { return _best_wire_length; }
  // setter
  void set_tg_net_list(const std::vector<TGNet>& tg_net_list) { _tg_net_list = tg_net_list; }
  void set_tg_com_param(const TGComParam& tg_com_param) { _tg_com_param = tg_com_param; }
  void set_tg_iter_param(const TGIterParam& tg_iter_param) { _tg_iter_param = tg_iter_param; }
  void set_tg_task_list(const std::vector<TGNet*>& tg_task_list) { _tg_task_list = tg_task_list; }
  void set_tg_node_map(const GridMap<TGNode>& tg_node_map) { _tg_node_map = tg_node_map; }
  void set_congestion_risk_map(const GridMap<double>& congestion_risk_map) { _congestion_risk_map = congestion_risk_map; }
  void set_shadow_stamp(const int32_t shadow_stamp) { _shadow_stamp = shadow_stamp; }
  void set_net_global_result_map(const std::map<int32_t, std::set<Segment<LayerCoord>*>>& net_global_result_map)
  {
    _net_global_result_map = net_global_result_map;
  }
  void set_best_net_global_result_map(const std::map<int32_t, std::vector<Segment<LayerCoord>>>& best_net_global_result_map)
  {
    _best_net_global_result_map = best_net_global_result_map;
  }
  void set_curr_overflow(const double curr_overflow) { _curr_overflow = curr_overflow; }
  void set_curr_high_usage(const double curr_high_usage) { _curr_high_usage = curr_high_usage; }
  void set_curr_congestion_risk(const double curr_congestion_risk) { _curr_congestion_risk = curr_congestion_risk; }
  void set_curr_wire_length(const double curr_wire_length) { _curr_wire_length = curr_wire_length; }
  void set_metric_valid(const bool metric_valid) { _metric_valid = metric_valid; }
  void set_best_overflow(const double best_overflow) { _best_overflow = best_overflow; }
  void set_best_high_usage(const double best_high_usage) { _best_high_usage = best_high_usage; }
  void set_best_congestion_risk(const double best_congestion_risk) { _best_congestion_risk = best_congestion_risk; }
  void set_best_wire_length(const double best_wire_length) { _best_wire_length = best_wire_length; }
#if 1
  // single task
  TGNet* get_curr_tg_task() { return _curr_tg_task; }
  void set_curr_tg_task(TGNet* curr_tg_task) { _curr_tg_task = curr_tg_task; }
#endif

 private:
  std::vector<TGNet> _tg_net_list;
  TGComParam _tg_com_param;
  TGIterParam _tg_iter_param;
  std::vector<TGNet*> _tg_task_list;
  GridMap<TGNode> _tg_node_map;
  GridMap<double> _congestion_risk_map;
  GridMap<uint8_t> _shadow_orient_mask_map;
  GridMap<int32_t> _shadow_stamp_map;
  int32_t _shadow_stamp = 0;
  std::map<int32_t, std::set<Segment<LayerCoord>*>> _net_global_result_map;
  std::map<int32_t, std::vector<Segment<LayerCoord>>> _best_net_global_result_map;
  std::set<int32_t> _changed_net_set;
  double _curr_overflow = 0.0;
  double _curr_high_usage = 0.0;
  double _curr_congestion_risk = 0.0;
  double _curr_wire_length = 0.0;
  bool _metric_valid = false;
  double _best_overflow = DBL_MAX;
  double _best_high_usage = DBL_MAX;
  double _best_congestion_risk = DBL_MAX;
  double _best_wire_length = DBL_MAX;
#if 1
  // single task
  TGNet* _curr_tg_task = nullptr;
#endif
};

}  // namespace irt
