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
#include "FPInterface.hpp"

#include "DataManager.hpp"
#include "DieBuilder.hpp"
#include "IdbBlockages.h"
#include "idm.h"
#include "IOPlacer.hpp"
#include "Logger.hpp"
#include "MacroPlacer.hpp"
#include "Monitor.hpp"
#include "PDNGenerator.hpp"
#include "PhyPlacer.hpp"
#include "Utility.hpp"

namespace ifp {

FPInterface* FPInterface::_fp_interface_instance = nullptr;

// public

FPInterface& FPInterface::getInst()
{
  if (_fp_interface_instance == nullptr) {
    _fp_interface_instance = new FPInterface();
  }
  return *_fp_interface_instance;
}

void FPInterface::destroyInst()
{
  if (_fp_interface_instance != nullptr) {
    delete _fp_interface_instance;
    _fp_interface_instance = nullptr;
  }
}

#if 1  // 外部调用FP的API

#if 1  // iFP

void FPInterface::initFP(std::map<std::string, std::any> config_map)
{
  Logger::initInst();
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  DataManager::initInst();
  FPDM.input(config_map);

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void FPInterface::runFP()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  DieBuilder::initInst();
  FPDB.build();
  writeFloorplan();
  DieBuilder::destroyInst();

  IOPlacer::initInst();
  FPIOP.place();
  writeIO();
  IOPlacer::destroyInst();

  MacroPlacer::initInst();
  FPMP.place();
  writeMacro();
  MacroPlacer::destroyInst();

  PDNGenerator::initInst();
  FPPG.generate();
  writePDN();
  PDNGenerator::destroyInst();

  PhyPlacer::initInst();
  FPP.place();
  writePhy();
  PhyPlacer::destroyInst();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void FPInterface::destroyFP()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPDM.output();
  DataManager::destroyInst();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  FPLOG.printLogFilePath();
  Logger::destroyInst();
}

#endif

#endif

#if 1  // FP调用外部的API

#if 1  // TopData

#if 1  // input

void FPInterface::input(std::map<std::string, std::any>& config_map)
{
  wrapConfig(config_map);
  wrapDatabase();
}

void FPInterface::wrapConfig(std::map<std::string, std::any>& config_map)
{
  Config& config = FPDM.getConfig();

  config.temp_directory_path = FPUTIL.getConfigValue<std::string>(config_map, "-temp_directory_path", "./fp_temp_directory");
  config.thread_number = FPUTIL.getConfigValue<int32_t>(config_map, "-thread_number", 128);
  omp_set_num_threads(std::max(config.thread_number, 1));

#if 1  // DieBuilder

  if (config_map.contains("-core_util")) {
    config.core_util = std::any_cast<double>(config_map["-core_util"]);
  }
  if (config_map.contains("-cell_area")) {
    config.cell_area = std::any_cast<double>(config_map["-cell_area"]);
  }
  if (config_map.contains("-x_margin")) {
    config.x_margin = std::any_cast<double>(config_map["-x_margin"]);
  }
  if (config_map.contains("-y_margin")) {
    config.y_margin = std::any_cast<double>(config_map["-y_margin"]);
  }
  if (config_map.contains("-xy_ratio")) {
    config.xy_ratio = std::any_cast<double>(config_map["-xy_ratio"]);
  }
  if (config_map.contains("-die_area")) {
    config.die_area = std::any_cast<std::vector<double>>(config_map["-die_area"]);
  }
  if (config_map.contains("-core_area")) {
    config.core_area = std::any_cast<std::vector<double>>(config_map["-core_area"]);
  }
  if (config_map.contains("-core_site")) {
    config.core_site = std::any_cast<std::string>(config_map["-core_site"]);
  }
  if (config_map.contains("-io_site")) {
    config.io_site = std::any_cast<std::string>(config_map["-io_site"]);
  }
  if (config_map.contains("-corner_site")) {
    config.corner_site = std::any_cast<std::string>(config_map["-corner_site"]);
  }
  if (config_map.contains("-track_list")) {
    config.track_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-track_list"]);
  }

#endif

#if 1  // IOPlacer

  if (config_map.contains("-io_pin_layer")) {
    config.io_pin_layer = std::any_cast<std::string>(config_map["-io_pin_layer"]);
  }
  if (config_map.contains("-io_pin_width")) {
    config.io_pin_width = std::any_cast<int32_t>(config_map["-io_pin_width"]);
  }
  if (config_map.contains("-io_pin_height")) {
    config.io_pin_height = std::any_cast<int32_t>(config_map["-io_pin_height"]);
  }
  if (config_map.contains("-io_pin_side_list")) {
    config.io_pin_side_list = std::any_cast<std::vector<std::string>>(config_map["-io_pin_side_list"]);
  }
  if (config_map.contains("-io_port_list")) {
    config.io_port_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-io_port_list"]);
  }
  if (config_map.contains("-io_pad_master_list")) {
    config.io_pad_master_list = std::any_cast<std::vector<std::string>>(config_map["-io_pad_master_list"]);
  }
  if (config_map.contains("-io_corner_master_list")) {
    config.io_corner_master_list = std::any_cast<std::vector<std::string>>(config_map["-io_corner_master_list"]);
  }
  if (config_map.contains("-io_filler_name_list")) {
    config.io_filler_name_list = std::any_cast<std::vector<std::string>>(config_map["-io_filler_name_list"]);
  }
  if (config_map.contains("-io_filler_prefix")) {
    config.io_filler_prefix = std::any_cast<std::string>(config_map["-io_filler_prefix"]);
  }

#endif

#if 1  // MacroPlacer

  if (config_map.contains("-macro_halo_micron")) {
    config.macro_halo_micron = std::any_cast<double>(config_map["-macro_halo_micron"]);
  }
  if (config_map.contains("-macro_dead_space_ratio")) {
    config.macro_dead_space_ratio = std::any_cast<double>(config_map["-macro_dead_space_ratio"]);
  }
  if (config_map.contains("-macro_weight_wl")) {
    config.macro_weight_wl = std::any_cast<double>(config_map["-macro_weight_wl"]);
  }
  if (config_map.contains("-macro_weight_ol")) {
    config.macro_weight_ol = std::any_cast<double>(config_map["-macro_weight_ol"]);
  }
  if (config_map.contains("-macro_weight_ob")) {
    config.macro_weight_ob = std::any_cast<double>(config_map["-macro_weight_ob"]);
  }
  if (config_map.contains("-macro_weight_periphery")) {
    config.macro_weight_periphery = std::any_cast<double>(config_map["-macro_weight_periphery"]);
  }
  if (config_map.contains("-macro_weight_blockage")) {
    config.macro_weight_blockage = std::any_cast<double>(config_map["-macro_weight_blockage"]);
  }
  if (config_map.contains("-macro_weight_io")) {
    config.macro_weight_io = std::any_cast<double>(config_map["-macro_weight_io"]);
  }
  if (config_map.contains("-macro_max_iters")) {
    config.macro_max_iters = std::any_cast<int32_t>(config_map["-macro_max_iters"]);
  }
  if (config_map.contains("-macro_cool_rate")) {
    config.macro_cool_rate = std::any_cast<double>(config_map["-macro_cool_rate"]);
  }
  if (config_map.contains("-macro_init_temperature")) {
    config.macro_init_temperature = std::any_cast<double>(config_map["-macro_init_temperature"]);
  }
  if (config_map.contains("-placement_blockage_list")) {
    config.placement_blockage_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-placement_blockage_list"]);
  }
  if (config_map.contains("-placement_halo_list")) {
    config.placement_halo_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-placement_halo_list"]);
  }
  if (config_map.contains("-routing_blockage_list")) {
    config.routing_blockage_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-routing_blockage_list"]);
  }
  if (config_map.contains("-routing_halo_list")) {
    config.routing_halo_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-routing_halo_list"]);
  }

#endif

#if 1  // PDNGenerator

  if (config_map.contains("-pdn_io_list")) {
    config.pdn_io_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_io_list"]);
  }
  if (config_map.contains("-pdn_global_connect_list")) {
    config.pdn_global_connect_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_global_connect_list"]);
  }
  if (config_map.contains("-pdn_port_list")) {
    config.pdn_port_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_port_list"]);
  }
  if (config_map.contains("-pdn_grid_list")) {
    config.pdn_grid_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_grid_list"]);
  }
  if (config_map.contains("-pdn_stripe_list")) {
    config.pdn_stripe_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_stripe_list"]);
  }
  if (config_map.contains("-pdn_layer_list")) {
    config.pdn_layer_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_layer_list"]);
  }
  if (config_map.contains("-pdn_macro_connect_list")) {
    config.pdn_macro_connect_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_macro_connect_list"]);
  }
  if (config_map.contains("-pdn_io_pin_connect_list")) {
    config.pdn_io_pin_connect_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_io_pin_connect_list"]);
  }
  if (config_map.contains("-pdn_stripe_connect_list")) {
    config.pdn_stripe_connect_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_stripe_connect_list"]);
  }
  if (config_map.contains("-pdn_segment_stripe_list")) {
    config.pdn_segment_stripe_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_segment_stripe_list"]);
  }
  if (config_map.contains("-pdn_segment_via_list")) {
    config.pdn_segment_via_list = std::any_cast<std::vector<std::vector<std::string>>>(config_map["-pdn_segment_via_list"]);
  }

#endif

#if 1  // PhyPlacer

  if (config_map.contains("-tapcell_name")) {
    config.tapcell_name = std::any_cast<std::string>(config_map["-tapcell_name"]);
  }
  if (config_map.contains("-endcap_name")) {
    config.endcap_name = std::any_cast<std::string>(config_map["-endcap_name"]);
  }
  if (config_map.contains("-tap_distance")) {
    config.tap_distance = std::any_cast<double>(config_map["-tap_distance"]);
  }

#endif
}

void FPInterface::wrapDatabase()
{
  wrapDBInfo();
  wrapMicronDBU();
  wrapManufactureGrid();
  wrapCellArea();
  wrapFloorplan();
  wrapCellMasterMap();
  wrapRoutingLayerList();
  wrapRowList();
  wrapInstanceList();
  wrapNetList();
  wrapIOPinList();
  wrapPGNetList();
  wrapPlacementBlockageRectList();
  wrapRoutingBlockageList();
}

void FPInterface::wrapDBInfo()
{
  FPDM.getDatabase().set_design_name(dmInst->get_idb_design()->get_design_name());
}

void FPInterface::wrapMicronDBU()
{
  FPDM.getDatabase().set_micron_dbu(dmInst->get_idb_design()->get_units()->get_micron_dbu());
}

void FPInterface::wrapManufactureGrid()
{
  FPDM.getDatabase().set_manufacture_grid(dmInst->get_idb_layout()->get_munufacture_grid());
}

void FPInterface::wrapCellArea()
{
  FPDM.getDatabase().set_cell_area(dmInst->instanceArea(idb::IdbInstanceType::kMax));
}

void FPInterface::wrapFloorplan()
{
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  idb::IdbDie* idb_die = idb_layout->get_die();
  idb::IdbCore* idb_core = idb_layout->get_core();

  Die& die = FPDM.getDatabase().get_die();
  die.set_rect(idb_die->get_llx(), idb_die->get_lly(), idb_die->get_urx(), idb_die->get_ury());

  if (idb_core != nullptr && idb_core->get_bounding_box() != nullptr) {
    Core& core = FPDM.getDatabase().get_core();
    core.set_rect(idb_core->get_bounding_box()->get_low_x(), idb_core->get_bounding_box()->get_low_y(),
                  idb_core->get_bounding_box()->get_high_x(), idb_core->get_bounding_box()->get_high_y());
  }
  wrapSiteMap();
}

void FPInterface::wrapSiteMap()
{
  std::map<std::string, Site>& site_map = FPDM.getDatabase().get_site_map();
  for (idb::IdbSite* idb_site : dmInst->get_idb_layout()->get_sites()->get_site_list()) {
    Site site;
    site.set_name(idb_site->get_name());
    site.set_width(idb_site->get_width());
    site.set_height(idb_site->get_height());
    site_map[site.get_name()] = site;
  }
}

void FPInterface::wrapCellMasterMap()
{
  std::map<std::string, CellMaster>& cell_master_map = FPDM.getDatabase().get_cell_master_map();
  cell_master_map.clear();
  for (idb::IdbCellMaster* idb_cell_master : dmInst->get_idb_layout()->get_cell_master_list()->get_cell_master()) {
    CellMaster cell_master;
    cell_master.set_name(idb_cell_master->get_name());
    cell_master.set_width(idb_cell_master->get_width());
    cell_master.set_height(idb_cell_master->get_height());
    cell_master.set_pad(idb_cell_master->is_pad());
    cell_master.set_pad_filler(idb_cell_master->is_pad_filler());
    cell_master.set_spacer(idb_cell_master->is_spacer());
    cell_master.set_corner(idb_cell_master->get_site() != nullptr && idb_cell_master->get_site()->is_corner_site());
    cell_master_map[cell_master.get_name()] = cell_master;
  }
}

void FPInterface::wrapRoutingLayerList()
{
  std::vector<RoutingLayer>& routing_layer_list = FPDM.getDatabase().get_routing_layer_list();
  routing_layer_list.clear();
  for (idb::IdbLayer* idb_layer : dmInst->get_idb_layout()->get_layers()->get_routing_layers()) {
    idb::IdbLayerRouting* idb_routing_layer = dynamic_cast<idb::IdbLayerRouting*>(idb_layer);
    RoutingLayer routing_layer;
    routing_layer.set_name(idb_routing_layer->get_name());
    routing_layer.set_layer_idx(idb_routing_layer->get_id());
    routing_layer.set_order(idb_routing_layer->get_order());
    routing_layer.set_pitch_x(idb_routing_layer->get_pitch_x());
    routing_layer.set_pitch_y(idb_routing_layer->get_pitch_y());
    routing_layer.set_prefer_track_offset(idb_routing_layer->get_offset_prefer());
    routing_layer.set_spacing(idb_routing_layer->get_spacing(0));
    routing_layer.set_horizontal(idb_routing_layer->is_horizontal());

    idb::IdbTrackGrid* prefer_track_grid = idb_routing_layer->get_prefer_track_grid();
    if (prefer_track_grid != nullptr && prefer_track_grid->get_track() != nullptr) {
      routing_layer.set_prefer_track_pitch(prefer_track_grid->get_track()->get_pitch());
    }
    idb::IdbTrackGrid* nonprefer_track_grid = idb_routing_layer->get_nonprefer_track_grid();
    if (nonprefer_track_grid != nullptr && nonprefer_track_grid->get_track() != nullptr) {
      routing_layer.set_nonprefer_track_pitch(nonprefer_track_grid->get_track()->get_pitch());
    }
    routing_layer_list.push_back(routing_layer);
  }
}

void FPInterface::wrapRowList()
{
  std::vector<Row>& row_list = FPDM.getDatabase().get_row_list();
  row_list.clear();
  for (idb::IdbRow* idb_row : dmInst->get_idb_layout()->get_rows()->get_row_list()) {
    Row row;
    row.set_name(idb_row->get_name());
    row.set_site_name(idb_row->get_site()->get_name());
    row.set_y(idb_row->get_original_coordinate()->get_y());
    row.set_orient_name(idb::IdbEnum::GetInstance()->get_site_property()->get_orient_name(idb_row->get_orient()));
    row.set_rect(idb_row->get_bounding_box()->get_low_x(), idb_row->get_bounding_box()->get_low_y(), idb_row->get_bounding_box()->get_high_x(),
                 idb_row->get_bounding_box()->get_high_y());
    row_list.push_back(row);
  }
}

void FPInterface::wrapInstanceList()
{
  std::vector<Instance>& instance_list = FPDM.getDatabase().get_instance_list();
  instance_list.clear();
  for (idb::IdbInstance* idb_instance : dmInst->get_idb_design()->get_instance_list()->get_instance_list()) {
    Instance instance;
    instance.set_name(idb_instance->get_name());
    instance.set_master_name(idb_instance->get_cell_master()->get_name());
    instance.set_orient_name(idb::IdbEnum::GetInstance()->get_site_property()->get_orient_name(idb_instance->get_orient()));
    instance.set_width(idb_instance->get_cell_master()->get_width());
    instance.set_height(idb_instance->get_cell_master()->get_height());
    instance.set_macro(idb_instance->get_cell_master()->is_block());
    instance.set_fixed(idb_instance->is_fixed());
    instance.set_cover(idb_instance->is_cover());
    instance.set_placed(idb_instance->has_placed());
    if (idb_instance->has_placed()) {
      instance.set_coord(idb_instance->get_coordinate()->get_x(), idb_instance->get_coordinate()->get_y());
      idb_instance->set_bounding_box();
      instance.set_bounding_rect(idb_instance->get_bounding_box()->get_low_x(), idb_instance->get_bounding_box()->get_low_y(),
                                 idb_instance->get_bounding_box()->get_high_x(), idb_instance->get_bounding_box()->get_high_y());
    }
    if (idb_instance->has_halo()) {
      idb::IdbHalo* idb_halo = idb_instance->get_halo();
      instance.set_halo_left(idb_halo->get_extend_lef());
      instance.set_halo_right(idb_halo->get_extend_right());
      instance.set_halo_bottom(idb_halo->get_extend_bottom());
      instance.set_halo_top(idb_halo->get_extend_top());
    }
    if (instance.get_macro() && instance.get_placed()) {
      for (idb::IdbPin* idb_pin : idb_instance->get_pin_list()->get_pin_list()) {
        idb_pin->set_bounding_box();
        for (idb::IdbLayerShape* idb_layer_shape : idb_pin->get_port_box_list()) {
          for (idb::IdbRect* idb_rect : idb_layer_shape->get_rect_list()) {
            InstancePinShape pin_shape;
            pin_shape.set_pin_name(idb_pin->get_pin_name());
            pin_shape.set_layer_name(idb_layer_shape->get_layer()->get_name());
            pin_shape.set_rect(idb_rect->get_low_x(), idb_rect->get_low_y(), idb_rect->get_high_x(), idb_rect->get_high_y());
            instance.get_pin_shape_list().push_back(pin_shape);
          }
        }
      }
    }
    instance_list.push_back(instance);
  }
}

void FPInterface::wrapNetList()
{
  std::vector<Net>& net_list = FPDM.getDatabase().get_net_list();
  net_list.clear();
  for (idb::IdbNet* idb_net : dmInst->get_idb_design()->get_net_list()->get_net_list()) {
    Net net;
    net.set_name(idb_net->get_net_name());
    net.set_pin_num(idb_net->get_pin_number());
    net.set_clock(idb_net->is_clock());
    net.set_pdn(idb_net->is_pdn());
    net.set_power(idb_net->is_power());
    net.set_ground(idb_net->is_ground());

    for (idb::IdbPin* idb_pin : idb_net->get_instance_pin_list()->get_pin_list()) {
      NetPin net_pin;
      net_pin.set_instance_name(idb_pin->get_instance()->get_name());
      net_pin.set_pin_name(idb_pin->get_pin_name());
      net_pin.set_coord(idb_pin->get_average_coordinate()->get_x(), idb_pin->get_average_coordinate()->get_y());
      net_pin.set_offset_x(idb_pin->get_term()->get_average_position().get_x());
      net_pin.set_offset_y(idb_pin->get_term()->get_average_position().get_y());
      net_pin.set_placed(idb_pin->get_instance()->has_placed());
      net.get_net_pin_list().push_back(net_pin);
    }
    for (idb::IdbPin* idb_pin : idb_net->get_io_pins()->get_pin_list()) {
      NetPin net_pin;
      net_pin.set_pin_name(idb_pin->get_pin_name());
      net_pin.set_coord(idb_pin->get_average_coordinate()->get_x(), idb_pin->get_average_coordinate()->get_y());
      net_pin.set_placed(idb_pin->get_term()->is_placed());
      net_pin.set_io(true);
      net.get_net_pin_list().push_back(net_pin);
    }
    net_list.push_back(net);
  }
}

void FPInterface::wrapIOPinList()
{
  std::vector<IOPin>& io_pin_list = FPDM.getDatabase().get_io_pin_list();
  io_pin_list.clear();
  for (idb::IdbPin* idb_pin : dmInst->get_idb_design()->get_io_pin_list()->get_pin_list()) {
    idb::IdbTerm* idb_term = idb_pin->get_term();
    IOPin io_pin;
    io_pin.set_name(idb_pin->get_pin_name());
    io_pin.set_orient_name(idb::IdbEnum::GetInstance()->get_site_property()->get_orient_name(idb_pin->get_orient()));
    io_pin.set_coord(idb_pin->get_average_coordinate()->get_x(), idb_pin->get_average_coordinate()->get_y());
    io_pin.set_offset(idb_term->get_average_position().get_x(), idb_term->get_average_position().get_y());
    io_pin.set_port_exist(idb_term->is_port_exist());
    io_pin.set_special_net(idb_pin->is_special_net_pin());
    io_pin.set_placed(idb_term->is_placed());
    io_pin.set_fixed(idb_term->get_placement_status() == idb::IdbPlacementStatus::kFixed);
    idb_pin->set_bounding_box();
    for (idb::IdbLayerShape* idb_layer_shape : idb_pin->get_port_box_list()) {
      for (idb::IdbRect* idb_rect : idb_layer_shape->get_rect_list()) {
        IOPort io_port;
        io_port.set_layer_name(idb_layer_shape->get_layer()->get_name());
        io_port.set_rect(idb_rect->get_low_x(), idb_rect->get_low_y(), idb_rect->get_high_x(), idb_rect->get_high_y());
        io_pin.get_port_list().push_back(io_port);
      }
    }

    if (idb_pin->get_net() != nullptr) {
      idb::IdbInstance* idb_instance = dmInst->getIoCellByIoPin(idb_pin);
      if (idb_instance != nullptr) {
        io_pin.set_instance_name(idb_instance->get_name());
      }
    }
    io_pin_list.push_back(io_pin);
  }
}

void FPInterface::wrapPGNetList()
{
  Database& database = FPDM.getDatabase();
  std::vector<PGNet>& pg_net_list = database.get_pg_net_list();
  std::vector<PGSegment>& pg_segment_list = database.get_pg_segment_list();
  pg_net_list.clear();
  pg_segment_list.clear();

  for (idb::IdbSpecialNet* idb_special_net : dmInst->get_idb_design()->get_special_net_list()->get_net_list()) {
    PGNet pg_net;
    pg_net.set_name(idb_special_net->get_net_name());
    if (idb_special_net->is_vdd()) {
      pg_net.set_type(PGNetType::kPower);
    } else if (idb_special_net->is_vss()) {
      pg_net.set_type(PGNetType::kGround);
    }
    pg_net.set_instance_pin_name_list(idb_special_net->get_pin_string_list());
    for (idb::IdbPin* idb_pin : idb_special_net->get_io_pin_list()->get_pin_list()) {
      pg_net.add_io_pin(idb_pin->get_pin_name(), "");
    }
    pg_net_list.push_back(pg_net);

    for (idb::IdbSpecialWire* idb_special_wire : idb_special_net->get_wire_list()->get_wire_list()) {
      for (idb::IdbSpecialWireSegment* idb_segment : idb_special_wire->get_segment_list()) {
        if (!idb_segment->is_line() || idb_segment->get_layer() == nullptr) {
          continue;
        }
        PGSegment pg_segment;
        pg_segment.set_net_name(idb_special_net->get_net_name());
        pg_segment.set_layer_name(idb_segment->get_layer()->get_name());
        pg_segment.set_type(idb_segment->is_follow_pin() ? PGSegmentType::kFollowPin : PGSegmentType::kStripe);
        pg_segment.set_width(idb_segment->get_route_width());
        pg_segment.set_start_coord(idb_segment->get_point_start()->get_x(), idb_segment->get_point_start()->get_y());
        pg_segment.set_end_coord(idb_segment->get_point_second()->get_x(), idb_segment->get_point_second()->get_y());
        pg_segment_list.push_back(pg_segment);
      }
    }
  }
}

void FPInterface::wrapPlacementBlockageRectList()
{
  std::vector<PlanarRect>& placement_blockage_rect_list = FPDM.getDatabase().get_placement_blockage_rect_list();
  placement_blockage_rect_list.clear();
  for (idb::IdbBlockage* idb_blockage : dmInst->get_idb_design()->get_blockage_list()->get_blockage_list()) {
    if (!idb_blockage->is_palcement_blockage()) {
      continue;
    }
    for (idb::IdbRect* idb_rect : idb_blockage->get_rect_list()) {
      placement_blockage_rect_list.emplace_back(idb_rect->get_low_x(), idb_rect->get_low_y(), idb_rect->get_high_x(), idb_rect->get_high_y());
    }
  }
}

void FPInterface::wrapRoutingBlockageList()
{
  std::vector<Blockage>& routing_blockage_list = FPDM.getDatabase().get_routing_blockage_list();
  routing_blockage_list.clear();
  for (idb::IdbBlockage* idb_blockage : dmInst->get_idb_design()->get_blockage_list()->get_blockage_list()) {
    if (!idb_blockage->is_routing_blockage()) {
      continue;
    }
    idb::IdbRoutingBlockage* idb_routing_blockage = dynamic_cast<idb::IdbRoutingBlockage*>(idb_blockage);
    for (idb::IdbRect* idb_rect : idb_routing_blockage->get_rect_list()) {
      Blockage blockage;
      blockage.set_type(BlockageType::kRouting);
      blockage.set_rect(idb_rect->get_low_x(), idb_rect->get_low_y(), idb_rect->get_high_x(), idb_rect->get_high_y());
      blockage.set_layer_name_list({idb_routing_blockage->get_layer_name()});
      blockage.set_except_pg_net(idb_routing_blockage->is_except_pgnet());
      routing_blockage_list.push_back(blockage);
    }
  }
}

#endif

#if 1  // output

void FPInterface::output()
{
}

void FPInterface::writeFloorplan()
{
  Database& database = FPDM.getDatabase();
  if (database.is_die_updated()) {
    writeDie();
    database.set_die_updated(false);
  }
  if (database.is_core_updated()) {
    writeCore();
    writeRowList();
    database.set_core_updated(false);
  }
  if (database.is_track_updated()) {
    writeTrackList();
    database.set_track_updated(false);
  }
  wrapFloorplan();
  wrapRowList();
}

void FPInterface::writeDie()
{
  Die& die = FPDM.getDatabase().get_die();
  idb::IdbDie* idb_die = dmInst->get_idb_layout()->get_die();
  idb_die->reset();
  idb_die->add_point(die.get_ll_x(), die.get_ll_y());
  idb_die->add_point(die.get_ur_x(), die.get_ur_y());
}

void FPInterface::writeCore()
{
  Core& core = FPDM.getDatabase().get_core();
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  idb::IdbSites* idb_site_list = idb_layout->get_sites();
  idb::IdbSite* core_site = idb_site_list->find_site(core.get_core_site_name());
  idb::IdbSite* io_site = idb_site_list->find_site(core.get_io_site_name());
  idb::IdbSite* corner_site = idb_site_list->find_site(core.get_corner_site_name());

  idb_site_list->set_core_site(core_site);
  idb_site_list->set_io_site(io_site);
  idb_site_list->set_corener_site(corner_site);

  idb_layout->get_core()->set_bounding_box(core.get_ll_x(), core.get_ll_y(), core.get_ur_x(), core.get_ur_y());
}

void FPInterface::writeRowList()
{
  Database& database = FPDM.getDatabase();
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  idb_layout->get_rows()->reset();
  for (Row& row : database.get_new_row_list()) {
    Site& site = database.get_site_map()[row.get_site_name()];
    int32_t site_num = std::abs(row.get_width()) / site.get_width();
    idb::IdbOrient orient = idb::IdbEnum::GetInstance()->get_site_property()->get_orient_value(row.get_orient_name());
    dmInst->createRow(row.get_name(), row.get_site_name(), row.get_ll_x(), row.get_y(), orient, site_num, 1, site.get_width(), 0);
  }
}

void FPInterface::writeTrackList()
{
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  for (Track& track : FPDM.getDatabase().get_new_track_list()) {
    idb::IdbLayerRouting* routing_layer = dynamic_cast<idb::IdbLayerRouting*>(idb_layout->get_layers()->find_layer(track.get_layer_name()));

    idb::IdbTrackGrid* x_track_grid = idb_layout->get_track_grid_list()->add_track_grid();
    x_track_grid->add_layer_list(routing_layer);
    routing_layer->add_track_grid(x_track_grid);
    x_track_grid->get_track()->set_direction(idb::IdbTrackDirection::kDirectionX);
    x_track_grid->get_track()->set_pitch(track.get_x_pitch());
    x_track_grid->get_track()->set_start(track.get_x_offset());
    x_track_grid->set_track_number((idb_layout->get_die()->get_width() - track.get_x_offset()) / track.get_x_pitch());

    idb::IdbTrackGrid* y_track_grid = idb_layout->get_track_grid_list()->add_track_grid();
    y_track_grid->add_layer_list(routing_layer);
    routing_layer->add_track_grid(y_track_grid);
    y_track_grid->get_track()->set_direction(idb::IdbTrackDirection::kDirectionY);
    y_track_grid->get_track()->set_pitch(track.get_y_pitch());
    y_track_grid->get_track()->set_start(track.get_y_offset());
    y_track_grid->set_track_number((idb_layout->get_die()->get_height() - track.get_y_offset()) / track.get_y_pitch());
  }
}

void FPInterface::writeIO()
{
  writeIOPinList();
  writeIOInstancePlacement();
  writeNewInstanceList();
  wrapInstanceList();
  wrapIOPinList();
}

void FPInterface::writeIOPinList()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  for (IOPin& io_pin : FPDM.getDatabase().get_io_pin_list()) {
    if (!io_pin.is_updated()) {
      continue;
    }

    idb::IdbPin* idb_pin = idb_design->get_io_pin_list()->find_pin(io_pin.get_name());
    idb::IdbTerm* idb_term = idb_pin->get_term();
    if (io_pin.get_fixed()) {
      idb_term->set_placement_status_fix();
    } else if (io_pin.get_placed()) {
      idb_term->set_placement_status_place();
    }
    if (io_pin.is_offset_updated()) {
      idb_term->set_average_position(io_pin.get_offset_x(), io_pin.get_offset_y());
    }
    if (io_pin.is_port_exist_updated()) {
      idb_term->set_has_port(io_pin.get_port_exist());
    }
    if (io_pin.get_direct_location()) {
      idb_pin->set_location(io_pin.get_x(), io_pin.get_y());
    }
    idb_pin->set_average_coordinate(io_pin.get_x(), io_pin.get_y());
    idb_pin->set_orient(idb::IdbEnum::GetInstance()->get_site_property()->get_orient_value(io_pin.get_orient_name()));

    for (IOPort& io_port : io_pin.get_new_port_list()) {
      idb::IdbPort* idb_port = idb_term->add_port();
      idb_port->set_coordinate(io_port.get_x(), io_port.get_y());
      if (io_port.get_placed()) {
        idb_port->set_placement_status_place();
      }
      idb::IdbLayerShape* idb_layer_shape = idb_port->add_layer_shape();
      idb_layer_shape->set_type_rect();
      idb_layer_shape->set_layer(idb_layout->get_layers()->find_layer(io_port.get_layer_name()));
      idb_layer_shape->add_rect(io_port.get_ll_x(), io_port.get_ll_y(), io_port.get_ur_x(), io_port.get_ur_y());
    }
    idb_pin->set_bounding_box();
    io_pin.get_new_port_list().clear();
    io_pin.set_updated(false);
  }
}

