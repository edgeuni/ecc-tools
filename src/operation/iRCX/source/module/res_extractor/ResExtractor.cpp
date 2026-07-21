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
#include "ResExtractor.hpp"

#include "EdgeEtchInterval.hpp"
#include "ProcessConductor.hpp"
#include "ProcessEffectType.hpp"
#include "ProcessVia.hpp"
#include "TopoEdge.hpp"
#include "Utility.hpp"

namespace ircx {

// public

void ResExtractor::initInst()
{
  if (_re_instance == nullptr) {
    _re_instance = new ResExtractor();
  }
}

ResExtractor& ResExtractor::getInst()
{
  if (_re_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_re_instance;
}

void ResExtractor::destroyInst()
{
  if (_re_instance != nullptr) {
    delete _re_instance;
    _re_instance = nullptr;
  }
}

// function

void ResExtractor::extract()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  extractRes();

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

ResExtractor* ResExtractor::_re_instance = nullptr;

void ResExtractor::extractRes()
{
  Database& database = RCXDM.getDatabase();
  int32_t corner_num = static_cast<int32_t>(database.get_corner_data_list().size());
  int32_t net_num = database.get_layout_data().get_regular_net_count();
  database.get_rc_table().init(corner_num, net_num, database.get_topo_pool());

  for (int32_t corner_idx = 0; corner_idx < corner_num; ++corner_idx) {
    extractCornerRes(corner_idx);
  }
}

void ResExtractor::extractCornerRes(int32_t corner_idx)
{
  int32_t net_num = RCXDM.getDatabase().get_layout_data().get_regular_net_count();
  int32_t thread_num = RCXUTIL.getThreadNum(net_num, RCXDM.getConfig().thread_number);
#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
  for (int32_t net_idx = 0; net_idx < net_num; ++net_idx) {
    extractNetRes(corner_idx, net_idx);
  }
}

void ResExtractor::extractNetRes(int32_t corner_idx, int32_t net_idx)
{
  Database& database = RCXDM.getDatabase();
  CornerData& corner_data = database.get_corner_data_list().at(corner_idx);
  std::span<TopoEdge> edge_list = database.get_topo_pool().get_net_edge_list(net_idx);
  std::span<double> res_list = database.get_rc_table().get_corner_net_res_list(CornerNetId(corner_idx, net_idx));
  NetEtchProfile& net_etch_profile = database.get_corner_net_etch_profile_pool().get_item(CornerNetId(corner_idx, net_idx));

  for (int32_t edge_idx = 0; edge_idx < static_cast<int32_t>(edge_list.size()); ++edge_idx) {
    TopoEdge& edge = edge_list[static_cast<size_t>(edge_idx)];
    if (edge.get_is_via()) {
      ProcessVia* via = getProcessVia(corner_data, edge.get_layer_id());
      if (via != nullptr) {
        res_list[edge_idx] = extractViaRes(corner_data, *via, edge);
      }
      continue;
    }

    ProcessConductor* conductor = getProcessConductor(corner_data, edge.get_layer_id());
    if (conductor == nullptr) {
      continue;
    }

    std::span<EdgeEtchInterval> edge_interval_list = net_etch_profile.get_edge_interval_list(edge_idx);
    res_list[edge_idx] = extractWireRes(corner_data, *conductor, edge, edge_interval_list);
  }
}

double ResExtractor::extractWireRes(CornerData& corner_data, ProcessConductor& conductor, TopoEdge& edge,
                                    std::span<EdgeEtchInterval> edge_interval_list)
{
  Database& database = RCXDM.getDatabase();
  TopoNode& start_node = database.get_topo_pool().get_node(edge.get_start_node_idx());
  TopoNode& end_node = database.get_topo_pool().get_node(edge.get_end_node_idx());
  double micron_per_dbu = 1 / 1.0 / database.get_layout_data().get_dbu_per_micron();
  double segment_start = edge.get_line_segment().get_is_horizontal() ? RCXUTIL.x(start_node.get_point()) * micron_per_dbu
                                                                     : RCXUTIL.y(start_node.get_point()) * micron_per_dbu;
  double segment_end = edge.get_line_segment().get_is_horizontal() ? RCXUTIL.x(end_node.get_point()) * micron_per_dbu
                                                                   : RCXUTIL.y(end_node.get_point()) * micron_per_dbu;
  if (segment_end < segment_start) {
    std::swap(segment_start, segment_end);
  }

  double res = 0.0;
  for (EdgeEtchInterval& edge_interval : edge_interval_list) {
    double overlap_start = std::max(edge_interval.get_start_coordinate(), segment_start);
    double overlap_end = std::min(edge_interval.get_end_coordinate(), segment_end);
    if (overlap_end <= overlap_start) {
      continue;
    }

    double length = overlap_end - overlap_start;
    double width = edge_interval.get_width();
    double thickness = edge_interval.get_thickness();
    if (width <= 0.0 || thickness <= 0.0) {
      continue;
    }

    std::optional<double> resistivity = conductor.get_resistivity_by_width_thickness_table().query(thickness, width);
    double resistivity_value = resistivity.has_value() ? resistivity.value() : conductor.get_resistivity();
    double sheet_res_value = 0.0;
    if (resistivity_value <= 0.0) {
      std::optional<double> sheet_res = conductor.get_sheet_res_by_width_table().query(width);
      sheet_res_value = sheet_res.has_value() ? sheet_res.value() : conductor.get_sheet_res();
    }

    double base_res = 0.0;
    if (resistivity_value > 0.0) {
      base_res = resistivity_value * length / (width * thickness);
    }
    if (sheet_res_value > 0.0) {
      base_res += sheet_res_value * length / width;
    }

    double temperature_coefficient1 = 0.0;
    double temperature_coefficient2 = 0.0;
    conductor.query_temperature_coefficient(width, temperature_coefficient1, temperature_coefficient2);
    double nominal_temperature
        = conductor.get_has_nominal_temperature() ? conductor.get_nominal_temperature() : corner_data.get_global_temperature();
    res += base_res
           * getTemperatureFactor(corner_data.get_temperature(), nominal_temperature, temperature_coefficient1, temperature_coefficient2);
  }
  return res;
}

double ResExtractor::extractViaRes(CornerData& corner_data, ProcessVia& via, TopoEdge& edge)
{
  Database& database = RCXDM.getDatabase();
  double micron_per_dbu = 1 / 1.0 / database.get_layout_data().get_dbu_per_micron();
  GTLRectInt& via_shape = edge.get_shape();
  double x_span = (RCXUTIL.maxX(via_shape) - RCXUTIL.minX(via_shape)) * micron_per_dbu;
  double y_span = (RCXUTIL.maxY(via_shape) - RCXUTIL.minY(via_shape)) * micron_per_dbu;
  double length = std::max(x_span, y_span) * corner_data.get_half_node_scale_factor();
  double width = std::min(x_span, y_span) * corner_data.get_half_node_scale_factor();
  std::pair<double, double> etch_pair = via.query_etch(ProcessEffectType::kRes, width, length);
  length = std::max<double>(0.0, length - 2.0 * etch_pair.first);
  width = std::max<double>(0.0, width - 2.0 * etch_pair.second);
  double area = length * width;
  std::optional<double> base_res = via.query_res(area);
  if (!base_res.has_value()) {
    return 0.0;
  }

  double temperature_coefficient1 = 0.0;
  double temperature_coefficient2 = 0.0;
  via.query_temperature_coefficient(area, temperature_coefficient1, temperature_coefficient2);
  double nominal_temperature = via.get_has_nominal_temperature() ? via.get_nominal_temperature() : corner_data.get_global_temperature();
  return base_res.value()
         * getTemperatureFactor(corner_data.get_temperature(), nominal_temperature, temperature_coefficient1, temperature_coefficient2);
}

double ResExtractor::getTemperatureFactor(double temperature, double nominal_temperature, double temperature_coefficient1,
                                          double temperature_coefficient2)
{
  double temperature_delta = temperature - nominal_temperature;
  return 1.0 + temperature_coefficient1 * temperature_delta + temperature_coefficient2 * temperature_delta * temperature_delta;
}

ProcessVia* ResExtractor::getProcessVia(CornerData& corner_data, int32_t design_layer_id)
{
  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  std::unordered_map<int32_t, std::string>& design_id_to_name_map = layer_table.get_design_id_to_name_map();
  if (design_id_to_name_map.count(design_layer_id) == 0) {
    return nullptr;
  }

  std::string& design_layer_name = design_id_to_name_map.at(design_layer_id);
  std::unordered_map<std::string, std::string>& design_name_to_process_name_map = layer_table.get_design_name_to_process_name_map();
  if (design_name_to_process_name_map.count(design_layer_name) == 0) {
    return nullptr;
  }

  std::string& process_layer_name = design_name_to_process_name_map.at(design_layer_name);
  for (ProcessVia& via : corner_data.get_process_via_list()) {
    if (via.get_layer_name() == process_layer_name) {
      return &via;
    }
  }
  return nullptr;
}

ProcessConductor* ResExtractor::getProcessConductor(CornerData& corner_data, int32_t design_layer_id)
{
  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  std::unordered_map<int32_t, std::string>& design_id_to_name_map = layer_table.get_design_id_to_name_map();
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
