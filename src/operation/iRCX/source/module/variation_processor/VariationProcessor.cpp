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
#include "VariationProcessor.hpp"

namespace ircx {

// public

void VariationProcessor::initInst()
{
  if (_vp_instance == nullptr) {
    _vp_instance = new VariationProcessor();
  }
}

VariationProcessor& VariationProcessor::getInst()
{
  if (_vp_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_vp_instance;
}

void VariationProcessor::destroyInst()
{
  if (_vp_instance != nullptr) {
    delete _vp_instance;
    _vp_instance = nullptr;
  }
}

// function

void VariationProcessor::process()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  VPModel vp_model = initVPModel();
  processVPModel(vp_model);

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

VariationProcessor* VariationProcessor::_vp_instance = nullptr;

VPModel VariationProcessor::initVPModel()
{
  VPModel vp_model;
  vp_model.set_database(&RCXDM.getDatabase());
  vp_model.set_temp_directory_path(RCXDM.getConfig().vp_temp_directory_path);
  return vp_model;
}

void VariationProcessor::processVPModel(VPModel&)
{
  buildCornerNetEtchProfilePool();
  applyCornerNetEffectiveGeometryList();
}

void VariationProcessor::buildCornerNetEtchProfilePool()
{
  Database& database = RCXDM.getDatabase();
  Size corner_num = database.get_corner_data_list().size();
  Size net_num = database.get_layout_data().get_regular_net_count();
  CornerNetPool<NetEtchProfile>& corner_net_etch_profile_pool = database.get_corner_net_etch_profile_pool();
  corner_net_etch_profile_pool.init(corner_num, net_num);

  for (Size corner_idx = 0; corner_idx < corner_num; corner_idx++) {
    int32_t thread_num = std::max(1, std::min(RCXDM.getConfig().thread_number, static_cast<int32_t>(net_num)));
#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
    for (Size net_idx = 0; net_idx < net_num; net_idx++) {
      buildNetEtchProfile(corner_idx, net_idx);
    }
  }
}

void VariationProcessor::buildNetEtchProfile(Size corner_idx, Size net_idx)
{
  Database& database = RCXDM.getDatabase();
  CornerData& corner_data = database.get_corner_data_list().at(corner_idx);
  NetEtchProfile& net_etch_profile = database.get_corner_net_etch_profile_pool().get_item(CornerNetId(corner_idx, net_idx));
  NetEnvironment& net_environment = database.get_net_environment_list().at(net_idx);
  std::span<TopoEdge> edge_list = database.get_topo_pool().get_net_edge_list(net_idx);
  Micron micron_per_dbu = unit::to_micron(1, database.get_layout_data().get_dbu_per_micron());

  for (Size edge_idx = 0; edge_idx < edge_list.size(); edge_idx++) {
    TopoEdge& edge = edge_list[edge_idx];
    std::vector<EdgeEtchInterval> edge_interval_list;
    if (!edge.get_is_via()) {
      ProcessConductor* conductor = getProcessConductor(corner_data, edge.get_layer_id());
      if (conductor != nullptr) {
        std::span<EdgeEnvironmentInterval> environment_interval_list = net_environment.get_edge_interval_list(edge_idx);
        for (EdgeEnvironmentInterval& environment_interval : environment_interval_list) {
          EdgeEtchInterval edge_interval;
          edge_interval.set_start_coordinate(environment_interval.get_start_coordinate() * micron_per_dbu);
          edge_interval.set_end_coordinate(environment_interval.get_end_coordinate() * micron_per_dbu);
          edge_interval.set_center(edge.get_line_segment().get_coordinate() * micron_per_dbu);
          edge_interval.set_width(edge.get_width() * micron_per_dbu);
          if (environment_interval.get_lower_adjacent_edge() != nullptr) {
            edge_interval.set_lower_spacing((environment_interval.get_lower_spacing() - edge.get_half_width()
                                              - environment_interval.get_lower_adjacent_edge()->get_half_width())
                                             * micron_per_dbu);
          }
          if (environment_interval.get_upper_adjacent_edge() != nullptr) {
            edge_interval.set_upper_spacing((environment_interval.get_upper_spacing() - edge.get_half_width()
                                              - environment_interval.get_upper_adjacent_edge()->get_half_width())
                                             * micron_per_dbu);
          }
          edge_interval.set_thickness(conductor->get_thickness());
          edge_interval_list.push_back(std::move(edge_interval));
        }
      }
    }
    net_etch_profile.append_edge_interval_list(std::move(edge_interval_list));
  }
}

void VariationProcessor::applyCornerNetEffectiveGeometryList()
{
  Database& database = RCXDM.getDatabase();
  Size corner_num = database.get_corner_data_list().size();
  Size net_num = database.get_layout_data().get_regular_net_count();
  int32_t thread_num = std::max(1, std::min(RCXDM.getConfig().thread_number, static_cast<int32_t>(net_num)));

  for (Size corner_idx = 0; corner_idx < corner_num; corner_idx++) {
#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
    for (Size net_idx = 0; net_idx < net_num; net_idx++) {
      applyNetEffectiveGeometry(corner_idx, net_idx);
    }
  }
}

void VariationProcessor::applyNetEffectiveGeometry(Size corner_idx, Size net_idx)
{
  Database& database = RCXDM.getDatabase();
  CornerData& corner_data = database.get_corner_data_list().at(corner_idx);
  NetEtchProfile& net_etch_profile = database.get_corner_net_etch_profile_pool().get_item(CornerNetId(corner_idx, net_idx));
  std::span<TopoEdge> edge_list = database.get_topo_pool().get_net_edge_list(net_idx);

  for (Size edge_idx = 0; edge_idx < edge_list.size(); edge_idx++) {
    TopoEdge& edge = edge_list[edge_idx];
    if (edge.get_is_via()) {
      continue;
    }
    ProcessConductor* conductor = getProcessConductor(corner_data, edge.get_layer_id());
    if (conductor == nullptr) {
      continue;
    }
    std::span<EdgeEtchInterval> edge_interval_list = net_etch_profile.get_edge_interval_list(edge_idx);
    for (EdgeEtchInterval& edge_interval : edge_interval_list) {
      applyEdgeEffectiveGeometry(*conductor, edge_interval);
    }
  }
}

void VariationProcessor::applyEdgeEffectiveGeometry(ProcessConductor& conductor, EdgeEtchInterval& edge_interval)
{
  for (ProcessEtchTable& etch_table : conductor.get_etch_table_list()) {
    Micron lower_etch = 0.0;
    std::optional<F64> lower_etch_value = etch_table.get_table().query(edge_interval.get_width(), edge_interval.get_lower_spacing());
    if (lower_etch_value.has_value()) {
      lower_etch = lower_etch_value.value();
    }

    Micron upper_etch = 0.0;
    std::optional<F64> upper_etch_value = etch_table.get_table().query(edge_interval.get_width(), edge_interval.get_upper_spacing());
    if (upper_etch_value.has_value()) {
      upper_etch = upper_etch_value.value();
    }

    edge_interval.set_center(edge_interval.get_center() + 0.5 * lower_etch - 0.5 * upper_etch);
    edge_interval.set_width(edge_interval.get_width() - lower_etch - upper_etch);
  }

  edge_interval.set_thickness(conductor.get_thickness());
}

ProcessConductor* VariationProcessor::getProcessConductor(CornerData& corner_data, Size design_layer_id)
{
  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  std::unordered_map<Size, std::string>& design_id_to_name_map = layer_table.get_design_id_to_name_map();
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

}  // namespace ircx