void FPInterface::writeIOInstancePlacement()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  for (Instance& instance : FPDM.getDatabase().get_instance_list()) {
    if (!instance.is_placement_updated()) {
      continue;
    }
    idb::IdbOrient orient = idb::IdbEnum::GetInstance()->get_site_property()->get_orient_value(instance.get_orient_name());
    if (orient == idb::IdbOrient::kNone) {
      orient = idb::IdbOrient::kN_R0;
    }
    idb_design->placeInstance(instance.get_name(), instance.get_x(), instance.get_y(), orient, idb::IdbPlacementStatus::kPlaced);
    instance.set_placement_updated(false);
  }
}

void FPInterface::writeNewInstanceList()
{
  for (Instance& instance : FPDM.getDatabase().get_instance_list()) {
    if (!instance.is_new_instance()) {
      continue;
    }
    idb::IdbOrient orient = idb::IdbEnum::GetInstance()->get_site_property()->get_orient_value(instance.get_orient_name());
    if (orient == idb::IdbOrient::kNone) {
      orient = idb::IdbOrient::kN_R0;
    }
    dmInst->createInstance(instance.get_name(), instance.get_master_name(), instance.get_x(), instance.get_y(), orient, idb::IdbInstanceType::kDist,
                           idb::IdbPlacementStatus::kFixed);
    instance.set_new_instance(false);
  }
}

