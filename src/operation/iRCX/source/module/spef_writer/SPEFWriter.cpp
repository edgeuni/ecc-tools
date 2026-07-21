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
#include "SPEFWriter.hpp"

#include "Utility.hpp"

namespace ircx {

// public

void SPEFWriter::initInst()
{
  if (_sw_instance == nullptr) {
    _sw_instance = new SPEFWriter();
  }
}

SPEFWriter& SPEFWriter::getInst()
{
  if (_sw_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_sw_instance;
}

void SPEFWriter::destroyInst()
{
  if (_sw_instance != nullptr) {
    delete _sw_instance;
    _sw_instance = nullptr;
  }
}

// function

void SPEFWriter::write()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  SWModel sw_model;
  writeSWModel(sw_model);

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

SPEFWriter* SPEFWriter::_sw_instance = nullptr;

void SPEFWriter::writeSWModel(SWModel& sw_model)
{
  SPEFNameMap spef_name_map;
  buildNameMap(spef_name_map);
  writeCornerSPEFList(sw_model, spef_name_map);
}

void SPEFWriter::buildNameMap(SPEFNameMap& spef_name_map)
{
  SpefContext& spef_context = RCXDM.getDatabase().get_spef_context();
  for (std::string& net_name : spef_context.get_net_name_list()) {
    size_t spef_id = spef_name_map.get_next_id();
    spef_name_map.get_net_name_to_id_map()[net_name] = spef_id;
    spef_name_map.get_id_to_net_name_map()[spef_id] = net_name;
    spef_name_map.set_next_id(spef_id + 1);
  }
  for (std::string& port_name : spef_context.get_port_name_list()) {
    size_t spef_id = spef_name_map.get_next_id();
    spef_name_map.get_port_name_to_id_map()[port_name] = spef_id;
    spef_name_map.get_id_to_port_name_map()[spef_id] = port_name;
    spef_name_map.set_next_id(spef_id + 1);
  }
  for (std::string& instance_name : spef_context.get_instance_name_list()) {
    size_t spef_id = spef_name_map.get_next_id();
    spef_name_map.get_instance_name_to_id_map()[instance_name] = spef_id;
    spef_name_map.get_id_to_instance_name_map()[spef_id] = instance_name;
    spef_name_map.set_next_id(spef_id + 1);
  }
}

void SPEFWriter::writeCornerSPEFList(SWModel& sw_model, SPEFNameMap& spef_name_map)
{
  for (size_t corner_idx = 0; corner_idx < RCXDM.getDatabase().get_corner_data_list().size(); corner_idx++) {
    writeCornerSPEF(sw_model, spef_name_map, corner_idx);
  }
}

void SPEFWriter::writeCornerSPEF(SWModel& sw_model, SPEFNameMap& spef_name_map, size_t corner_idx)
{
  Database& database = RCXDM.getDatabase();
  CornerData& corner_data = database.get_corner_data_list().at(corner_idx);
  std::filesystem::path spef_file_path = std::filesystem::path(RCXDM.getConfig().sw_temp_directory_path)
                                          / RCXUTIL.getString(database.get_design_name(), "_", corner_data.get_corner_name(), ".spef");
  std::ofstream* spef_file_stream = RCXUTIL.getOutputFileStream(spef_file_path.string());
  if (!spef_file_stream->is_open()) {
    RCXUTIL.closeFileStream(spef_file_stream);
    return;
  }

  buildNetCouplingRefList(sw_model, corner_idx);
  buildReportLayerList(sw_model);
  writeHeader(*spef_file_stream, corner_idx);
  writeNameMap(*spef_file_stream, spef_name_map);
  writePortList(*spef_file_stream, spef_name_map);
  writeLayerMap(sw_model, *spef_file_stream);
  writeDNetList(sw_model, *spef_file_stream, spef_name_map, corner_idx);
  RCXUTIL.closeFileStream(spef_file_stream);
  RCXLOG.info(Loc::current(), "Wrote SPEF: ", spef_file_path.string());
}

void SPEFWriter::buildNetCouplingRefList(SWModel& sw_model, size_t corner_idx)
{
  Database& database = RCXDM.getDatabase();
  TopoPool& topo_pool = database.get_topo_pool();
  size_t net_num = database.get_layout_data().get_regular_net_count();
  std::vector<std::vector<SPEFCouplingRef>>& net_coupling_ref_list = sw_model.get_net_coupling_ref_list();
  net_coupling_ref_list.assign(net_num, std::vector<SPEFCouplingRef>());

  for (auto& [coupling_key, capacitance_list] : database.get_rc_table().get_merged_ccap_map()) {
    if (corner_idx >= capacitance_list.size()) {
      continue;
    }
    F64 capacitance = capacitance_list[corner_idx];
    if (capacitance <= 0.0) {
      continue;
    }

    size_t first_edge_idx = coupling_key.get_first_edge_idx();
    size_t second_edge_idx = coupling_key.get_second_edge_idx();
    if (first_edge_idx >= topo_pool.get_edge_pool().size() || second_edge_idx >= topo_pool.get_edge_pool().size()) {
      continue;
    }

    TopoEdge& first_edge = topo_pool.get_edge(first_edge_idx);
    TopoEdge& second_edge = topo_pool.get_edge(second_edge_idx);
    if (first_edge.get_net_id() >= net_num || second_edge.get_net_id() >= net_num) {
      continue;
    }

    net_coupling_ref_list[first_edge.get_net_id()].emplace_back(first_edge_idx, second_edge_idx, capacitance);
    if (first_edge.get_net_id() != second_edge.get_net_id()) {
      net_coupling_ref_list[second_edge.get_net_id()].emplace_back(second_edge_idx, first_edge_idx, capacitance);
    }
  }
}

void SPEFWriter::buildReportLayerList(SWModel& sw_model)
{
  std::vector<SPEFReportLayer>& report_layer_list = sw_model.get_report_layer_list();
  std::unordered_map<size_t, size_t>& design_layer_id_to_report_layer_id_map = sw_model.get_design_layer_id_to_report_layer_id_map();
  report_layer_list.clear();
  design_layer_id_to_report_layer_id_map.clear();
  if (!RCXDM.getConfig().report_geometry) {
    return;
  }

  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  std::map<size_t, std::string> design_layer_id_to_name_map;
  for (std::pair<const size_t, std::string>& design_layer : layer_table.get_design_id_to_name_map()) {
    design_layer_id_to_name_map[design_layer.first] = design_layer.second;
  }
  for (std::pair<const size_t, std::string>& design_layer : design_layer_id_to_name_map) {
    size_t design_layer_id = design_layer.first;
    std::string& design_layer_name = design_layer.second;
    if (design_layer_id != 0 && layer_table.get_design_name_to_process_name_map().count(design_layer_name) == 0) {
      continue;
    }

    SPEFReportLayer report_layer;
    report_layer.set_report_layer_id(report_layer_list.size());
    report_layer.set_design_layer_id(design_layer_id);
    report_layer.set_design_layer_name(design_layer_name);
    if (design_layer_id != 0) {
      report_layer.set_process_layer_name(layer_table.get_design_name_to_process_name_map().at(design_layer_name));
    }
    design_layer_id_to_report_layer_id_map[design_layer_id] = report_layer.get_report_layer_id();
    report_layer_list.push_back(std::move(report_layer));
  }
}

void SPEFWriter::writeHeader(std::ofstream& spef_file_stream, size_t corner_idx)
{
  CornerData& corner_data = RCXDM.getDatabase().get_corner_data_list().at(corner_idx);
  std::time_t current_time = std::time(nullptr);
  std::tm* local_time = std::localtime(&current_time);
  std::stringstream date_stream;
  if (local_time != nullptr) {
    date_stream << std::put_time(local_time, "%a %b %d %H:%M:%S %Y");
  }

  spef_file_stream << "*SPEF \"IEEE 1481-1998\"\n";
  spef_file_stream << "*DESIGN \"" << RCXDM.getDatabase().get_design_name() << "\"\n";
  spef_file_stream << "*DATE \"" << date_stream.str() << "\"\n";
  spef_file_stream << "*VENDOR \"ECOS\"\n";
  spef_file_stream << "*PROGRAM \"iRCX\"\n";
  spef_file_stream << "*VERSION \"1.0\"\n";
  spef_file_stream << "*DESIGN_FLOW \"PIN_CAP NONE\"\n";
  spef_file_stream << "*DIVIDER /\n";
  spef_file_stream << "*DELIMITER :\n";
  spef_file_stream << "*BUS_DELIMITER []\n";
  spef_file_stream << "*T_UNIT 1.0 NS\n";
  spef_file_stream << "*C_UNIT 1.0 FF\n";
  spef_file_stream << "*R_UNIT 1.0 OHM\n";
  spef_file_stream << "*L_UNIT 1.0 HENRY\n";
  if (RCXDM.getConfig().report_geometry) {
    spef_file_stream << "\n// COMMENTS\n\n";
    spef_file_stream << "//   HALF_NODE_SCALING_FACTOR " << std::setprecision(6) << corner_data.get_half_node_scale_factor() << "\n";
  }
}

void SPEFWriter::writeNameMap(std::ofstream& spef_file_stream, SPEFNameMap& spef_name_map)
{
  spef_file_stream << "\n*NAME_MAP\n";
  for (std::pair<const size_t, std::string>& id_to_port_name : spef_name_map.get_id_to_port_name_map()) {
    spef_file_stream << "*" << id_to_port_name.first << " " << id_to_port_name.second << "\n";
  }
  for (std::pair<const size_t, std::string>& id_to_instance_name : spef_name_map.get_id_to_instance_name_map()) {
    spef_file_stream << "*" << id_to_instance_name.first << " " << id_to_instance_name.second << "\n";
  }
  for (std::pair<const size_t, std::string>& id_to_net_name : spef_name_map.get_id_to_net_name_map()) {
    spef_file_stream << "*" << id_to_net_name.first << " " << id_to_net_name.second << "\n";
  }
}

void SPEFWriter::writePortList(std::ofstream& spef_file_stream, SPEFNameMap& spef_name_map)
{
  SpefContext& spef_context = RCXDM.getDatabase().get_spef_context();
  spef_file_stream << "\n*PORTS\n\n";
  for (size_t port_idx = 0; port_idx < spef_context.get_port_name_list().size(); port_idx++) {
    std::string& port_name = spef_context.get_port_name_list().at(port_idx);
    char port_io = spef_context.get_port_io_list().at(port_idx);
    spef_file_stream << "*" << spef_name_map.get_port_name_to_id_map().at(port_name) << " " << port_io << "\n";
  }
}

void SPEFWriter::writeLayerMap(SWModel& sw_model, std::ofstream& spef_file_stream)
{
  if (!RCXDM.getConfig().report_geometry) {
    return;
  }

  spef_file_stream << "\n// *LAYER_MAP\n\n";
  for (SPEFReportLayer& report_layer : sw_model.get_report_layer_list()) {
    spef_file_stream << "// *" << report_layer.get_report_layer_id() << " " << report_layer.get_design_layer_name();
    if (!report_layer.get_process_layer_name().empty()) {
      spef_file_stream << "    ITF=" << report_layer.get_process_layer_name();
    }
    spef_file_stream << "\n";
  }
}

void SPEFWriter::writeDNetList(SWModel& sw_model,
                               std::ofstream& spef_file_stream,
                               SPEFNameMap& spef_name_map,
                               size_t corner_idx)
{
  size_t net_num = RCXDM.getDatabase().get_layout_data().get_regular_net_count();
  for (size_t net_idx = 0; net_idx < net_num; net_idx++) {
    writeDNet(sw_model, spef_file_stream, spef_name_map, corner_idx, net_idx);
  }
}

void SPEFWriter::writeDNet(SWModel& sw_model,
                           std::ofstream& spef_file_stream,
                           SPEFNameMap& spef_name_map,
                           size_t corner_idx,
                           size_t net_idx)
{
  Database& database = RCXDM.getDatabase();
  Net& net = database.get_layout_data().get_net_list().at(net_idx);
  TopoPool& topo_pool = database.get_topo_pool();
  std::span<TopoNode> node_list = topo_pool.get_net_node_list(net_idx);
  std::span<TopoEdge> edge_list = topo_pool.get_net_edge_list(net_idx);
  if (node_list.empty()) {
    return;
  }

  std::string net_spef_name = net.get_net_name();
  if (spef_name_map.get_net_name_to_id_map().count(net_spef_name) != 0) {
    net_spef_name = RCXUTIL.getString("*", spef_name_map.get_net_name_to_id_map().at(net_spef_name));
  }

  size_t node_offset = topo_pool.get_net_node_range(net_idx).first;
  std::vector<F64> node_ground_capacitance_list(node_list.size(), 0.0);
  std::span<F64> ground_capacitance_list = database.get_rc_table().get_corner_net_gcap_list(CornerNetId(corner_idx, net_idx));
  for (size_t edge_idx = 0; edge_idx < edge_list.size(); edge_idx++) {
    TopoEdge& edge = edge_list[edge_idx];
    if (edge.get_is_via() || ground_capacitance_list[edge_idx] <= 0.0) {
      continue;
    }
    node_ground_capacitance_list[edge.get_start_node_idx() - node_offset] += ground_capacitance_list[edge_idx] / 2.0;
    node_ground_capacitance_list[edge.get_end_node_idx() - node_offset] += ground_capacitance_list[edge_idx] / 2.0;
  }

  std::map<std::pair<size_t, std::string>, F64> node_coupling_capacitance_map;
  for (SPEFCouplingRef& coupling_ref : sw_model.get_net_coupling_ref_list().at(net_idx)) {
    TopoEdge& self_edge = topo_pool.get_edge(coupling_ref.get_self_edge_idx());
    TopoEdge& other_edge = topo_pool.get_edge(coupling_ref.get_other_edge_idx());
    if (self_edge.get_start_node_idx() == kMaxSize || self_edge.get_end_node_idx() == kMaxSize
        || other_edge.get_start_node_idx() == kMaxSize
        || other_edge.get_end_node_idx() == kMaxSize) {
      continue;
    }

    size_t self_node_idx = kMaxSize;
    size_t other_node_idx = kMaxSize;
    getNearestNodePair(self_edge, other_edge, self_node_idx, other_node_idx);
    TopoNode& other_node = topo_pool.get_node(other_node_idx);
    node_coupling_capacitance_map[
        std::make_pair(self_node_idx - node_offset, getNodeSPEFName(spef_name_map, other_node))]
        += coupling_ref.get_capacitance();
  }

  F64 total_capacitance = 0.0;
  for (F64 ground_capacitance : node_ground_capacitance_list) {
    total_capacitance += ground_capacitance;
  }
  for (std::pair<const std::pair<size_t, std::string>, F64>& node_coupling_capacitance : node_coupling_capacitance_map) {
    total_capacitance += node_coupling_capacitance.second;
  }

  double micron_per_dbu = unit::to_micron(1, database.get_layout_data().get_dbu_per_micron());
  spef_file_stream << "\n*D_NET " << net_spef_name << " " << std::fixed << std::setprecision(6) << total_capacitance << "\n\n";
  spef_file_stream << "*CONN\n";
  for (TopoNode& node : node_list) {
    if (!node.get_is_pin_node() || node.get_pin_name().find(':') != std::string::npos) {
      continue;
    }
    double x = boost::polygon::x(node.get_point()) * micron_per_dbu;
    double y = boost::polygon::y(node.get_point()) * micron_per_dbu;
    char port_io = 'B';
    for (size_t port_idx = 0; port_idx < database.get_spef_context().get_port_name_list().size(); port_idx++) {
      if (database.get_spef_context().get_port_name_list().at(port_idx) == node.get_pin_name()) {
        port_io = database.get_spef_context().get_port_io_list().at(port_idx);
        break;
      }
    }
    spef_file_stream << "*P " << getNodeSPEFName(spef_name_map, node) << " " << port_io << " *C " << std::fixed
                     << std::setprecision(3) << x << " " << y;
    writeNodeGeometry(sw_model, spef_file_stream, node, micron_per_dbu);
    spef_file_stream << "\n";
  }

  for (TopoNode& node : node_list) {
    if (!node.get_is_pin_node() || node.get_pin_name().find(':') == std::string::npos) {
      continue;
    }
    double x = boost::polygon::x(node.get_point()) * micron_per_dbu;
    double y = boost::polygon::y(node.get_point()) * micron_per_dbu;
    spef_file_stream << "*I " << getNodeSPEFName(spef_name_map, node) << " " << getPinIO(net, node.get_pin_name()) << " *C " << std::fixed
                     << std::setprecision(3) << x << " " << y;
    writeNodeGeometry(sw_model, spef_file_stream, node, micron_per_dbu);
    spef_file_stream << "\n";
  }

  for (TopoNode& node : node_list) {
    if (node.get_is_pin_node()) {
      continue;
    }
    double x = boost::polygon::x(node.get_point()) * micron_per_dbu;
    double y = boost::polygon::y(node.get_point()) * micron_per_dbu;
    spef_file_stream << "*N " << getNodeSPEFName(spef_name_map, node) << " *C " << std::fixed << std::setprecision(3) << x << " " << y;
    writeNodeGeometry(sw_model, spef_file_stream, node, micron_per_dbu);
    spef_file_stream << "\n";
  }

  spef_file_stream << "\n*CAP\n";
  size_t cap_id = 1;
  for (std::pair<const std::pair<size_t, std::string>, F64>& node_coupling_capacitance : node_coupling_capacitance_map) {
    if (node_coupling_capacitance.second <= 0.0) {
      continue;
    }
    spef_file_stream << cap_id++ << " " << getNodeSPEFName(spef_name_map, node_list[node_coupling_capacitance.first.first]) << " "
                     << node_coupling_capacitance.first.second << " " << std::setprecision(6) << node_coupling_capacitance.second << "\n";
  }
  for (size_t node_idx = 0; node_idx < node_list.size(); node_idx++) {
    if (node_ground_capacitance_list[node_idx] <= 0.0) {
      continue;
    }
    spef_file_stream << cap_id++ << " " << getNodeSPEFName(spef_name_map, node_list[node_idx]) << " " << std::setprecision(6)
                     << node_ground_capacitance_list[node_idx] << "\n";
  }

  spef_file_stream << "\n*RES\n";
  std::span<F64> resistance_list = database.get_rc_table().get_corner_net_res_list(CornerNetId(corner_idx, net_idx));
  size_t resistance_id = 1;
  for (size_t edge_idx = 0; edge_idx < edge_list.size(); edge_idx++) {
    TopoEdge& edge = edge_list[edge_idx];
    if (edge.get_start_node_idx() == kMaxSize || edge.get_end_node_idx() == kMaxSize) {
      continue;
    }
    TopoNode& start_node = topo_pool.get_node(edge.get_start_node_idx());
    TopoNode& end_node = topo_pool.get_node(edge.get_end_node_idx());
    spef_file_stream << resistance_id++ << " " << getNodeSPEFName(spef_name_map, start_node) << " "
                     << getNodeSPEFName(spef_name_map, end_node)
                     << " " << std::setprecision(6) << resistance_list[edge_idx];
    writeResistanceGeometry(sw_model, spef_file_stream, corner_idx, edge, micron_per_dbu);
    spef_file_stream << "\n";
  }
  spef_file_stream << "*END\n";
}

void SPEFWriter::getNearestNodePair(TopoEdge& self_edge, TopoEdge& other_edge, size_t& self_node_idx, size_t& other_node_idx)
{
  TopoPool& topo_pool = RCXDM.getDatabase().get_topo_pool();
  size_t self_node_idx_list[2] = {self_edge.get_start_node_idx(), self_edge.get_end_node_idx()};
  size_t other_node_idx_list[2] = {other_edge.get_start_node_idx(), other_edge.get_end_node_idx()};
  int32_t min_distance = kMaxDbu;
  for (size_t self_idx = 0; self_idx < 2; self_idx++) {
    for (size_t other_idx = 0; other_idx < 2; other_idx++) {
      TopoNode& self_node = topo_pool.get_node(self_node_idx_list[self_idx]);
      TopoNode& other_node = topo_pool.get_node(other_node_idx_list[other_idx]);
      int32_t distance = std::abs(boost::polygon::x(self_node.get_point()) - boost::polygon::x(other_node.get_point()))
                     + std::abs(boost::polygon::y(self_node.get_point()) - boost::polygon::y(other_node.get_point()));
      if (distance < min_distance) {
        min_distance = distance;
        self_node_idx = self_node_idx_list[self_idx];
        other_node_idx = other_node_idx_list[other_idx];
      }
    }
  }
}

std::string SPEFWriter::getNodeSPEFName(SPEFNameMap& spef_name_map, TopoNode& node)
{
  if (node.get_is_pin_node()) {
    std::string& pin_name = node.get_pin_name();
    size_t delimiter_pos = pin_name.find(':');
    if (delimiter_pos == std::string::npos) {
      if (spef_name_map.get_port_name_to_id_map().count(pin_name) != 0) {
        return RCXUTIL.getString("*", spef_name_map.get_port_name_to_id_map().at(pin_name));
      }
      return pin_name;
    }

    std::string instance_name = pin_name.substr(0, delimiter_pos);
    std::string instance_pin_name = pin_name.substr(delimiter_pos + 1);
    if (spef_name_map.get_instance_name_to_id_map().count(instance_name) != 0) {
      return RCXUTIL.getString("*", spef_name_map.get_instance_name_to_id_map().at(instance_name), ":", instance_pin_name);
    }
    return pin_name;
  }

  Net& node_net = RCXDM.getDatabase().get_layout_data().get_net_list().at(node.get_net_id());
  if (spef_name_map.get_net_name_to_id_map().count(node_net.get_net_name()) != 0) {
    return RCXUTIL.getString("*", spef_name_map.get_net_name_to_id_map().at(node_net.get_net_name()), ":", node.get_node_id() + 1);
  }
  return RCXUTIL.getString("*", node_net.get_net_name(), ":", node.get_node_id() + 1);
}

char SPEFWriter::getPinIO(Net& net, const std::string& pin_name)
{
  for (Pin& pin : net.get_pin_list()) {
    if (pin.get_pin_name() != pin_name) {
      continue;
    }
    if (pin.get_is_input() && pin.get_is_output()) {
      return 'B';
    }
    if (pin.get_is_input()) {
      return 'I';
    }
    if (pin.get_is_output()) {
      return 'O';
    }
  }
  return 'B';
}

void SPEFWriter::writeNodeGeometry(SWModel& sw_model, std::ofstream& spef_file_stream, TopoNode& node, double micron_per_dbu)
{
  if (!RCXDM.getConfig().report_geometry) {
    return;
  }

  GtlRectI& shape = node.get_shape();
  spef_file_stream << " // $llx=" << std::fixed << std::setprecision(3) << boost::polygon::xl(shape) * micron_per_dbu << " $lly="
                   << boost::polygon::yl(shape) * micron_per_dbu << " $urx=" << boost::polygon::xh(shape) * micron_per_dbu << " $ury="
                   << boost::polygon::yh(shape) * micron_per_dbu << " $lvl=" << getReportLayerLevel(sw_model, node.get_layer_id());
}

void SPEFWriter::writeResistanceGeometry(SWModel& sw_model, std::ofstream& spef_file_stream, size_t corner_idx, TopoEdge& edge,
                                         double micron_per_dbu)
{
  if (!RCXDM.getConfig().report_geometry) {
    return;
  }

  GtlRectI& shape = edge.get_shape();
  int32_t delta_x = boost::polygon::xh(shape) - boost::polygon::xl(shape);
  int32_t delta_y = boost::polygon::yh(shape) - boost::polygon::yl(shape);
  spef_file_stream << " // ";
  if (edge.get_is_via()) {
    double area = delta_x * delta_y * micron_per_dbu * micron_per_dbu;
    spef_file_stream << " $a=" << std::fixed << std::setprecision(6) << area << " $lvl="
                     << getReportLayerLevel(sw_model, edge.get_layer_id())
                     << " $llx=" << std::setprecision(3) << boost::polygon::xl(shape) * micron_per_dbu << " $lly="
                     << boost::polygon::yl(shape) * micron_per_dbu << " $urx=" << boost::polygon::xh(shape) * micron_per_dbu << " $ury="
                     << boost::polygon::yh(shape) * micron_per_dbu;
    return;
  }

  TopoPool& topo_pool = RCXDM.getDatabase().get_topo_pool();
  TopoNode& start_node = topo_pool.get_node(edge.get_start_node_idx());
  TopoNode& end_node = topo_pool.get_node(edge.get_end_node_idx());
  int32_t node_delta_x = std::abs(boost::polygon::x(start_node.get_point()) - boost::polygon::x(end_node.get_point()));
  int32_t node_delta_y = std::abs(boost::polygon::y(start_node.get_point()) - boost::polygon::y(end_node.get_point()));
  bool is_horizontal = node_delta_x == 0 && node_delta_y == 0 ? edge.get_line_segment().get_is_horizontal() : node_delta_x >= node_delta_y;
  int32_t axis_distance = is_horizontal ? node_delta_x : node_delta_y;
  int32_t shape_axis_distance = is_horizontal ? delta_x : delta_y;
  int32_t shape_width = is_horizontal ? delta_y : delta_x;
  CornerData& corner_data = RCXDM.getDatabase().get_corner_data_list().at(corner_idx);
  double length = (axis_distance > 0 ? axis_distance : shape_axis_distance) * micron_per_dbu * corner_data.get_half_node_scale_factor();
  double width = shape_width * micron_per_dbu;
  bool is_virtual_overlap = (node_delta_x == 0 && node_delta_y == 0) || delta_x == 0 || delta_y == 0;
  spef_file_stream << " $l=" << std::fixed << std::setprecision(3) << length << " $w=" << width << " $lvl="
                   << getReportLayerLevel(sw_model, edge.get_layer_id());
  if (is_virtual_overlap) {
    return;
  }

  spef_file_stream << " $llx=" << boost::polygon::xl(shape) * micron_per_dbu << " $lly=" << boost::polygon::yl(shape) * micron_per_dbu
                   << " $urx=" << boost::polygon::xh(shape) * micron_per_dbu << " $ury=" << boost::polygon::yh(shape) * micron_per_dbu
                   << " $dir=" << (is_horizontal ? 0 : 1);
}

size_t SPEFWriter::getReportLayerLevel(SWModel& sw_model, size_t design_layer_id)
{
  std::unordered_map<size_t, size_t>& design_layer_id_to_report_layer_id_map = sw_model.get_design_layer_id_to_report_layer_id_map();
  if (design_layer_id_to_report_layer_id_map.count(design_layer_id) != 0) {
    return design_layer_id_to_report_layer_id_map.at(design_layer_id);
  }
  return design_layer_id == kMaxSize ? 0 : design_layer_id;
}

}  // namespace ircx
