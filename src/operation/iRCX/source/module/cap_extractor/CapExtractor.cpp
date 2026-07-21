// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of the Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "CapExtractor.hpp"

namespace ircx {

// public

void CapExtractor::initInst()
{
  if (_ce_instance == nullptr) {
    _ce_instance = new CapExtractor();
  }
}

CapExtractor& CapExtractor::getInst()
{
  if (_ce_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_ce_instance;
}

void CapExtractor::destroyInst()
{
  if (_ce_instance != nullptr) {
    delete _ce_instance;
    _ce_instance = nullptr;
  }
}

// function

void CapExtractor::extract()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  extractCap();

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

CapExtractor* CapExtractor::_ce_instance = nullptr;

void CapExtractor::extractCap()
{
  size_t corner_num = RCXDM.getDatabase().get_corner_data_list().size();
  for (size_t corner_idx = 0; corner_idx < corner_num; corner_idx++) {
    extractCornerCap(corner_idx);
  }
  RCXDM.getDatabase().get_rc_table().merge_net_ccap_entry_list();
}

void CapExtractor::extractCornerCap(size_t corner_idx)
{
  size_t net_num = RCXDM.getDatabase().get_layout_data().get_regular_net_count();
  int32_t thread_num = std::max(1, std::min(RCXDM.getConfig().thread_number, static_cast<int32_t>(net_num)));
#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
  for (size_t net_idx = 0; net_idx < net_num; net_idx++) {
    extractNetCap(corner_idx, net_idx);
  }
}

void CapExtractor::extractNetCap(size_t corner_idx, size_t net_idx)
{
  std::span<TopoEdge> edge_list = RCXDM.getDatabase().get_topo_pool().get_net_edge_list(net_idx);
  for (size_t edge_idx = 0; edge_idx < edge_list.size(); edge_idx++) {
    extractEdgeCap(corner_idx, net_idx, edge_idx);
  }
}

void CapExtractor::extractEdgeCap(size_t corner_idx, size_t net_idx, size_t edge_idx)
{
  Database& database = RCXDM.getDatabase();
  std::span<TopoEdge> edge_list = database.get_topo_pool().get_net_edge_list(net_idx);
  TopoEdge& edge = edge_list[edge_idx];
  if (edge.get_is_via()) {
    return;
  }

  NetEnv& net_env = database.get_net_env_list().at(net_idx);
  NetEtchProfile& net_etch_profile = database.get_corner_net_etch_profile_pool().get_item(CornerNetId(corner_idx, net_idx));
  std::span<EdgeEnvInterval> env_interval_list = net_env.get_edge_interval_list(edge_idx);
  std::span<EdgeEtchInterval> etch_interval_list = net_etch_profile.get_edge_interval_list(edge_idx);
  size_t interval_num = std::min(env_interval_list.size(), etch_interval_list.size());
  for (size_t interval_idx = 0; interval_idx < interval_num; interval_idx++) {
    extractEdgeIntervalCap(corner_idx, net_idx, edge_idx, interval_idx);
  }
}

void CapExtractor::extractEdgeIntervalCap(size_t corner_idx, size_t net_idx, size_t edge_idx, size_t interval_idx)
{
  Database& database = RCXDM.getDatabase();
  NetEnv& net_env = database.get_net_env_list().at(net_idx);
  EdgeEnvInterval& env_interval = net_env.get_edge_interval_list(edge_idx)[interval_idx];
  std::vector<int32_t> coordinate_list;
  coordinate_list.push_back(env_interval.get_start_coordinate());
  coordinate_list.push_back(env_interval.get_end_coordinate());
  for (CrossOverlapSub& cross_overlap_sub : env_interval.get_cross_overlap_sub_list()) {
    coordinate_list.push_back(std::max(env_interval.get_start_coordinate(), cross_overlap_sub.get_start_coordinate()));
    coordinate_list.push_back(std::min(env_interval.get_end_coordinate(), cross_overlap_sub.get_end_coordinate()));
  }
  std::sort(coordinate_list.begin(), coordinate_list.end());
  coordinate_list.erase(std::unique(coordinate_list.begin(), coordinate_list.end()), coordinate_list.end());

  for (size_t coordinate_idx = 0; coordinate_idx + 1 < coordinate_list.size(); coordinate_idx++) {
    extractCapSpan(
        corner_idx, net_idx, edge_idx, interval_idx, coordinate_list[coordinate_idx], coordinate_list[coordinate_idx + 1]);
  }
}

void CapExtractor::extractCapSpan(size_t corner_idx,
                                                      size_t net_idx,
                                                      size_t edge_idx,
                                                      size_t interval_idx,
                                                      int32_t start_coordinate,
                                                      int32_t end_coordinate)
{
  if (end_coordinate <= start_coordinate) {
    return;
  }

  Database& database = RCXDM.getDatabase();
  CornerData& corner_data = database.get_corner_data_list().at(corner_idx);
  std::span<TopoEdge> edge_list = database.get_topo_pool().get_net_edge_list(net_idx);
  TopoEdge& edge = edge_list[edge_idx];
  ProcessConductor* conductor = getProcessConductor(corner_data, edge.get_layer_id());
  if (conductor == nullptr) {
    return;
  }

  NetEnv& net_env = database.get_net_env_list().at(net_idx);
  NetEtchProfile& net_etch_profile = database.get_corner_net_etch_profile_pool().get_item(CornerNetId(corner_idx, net_idx));
  EdgeEnvInterval& env_interval = net_env.get_edge_interval_list(edge_idx)[interval_idx];
  EdgeEtchInterval& etch_interval = net_etch_profile.get_edge_interval_list(edge_idx)[interval_idx];
  std::string below_layer_name;
  std::string above_layer_name;
  getCrossLayerName(
      env_interval.get_cross_overlap_sub_list(), start_coordinate, end_coordinate, below_layer_name, above_layer_name);

  CapTableConfig* cap_table_config = getCapTableConfig(corner_data, conductor->get_layer_name(), below_layer_name, above_layer_name);
  if (cap_table_config == nullptr) {
    return;
  }

  double micron_per_dbu = unit::to_micron(1, database.get_layout_data().get_dbu_per_micron());
  double span_length = (end_coordinate - start_coordinate) * micron_per_dbu;
  TopoEdge* lower_adjacent_edge = env_interval.get_lower_adjacent_edge();
  TopoEdge* upper_adjacent_edge = env_interval.get_upper_adjacent_edge();
  if (lower_adjacent_edge != nullptr && upper_adjacent_edge != nullptr) {
    F64 lower_coupling_cap = 0.0;
    F64 lower_ground_cap = 0.0;
    F64 upper_coupling_cap = 0.0;
    F64 upper_ground_cap = 0.0;
    getCap(*cap_table_config, etch_interval.get_cap_lower_spacing(), lower_coupling_cap,
                   lower_ground_cap);
    getCap(*cap_table_config, etch_interval.get_cap_upper_spacing(), upper_coupling_cap,
                   upper_ground_cap);
    addGroundCap(corner_idx, net_idx, edge_idx, lower_adjacent_edge, span_length * lower_ground_cap);
    addGroundCap(corner_idx, net_idx, edge_idx, upper_adjacent_edge, span_length * upper_ground_cap);
    addCouplingCap(corner_idx, net_idx, edge_idx, lower_adjacent_edge, span_length * lower_coupling_cap / 2.0);
    addCouplingCap(corner_idx, net_idx, edge_idx, upper_adjacent_edge, span_length * upper_coupling_cap / 2.0);
    return;
  }

  if (lower_adjacent_edge != nullptr || upper_adjacent_edge != nullptr) {
    TopoEdge* adjacent_edge = lower_adjacent_edge != nullptr ? lower_adjacent_edge : upper_adjacent_edge;
    double spacing = lower_adjacent_edge != nullptr ? etch_interval.get_cap_lower_spacing()
                                                     : etch_interval.get_cap_upper_spacing();
    F64 coupling_cap = 0.0;
    F64 ground_cap = 0.0;
    getCap(*cap_table_config, spacing, coupling_cap, ground_cap);
    addGroundCap(corner_idx, net_idx, edge_idx, adjacent_edge, 2.0 * span_length * ground_cap);
    addCouplingCap(corner_idx, net_idx, edge_idx, adjacent_edge, span_length * coupling_cap);
    return;
  }

  F64 coupling_cap = 0.0;
  F64 ground_cap = 0.0;
  getFarthestCap(*cap_table_config, coupling_cap, ground_cap);
  std::span<F64> ground_cap_list = database.get_rc_table().get_corner_net_gcap_list(CornerNetId(corner_idx, net_idx));
  ground_cap_list[edge_idx] += 2.0 * span_length * (coupling_cap + ground_cap);
}

void CapExtractor::getCrossLayerName(std::vector<CrossOverlapSub>& cross_overlap_sub_list, int32_t start_coordinate,
                                               int32_t end_coordinate, std::string& below_layer_name, std::string& above_layer_name)
{
  below_layer_name = "SUBSTRATE";
  above_layer_name.clear();
  size_t below_layer_id = 0;
  size_t above_layer_id = kMaxSize;
  for (CrossOverlapSub& cross_overlap_sub : cross_overlap_sub_list) {
    if (cross_overlap_sub.get_start_coordinate() > start_coordinate || end_coordinate > cross_overlap_sub.get_end_coordinate()) {
      continue;
    }
    if (cross_overlap_sub.get_below_layer_id() != 0
        && (below_layer_id == 0 || below_layer_id < cross_overlap_sub.get_below_layer_id())) {
      below_layer_id = cross_overlap_sub.get_below_layer_id();
    }
    if (cross_overlap_sub.get_above_layer_id() != 0 && cross_overlap_sub.get_above_layer_id() < above_layer_id) {
      above_layer_id = cross_overlap_sub.get_above_layer_id();
    }
  }

  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  if (below_layer_id != 0) {
    std::string& design_layer_name = layer_table.get_design_id_to_name_map().at(below_layer_id);
    below_layer_name = layer_table.get_design_name_to_process_name_map().at(design_layer_name);
  }
  if (above_layer_id != kMaxSize) {
    std::string& design_layer_name = layer_table.get_design_id_to_name_map().at(above_layer_id);
    above_layer_name = layer_table.get_design_name_to_process_name_map().at(design_layer_name);
  }
}

void CapExtractor::addGroundCap(size_t corner_idx, size_t net_idx, size_t edge_idx, TopoEdge* adjacent_edge,
                                                  F64 ground_cap)
{
  if (ground_cap <= 0.0) {
    return;
  }

  std::span<F64> ground_cap_list = RCXDM.getDatabase().get_rc_table().get_corner_net_gcap_list(CornerNetId(corner_idx, net_idx));
  if (adjacent_edge->get_net_id() == net_idx) {
    ground_cap_list[edge_idx] += ground_cap / 2.0;
  } else {
    ground_cap_list[edge_idx] += ground_cap;
  }
}

void CapExtractor::addCouplingCap(size_t corner_idx, size_t net_idx, size_t edge_idx, TopoEdge* adjacent_edge,
                                                    F64 coupling_cap)
{
  if (coupling_cap <= 0.0) {
    return;
  }

  Database& database = RCXDM.getDatabase();
  if (adjacent_edge->get_net_id() == kSpecialNetId) {
    std::span<F64> ground_cap_list = database.get_rc_table().get_corner_net_gcap_list(CornerNetId(corner_idx, net_idx));
    ground_cap_list[edge_idx] += coupling_cap;
  } else if (adjacent_edge->get_net_id() != net_idx) {
    size_t edge_global_idx = database.get_topo_pool().get_edge_idx(net_idx, edge_idx);
    size_t adjacent_edge_global_idx = database.get_topo_pool().get_edge_idx(*adjacent_edge);
    database.get_rc_table().append_net_ccap_entry(net_idx, edge_global_idx, adjacent_edge_global_idx, corner_idx,
                                                   static_cast<F32>(coupling_cap));
  }
}

ProcessConductor* CapExtractor::getProcessConductor(CornerData& corner_data, size_t design_layer_id)
{
  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  std::unordered_map<size_t, std::string>& design_id_to_name_map = layer_table.get_design_id_to_name_map();
  if (design_id_to_name_map.count(design_layer_id) == 0) {
    return nullptr;
  }

  std::string& design_layer_name = design_id_to_name_map.at(design_layer_id);
  std::unordered_map<std::string, std::string>& design_name_to_process_name_map = layer_table.get_design_name_to_process_name_map();
  if (design_name_to_process_name_map.count(design_layer_name) == 0) {
    return nullptr;
  }

  std::string& process_layer_name = design_name_to_process_name_map.at(design_layer_name);
  for (ProcessConductor& conductor : corner_data.get_process_conductor_list()) {
    if (conductor.get_layer_name() == process_layer_name) {
      return &conductor;
    }
  }
  return nullptr;
}

CapTableConfig* CapExtractor::getCapTableConfig(CornerData& corner_data, std::string& process_layer_name,
                                                          std::string& below_layer_name, std::string& above_layer_name)
{
  for (CapTableConfig& cap_table_config : corner_data.get_cap_table_config_list()) {
    std::string type = above_layer_name.empty() ? "A" : "B";
    if (cap_table_config.get_type() == type && cap_table_config.get_layer_name() == process_layer_name
        && cap_table_config.get_over_layer_name() == below_layer_name && cap_table_config.get_under_layer_name() == above_layer_name) {
      return &cap_table_config;
    }
  }
  return nullptr;
}

void CapExtractor::getCap(CapTableConfig& cap_table_config, double spacing, F64& coupling_cap,
                                            F64& ground_cap)
{
  std::vector<CapTableEntry>& entry_list = cap_table_config.get_entry_list();
  if (entry_list.empty()) {
    return;
  }
  spacing = std::max(spacing, 0.0);
  if (spacing > entry_list.back().get_distance()) {
    ground_cap = entry_list.back().get_ground_cap();
    return;
  }
  if (spacing <= entry_list.front().get_distance()) {
    coupling_cap = entry_list.front().get_coupling_cap();
    ground_cap = entry_list.front().get_ground_cap();
    return;
  }

  for (size_t entry_idx = 0; entry_idx + 1 < entry_list.size(); entry_idx++) {
    CapTableEntry& first_entry = entry_list[entry_idx];
    CapTableEntry& second_entry = entry_list[entry_idx + 1];
    if (first_entry.get_distance() <= spacing && spacing <= second_entry.get_distance()) {
      F64 distance_delta = second_entry.get_distance() - first_entry.get_distance();
      if (distance_delta == 0.0) {
        coupling_cap = (first_entry.get_coupling_cap() + second_entry.get_coupling_cap()) / 2.0;
        ground_cap = (first_entry.get_ground_cap() + second_entry.get_ground_cap()) / 2.0;
        return;
      }
      coupling_cap = first_entry.get_coupling_cap()
                             + (second_entry.get_coupling_cap() - first_entry.get_coupling_cap())
                                   * (spacing - first_entry.get_distance()) / distance_delta;
      ground_cap = first_entry.get_ground_cap()
                           + (second_entry.get_ground_cap() - first_entry.get_ground_cap())
                                 * (spacing - first_entry.get_distance()) / distance_delta;
      return;
    }
  }
  coupling_cap = entry_list.back().get_coupling_cap();
  ground_cap = entry_list.back().get_ground_cap();
}

void CapExtractor::getFarthestCap(CapTableConfig& cap_table_config, F64& coupling_cap,
                                                    F64& ground_cap)
{
  std::vector<CapTableEntry>& entry_list = cap_table_config.get_entry_list();
  if (entry_list.empty()) {
    return;
  }
  coupling_cap = entry_list.back().get_coupling_cap();
  ground_cap = entry_list.back().get_ground_cap();
}

}  // namespace ircx