void FPInterface::writeMacro()
{
  writeMacroPlacement();
  writeInstanceHalo();
  writeNewBlockageList();
  writeNewHaloList();
  wrapInstanceList();
  wrapPlacementBlockageRectList();
  wrapRoutingBlockageList();
}

void FPInterface::writeMacroPlacement()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  for (Instance& instance : FPDM.getDatabase().get_instance_list()) {
    if (!instance.is_placement_updated()) {
      continue;
    }
    idb::IdbOrient orient = idb::IdbEnum::GetInstance()->get_site_property()->get_orient_value(instance.get_orient_name());
    if (orient == idb::IdbOrient::kNone) {
      orient = idb::IdbOrient::kN_R0;
    }
    idb_design->placeInstance(instance.get_name(), instance.get_x(), instance.get_y(), orient, idb::IdbPlacementStatus::kFixed);
    instance.set_placement_updated(false);
  }
}

void FPInterface::writeInstanceHalo()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  for (Instance& instance : FPDM.getDatabase().get_instance_list()) {
    if (!instance.is_halo_updated()) {
      continue;
    }
    idb::IdbInstance* idb_instance = idb_design->get_instance_list()->find_instance(instance.get_name());
    idb::IdbHalo* idb_halo = idb_instance->set_halo();
    idb_halo->set_extend_lef(instance.get_halo_left());
    idb_halo->set_extend_bottom(instance.get_halo_bottom());
    idb_halo->set_extend_right(instance.get_halo_right());
    idb_halo->set_extend_top(instance.get_halo_top());
    idb_instance->set_halo_coodinate();
    instance.set_halo_updated(false);
  }
}

void FPInterface::writeNewBlockageList()
{
  std::vector<Blockage>& new_blockage_list = FPDM.getDatabase().get_new_blockage_list();
  for (Blockage& blockage : new_blockage_list) {
    if (blockage.get_type() == BlockageType::kPlacement) {
      dmInst->addPlacementBlockage(blockage.get_ll_x(), blockage.get_ll_y(), blockage.get_ur_x(), blockage.get_ur_y());
    } else if (blockage.get_type() == BlockageType::kRouting) {
      dmInst->addRoutingBlockage(blockage.get_ll_x(), blockage.get_ll_y(), blockage.get_ur_x(), blockage.get_ur_y(), blockage.get_layer_name_list(),
                                 blockage.get_except_pg_net());
    }
  }
  new_blockage_list.clear();
}

void FPInterface::writeNewHaloList()
{
  std::vector<Halo>& new_halo_list = FPDM.getDatabase().get_new_halo_list();
  for (Halo& halo : new_halo_list) {
    if (halo.get_type() == HaloType::kPlacement) {
      dmInst->addPlacementHalo(halo.get_instance_name(), halo.get_top(), halo.get_bottom(), halo.get_left(), halo.get_right());
    } else if (halo.get_type() == HaloType::kRouting) {
      dmInst->addRoutingHalo(halo.get_instance_name(), halo.get_layer_name_list(), halo.get_top(), halo.get_bottom(), halo.get_left(), halo.get_right(),
                             halo.get_except_pg_net());
    }
  }
  new_halo_list.clear();
}

void FPInterface::writePDN()
{
  writePGNetList();
  writeIOPinList();
  writePGSegmentList();
}

void FPInterface::writePGNetList()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  for (PGNet& pg_net : FPDM.getDatabase().get_pg_net_list()) {
    idb::IdbConnectType connect_type = idb::IdbConnectType::kNone;
    if (pg_net.get_type() == PGNetType::kPower) {
      connect_type = idb::IdbConnectType::kPower;
    } else if (pg_net.get_type() == PGNetType::kGround) {
      connect_type = idb::IdbConnectType::kGround;
    }
    idb::IdbSpecialNet* idb_special_net = idb_design->createOrFindSpecialNet(pg_net.get_name(), connect_type);
    for (std::string& instance_pin_name : pg_net.get_instance_pin_name_list()) {
      if (std::find(idb_special_net->get_pin_string_list().begin(), idb_special_net->get_pin_string_list().end(), instance_pin_name)
          == idb_special_net->get_pin_string_list().end()) {
        idb_special_net->add_pin_string(instance_pin_name);
      }
    }
    for (std::pair<const std::string, std::string>& pair : pg_net.get_io_pin_name_to_direction_map()) {
      idb::IdbPin* idb_pin = idb_design->get_io_pin_list()->find_pin(pair.first);
      if (idb_pin == nullptr) {
        idb_pin = idb_design->createOrFindIoPin(pair.first);
      }
      idb_pin->set_as_io();
      idb::IdbTerm* idb_term = idb_pin->get_term();
      if (idb_term == nullptr) {
        idb_term = idb_pin->set_term();
      }
      if (!pair.second.empty()) {
        idb_term->set_direction(idb::IdbEnum::GetInstance()->get_connect_property()->get_direction(pair.second));
      }
      idb_term->set_type(connect_type);
      idb_design->connectPinToSpecialNet(idb_pin, idb_special_net);
    }
  }
}

void FPInterface::writePGSegmentList()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  std::map<std::string, idb::IdbSpecialWire*> pg_net_name_to_wire_map;
  for (PGSegment& pg_segment : FPDM.getDatabase().get_pg_segment_list()) {
    if (!pg_segment.get_generated()) {
      continue;
    }
    idb::IdbSpecialNet* idb_special_net = idb_design->get_special_net_list()->find_net(pg_segment.get_net_name());
    if (idb_special_net == nullptr) {
      continue;
    }
    idb::IdbSpecialWire* idb_special_wire = nullptr;
    std::map<std::string, idb::IdbSpecialWire*>::iterator wire_iter = pg_net_name_to_wire_map.find(pg_segment.get_net_name());
    if (wire_iter == pg_net_name_to_wire_map.end()) {
      idb_special_wire = idb_design->get_special_net_list()->generateWire(pg_segment.get_net_name());
      pg_net_name_to_wire_map[pg_segment.get_net_name()] = idb_special_wire;
    } else {
      idb_special_wire = wire_iter->second;
    }
    if (pg_segment.get_type() == PGSegmentType::kVia) {
      writePGVia(idb_special_wire, pg_segment);
      pg_segment.set_generated(false);
      continue;
    }
    idb::IdbLayer* idb_layer = idb_layout->get_layers()->find_layer(pg_segment.get_layer_name());
    if (idb_layer == nullptr) {
      continue;
    }
    idb::IdbSpecialWireSegment* idb_segment = idb_special_wire->add_segment();
    idb_segment->set_layer_as_new();
    idb_segment->set_layer(idb_layer);
    idb_segment->set_route_width(pg_segment.get_width());
    idb_segment->set_shape_type(pg_segment.get_type() == PGSegmentType::kFollowPin ? idb::IdbWireShapeType::kFollowPin
                                                                                     : idb::IdbWireShapeType::kStripe);
    idb_segment->add_point(pg_segment.get_start_x(), pg_segment.get_start_y());
    idb_segment->add_point(pg_segment.get_end_x(), pg_segment.get_end_y());
    idb_segment->set_bounding_box();
    pg_segment.set_generated(false);
  }
}

void FPInterface::writePGVia(idb::IdbSpecialWire* idb_special_wire, PGSegment& pg_segment)
{
  idb::IdbLayout* idb_layout = dmInst->get_idb_layout();
  std::vector<idb::IdbLayerCut*> idb_cut_layer_list;
  if (!pg_segment.get_cut_layer_name().empty()) {
    idb::IdbLayer* idb_layer = idb_layout->get_layers()->find_layer(pg_segment.get_cut_layer_name());
    idb::IdbLayerCut* idb_cut_layer = dynamic_cast<idb::IdbLayerCut*>(idb_layer);
    if (idb_cut_layer != nullptr) {
      idb_cut_layer_list.push_back(idb_cut_layer);
    }
  } else {
    idb_cut_layer_list = idb_layout->get_layers()->find_cut_layer_list(pg_segment.get_bottom_layer_name(), pg_segment.get_top_layer_name());
  }
  for (idb::IdbLayerCut* idb_cut_layer : idb_cut_layer_list) {
    std::string via_name = idb_cut_layer->get_name() + "_" + std::to_string(pg_segment.get_via_width()) + "x"
                           + std::to_string(pg_segment.get_via_height());
    idb::IdbVia* idb_via = dmInst->get_idb_design()->get_via_list()->find_via(via_name);
    if (idb_via == nullptr) {
      idb_via = dmInst->get_idb_design()->get_via_list()->createVia(via_name, idb_cut_layer, pg_segment.get_via_width(),
                                                                      pg_segment.get_via_height());
    }
    idb::IdbSpecialWireSegment* idb_segment = idb_special_wire->add_segment();
    idb_segment->set_is_via(true);
    idb_segment->add_point(pg_segment.get_start_x(), pg_segment.get_start_y());
    idb_segment->set_layer_as_new();
    idb_segment->set_layer(idb_via->get_top_layer_shape().get_layer());
    idb_segment->set_shape_type(idb::IdbWireShapeType::kStripe);
    idb_segment->set_route_width(0);
    idb::IdbVia* idb_via_copy = idb_segment->copy_via(idb_via);
    idb_via_copy->set_coordinate(pg_segment.get_start_x(), pg_segment.get_start_y());
    idb_segment->set_bounding_box();
  }
}

void FPInterface::writePhy()
{
  writeNewInstanceList();
}

#endif

#endif

#endif

}  // namespace ifp
