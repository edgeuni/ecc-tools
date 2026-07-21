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
#include "RCXInterface.hpp"

#include "CapExtractor.hpp"
#include "Corner.hpp"
#include "DataManager.hpp"
#include "EnvBuilder.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "RCXHeader.hpp"
#include "ResExtractor.hpp"
#include "SPEFWriter.hpp"
#include "TopoBuilder.hpp"
#include "Utility.hpp"
#include "VarProcessor.hpp"
#include "builder.h"
#include "idm.h"

namespace ircx {

// public

RCXInterface& RCXInterface::getInst()
{
  if (_rcx_interface_instance == nullptr) {
    _rcx_interface_instance = new RCXInterface();
  }
  return *_rcx_interface_instance;
}

void RCXInterface::destroyInst()
{
  if (_rcx_interface_instance != nullptr) {
    delete _rcx_interface_instance;
    _rcx_interface_instance = nullptr;
  }
}

#if 1  // 外部调用RCX的API

#if 1  // iRCX

void RCXInterface::initRCX(std::map<std::string, std::any> config_map)
{
  Logger::initInst();
  // clang-format off
  RCXLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  RCXLOG.info(Loc::current(), "_________________________  __    _____________________________________  ");
  RCXLOG.info(Loc::current(), "___(_)__  __ \\_  ____/_  |/ /    __  ___/__  __/__    |__  __ \\__  __/");
  RCXLOG.info(Loc::current(), "__  /__  /_/ /  /    __    /     _____ \\__  /  __  /| |_  /_/ /_  /    ");
  RCXLOG.info(Loc::current(), "_  / _  _, _// /___  _    |      ____/ /_  /   _  ___ |  _, _/_  /      ");
  RCXLOG.info(Loc::current(), "/_/  /_/ |_| \\____/  /_/|_|      /____/ /_/    /_/  |_/_/ |_| /_/      ");
  RCXLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // clang-format on
  RCXLOG.printLogFilePath();
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  DataManager::initInst();
  RCXDM.input(config_map);

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void RCXInterface::runRCX()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  TopoBuilder::initInst();
  RCXTB.build();
  TopoBuilder::destroyInst();

  EnvBuilder::initInst();
  RCXEB.build();
  EnvBuilder::destroyInst();

  VarProcessor::initInst();
  RCXVP.process();
  VarProcessor::destroyInst();

  ResExtractor::initInst();
  RCXRE.extract();
  ResExtractor::destroyInst();

  CapExtractor::initInst();
  RCXCE.extract();
  CapExtractor::destroyInst();

  SPEFWriter::initInst();
  RCXSW.write();
  SPEFWriter::destroyInst();

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void RCXInterface::destroyRCX()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  RCXDM.output();
  DataManager::destroyInst();

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());

  RCXLOG.printLogFilePath();
  // clang-format off
  RCXLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  RCXLOG.info(Loc::current(), "_________________________  __    _____________________   _____________________  __ ");
  RCXLOG.info(Loc::current(), "___(_)__  __ \\_  ____/_  |/ /    ___  ____/___  _/__  | / /___  _/_  ___/__  / / /");
  RCXLOG.info(Loc::current(), "__  /__  /_/ /  /    __    /     __  /_    __  / __   |/ / __  / _____ \\__  /_/ / ");
  RCXLOG.info(Loc::current(), "_  / _  _, _// /___  _    |      _  __/   __/ /  _  /|  / __/ /  ____/ /_  __  /   ");
  RCXLOG.info(Loc::current(), "/_/  /_/ |_| \\____/  /_/|_|      /_/      /___/  /_/ |_/  /___/  /____/ /_/ /_/   ");
  RCXLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // clang-format on
  Logger::destroyInst();
}

void RCXInterface::compareSpef(std::map<std::string, std::any>)
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void RCXInterface::dumpNetShape()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void RCXInterface::runRCXFromTopo(std::map<std::string, std::any>)
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void RCXInterface::plotSpef(std::map<std::string, std::any>)
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

#endif

#endif

#if 1  // RCX调用外部的API

#if 1  // TopData

#if 1  // input

void RCXInterface::input(std::map<std::string, std::any>& config_map)
{
  wrapConfig(config_map);
  wrapDatabase();
}

void RCXInterface::wrapConfig(std::map<std::string, std::any>& config_map)
{
  Config& config = RCXDM.getConfig();

  // 配置文件
  std::filesystem::path config_file_path = std::filesystem::absolute(RCXUTIL.getConfigValue<std::string>(config_map, "-config", ""));
  config.config_file_path = config_file_path.string();

  std::ifstream config_file_stream(config_file_path);
  if (!config_file_stream.is_open()) {
    RCXLOG.error(Loc::current(), "Failed to open config file '", config.config_file_path, "'!");
  }

  nlohmann::json config_json;
  config_file_stream >> config_json;

  std::filesystem::path config_directory_path = config_file_path.parent_path();

  // 通用配置
  config.thread_number = std::max(config_json.at("thread_num").get<int32_t>(), 1);
  if (config_json.contains("output")) {
    std::string output_directory_path = config_json.at("output").get<std::string>();
    if (!output_directory_path.empty()) {
      config.output_directory_path = RCXUTIL.getAbsolutePath(config_directory_path, output_directory_path);
      config.temp_directory_path = config.output_directory_path;
    }
  }
  if (config_json.contains("report_geometry")) {
    config.report_geometry = config_json.at("report_geometry").get<bool>();
  }
  config.mapping_file_path = RCXUTIL.getAbsolutePath(config_directory_path, config_json.at("mapping_file").get<std::string>());

  // 工艺角配置
  for (nlohmann::json& corner_json : config_json.at("corners")) {
    Corner corner;
    corner.set_corner_name(corner_json.at("name").get<std::string>());
    if (corner_json.contains("temperature")) {
      std::vector<double> temperature_list;
      for (nlohmann::json& temperature_json : corner_json.at("temperature")) {
        temperature_list.push_back(temperature_json.get<double>());
      }
      corner.set_temperature_list(temperature_list);
    }
    corner.set_itf_file_path(RCXUTIL.getAbsolutePath(config_directory_path, corner_json.at("itf_file").get<std::string>()));
    corner.set_captab_file_path(RCXUTIL.getAbsolutePath(config_directory_path, corner_json.at("captab_file").get<std::string>()));
    config.corner_list.push_back(std::move(corner));
  }

  omp_set_num_threads(config.thread_number);
}

#if 1  // database

void RCXInterface::wrapDatabase()
{
  if (dmInst == nullptr) {
    return;
  }

  wrapDBInfo();
  wrapLayerList();
  wrapSpefContext();
  wrapNetList();
  wrapSpecialNet();
}

void RCXInterface::wrapDBInfo()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_def_service()->get_design();
  idb::IdbLayout* idb_layout = dmInst->get_idb_lef_service()->get_layout();
  if (idb_design == nullptr || idb_layout == nullptr) {
    return;
  }

  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  layout_data.set_design_name(idb_design->get_design_name());

  idb::IdbDie* idb_die = idb_layout->get_die();
  if (idb_die != nullptr) {
    idb::IdbRect* idb_die_shape = idb_die->get_bounding_box();
    layout_data.set_die_shape(
        GtlRectI(idb_die_shape->get_low_x(), idb_die_shape->get_low_y(), idb_die_shape->get_high_x(), idb_die_shape->get_high_y()));
  }

  idb::IdbUnits* idb_units = idb_design->get_units();
  if (idb_units != nullptr) {
    layout_data.set_dbu_per_micron(idb_units->get_micron_dbu());
  }
}

void RCXInterface::wrapLayerList()
{
  idb::IdbLayers* idb_layers = dmInst->get_idb_lef_service()->get_layout()->get_layers();
  if (idb_layers == nullptr) {
    return;
  }

  LayerTable& layer_table = RCXDM.getDatabase().get_layer_table();
  layer_table.register_design_layer(0, "SUBSTRATE");

  size_t layer_id = 1;
  for (idb::IdbLayer* idb_layer : idb_layers->get_routing_layers()) {
    layer_table.register_design_layer(layer_id, idb_layer->get_name());
    layer_id++;
  }
  for (idb::IdbLayer* idb_layer : idb_layers->get_cut_layers()) {
    layer_table.register_design_layer(layer_id, idb_layer->get_name());
    layer_id++;
  }

  for (idb::IdbLayer* idb_layer : idb_layers->get_routing_layers()) {
    idb::IdbLayerRouting* idb_routing_layer = dynamic_cast<idb::IdbLayerRouting*>(idb_layer);
    if (idb_routing_layer == nullptr) {
      continue;
    }
    wrapRoutingLayer(idb_routing_layer);
  }
}

void RCXInterface::wrapRoutingLayer(idb::IdbLayerRouting* idb_routing_layer)
{
  RoutingLayer routing_layer;
  routing_layer.set_layer_id(RCXDM.getDatabase().get_layer_table().get_design_id(idb_routing_layer->get_name()));
  routing_layer.set_layer_name(idb_routing_layer->get_name());
  routing_layer.set_layer_width(idb_routing_layer->get_width());
  if (idb_routing_layer->is_horizontal()) {
    routing_layer.set_is_prefer_horizontal(true);
  }

  TrackInfo track_info;
  for (idb::IdbTrackGrid* idb_track_grid : idb_routing_layer->get_track_grid_list()) {
    idb::IdbTrack* idb_track = idb_track_grid->get_track();
    if (idb_track->get_direction() == idb::IdbTrackDirection::kDirectionX) {
      track_info.set_x_start(idb_track->get_start());
      track_info.set_x_step(idb_track->get_pitch());
      track_info.set_x_count(idb_track_grid->get_track_num());
    } else if (idb_track->get_direction() == idb::IdbTrackDirection::kDirectionY) {
      track_info.set_y_start(idb_track->get_start());
      track_info.set_y_step(idb_track->get_pitch());
      track_info.set_y_count(idb_track_grid->get_track_num());
    }
  }
  routing_layer.set_track_info(track_info);
  RCXDM.getDatabase().get_layout_data().get_routing_layer_map()[routing_layer.get_layer_id()] = std::move(routing_layer);
}

void RCXInterface::wrapSpefContext()
{
  idb::IdbDesign* idb_design = dmInst->get_idb_def_service()->get_design();
  if (idb_design == nullptr) {
    return;
  }

  SpefContext& spef_context = RCXDM.getDatabase().get_spef_context();
  for (idb::IdbNet* idb_net : idb_design->get_net_list()->get_net_list()) {
    if (idb_net->is_pdn()) {
      continue;
    }
    spef_context.get_net_name_list().push_back(getSpefName(idb_net->get_net_name()));
  }

  for (idb::IdbPin* idb_pin : idb_design->get_io_pin_list()->get_pin_list()) {
    if (idb_pin->is_special_net_pin() || idb_pin->get_net() == nullptr) {
      continue;
    }

    spef_context.get_port_name_list().push_back(getSpefName(idb_pin->get_pin_name()));
    if (idb_pin->is_primary_input()) {
      spef_context.get_port_io_list().push_back('I');
    } else if (idb_pin->is_primary_output()) {
      spef_context.get_port_io_list().push_back('O');
    } else {
      spef_context.get_port_io_list().push_back('B');
    }
  }

  for (idb::IdbInstance* idb_instance : idb_design->get_instance_list()->get_instance_list()) {
    std::string instance_name = getSpefName(idb_instance->get_name());
    spef_context.get_instance_name_list().push_back(instance_name);
    spef_context.get_instance_name_to_cell_name_map()[instance_name] = getSpefName(idb_instance->get_cell_master()->get_name());
  }
}

void RCXInterface::wrapNetList()
{
  idb::IdbNetList* idb_net_list = dmInst->get_idb_def_service()->get_design()->get_net_list();
  if (idb_net_list == nullptr) {
    return;
  }

  std::vector<idb::IdbNet*>& idb_net_list_ref = idb_net_list->get_net_list();
  std::vector<Net>& net_list = RCXDM.getDatabase().get_layout_data().get_net_list();
  net_list.resize(idb_net_list_ref.size());
  for (size_t net_idx = 0; net_idx < idb_net_list_ref.size(); net_idx++) {
    wrapNet(net_list[net_idx], idb_net_list_ref[net_idx], net_idx);
  }
}

void RCXInterface::wrapNet(Net& net, idb::IdbNet* idb_net, std::size_t net_idx)
{
  net.set_net_id(net_idx);
  net.set_net_name(getSpefName(idb_net->get_net_name()));
  wrapPinList(net, idb_net);
  wrapSegmentList(net, idb_net);
}

void RCXInterface::wrapPinList(Net& net, idb::IdbNet* idb_net)
{
  idb::IdbPin* idb_driver_pin = idb_net->get_driving_pin();
  if (idb_driver_pin != nullptr) {
    wrapPin(net, idb_driver_pin, true);
  }

  for (idb::IdbPin* idb_load_pin : idb_net->get_load_pins()) {
    if (idb_load_pin == nullptr) {
      continue;
    }
    wrapPin(net, idb_load_pin, false);
  }
}

void RCXInterface::wrapPin(Net& net, idb::IdbPin* idb_pin, bool is_driver)
{
  Pin pin;
  if (idb_pin->is_io_pin()) {
    pin.set_pin_name(getSpefName(idb_pin->get_pin_name()));
  } else {
    pin.set_pin_name(RCXUTIL.getString(getSpefName(idb_pin->get_instance()->get_name()), ":", getSpefName(idb_pin->get_pin_name())));
  }
  pin.set_is_driver(is_driver);

  idb::IdbTerm* idb_term = idb_pin->get_term();
  if (idb_term != nullptr) {
    idb::IdbConnectDirection direction = idb_term->get_direction();
    if (direction == idb::IdbConnectDirection::kInput) {
      pin.set_is_input(true);
    } else if (direction == idb::IdbConnectDirection::kOutput || direction == idb::IdbConnectDirection::kOutputTriState) {
      pin.set_is_output(true);
    } else if (direction == idb::IdbConnectDirection::kInOut || direction == idb::IdbConnectDirection::kFeedThru) {
      pin.set_is_input(true);
      pin.set_is_output(true);
    }
  }

  for (idb::IdbLayerShape* idb_layer_shape : idb_pin->get_port_box_list()) {
    if (idb_layer_shape == nullptr || idb_layer_shape->get_layer() == nullptr) {
      continue;
    }

    size_t layer_id = RCXDM.getDatabase().get_layer_table().get_design_id(
        idb_layer_shape->get_layer()->get_name());
    for (idb::IdbRect* idb_rect : idb_layer_shape->get_rect_list()) {
      if (idb_rect == nullptr) {
        continue;
      }
      pin.get_layer_shape_list().emplace_back(
          layer_id, GtlRectI(idb_rect->get_low_x(), idb_rect->get_low_y(), idb_rect->get_high_x(), idb_rect->get_high_y()));
    }
  }
  net.get_pin_list().push_back(std::move(pin));
}

std::string RCXInterface::getSpefName(std::string name)
{
  if (name.find('.') == std::string::npos) {
    return name;
  }

  std::string spef_name;
  spef_name.reserve(name.size());
  for (size_t name_idx = 0; name_idx < name.size(); name_idx++) {
    char name_char = name[name_idx];
    if ((name_char == '.' || name_char == '[' || name_char == ']')
        && (name_idx == 0 || name[name_idx - 1] != '\\')) {
      spef_name.push_back('\\');
    }
    spef_name.push_back(name_char);
  }
  return spef_name;
}

void RCXInterface::wrapSegmentList(Net& net, idb::IdbNet* idb_net)
{
  idb::IdbRegularWireList* idb_wire_list = idb_net->get_wire_list();
  if (idb_wire_list == nullptr || idb_wire_list->get_num() == 0) {
    return;
  }

  for (idb::IdbRegularWire* idb_wire : idb_wire_list->get_wire_list()) {
    for (idb::IdbRegularWireSegment* idb_segment : idb_wire->get_segment_list()) {
      if (idb_segment->is_wire()) {
        wrapSegment(net, idb_segment);
      }
      if (idb_segment->is_rect()) {
        wrapPatch(net, idb_segment);
      }
      wrapViaList(net, idb_segment);
    }
  }
}

void RCXInterface::wrapSegment(Net& net, idb::IdbRegularWireSegment* idb_segment)
{
  idb::IdbCoordinate<int32_t>* idb_start_point = idb_segment->get_point_start();
  idb::IdbCoordinate<int32_t>* idb_end_point = idb_segment->get_point_end();
  idb::IdbLayer* idb_layer = idb_segment->get_layer();
  if (idb_start_point == nullptr || idb_end_point == nullptr || idb_layer == nullptr) {
    return;
  }

  idb::IdbRect idb_shape = idb_segment->get_segment_rect();
  Segment segment;
  segment.set_layer_id(RCXDM.getDatabase().get_layer_table().get_design_id(idb_layer->get_name()));
  segment.set_start_point(GtlPointI(idb_start_point->get_x(), idb_start_point->get_y()));
  segment.set_end_point(GtlPointI(idb_end_point->get_x(), idb_end_point->get_y()));
  segment.set_shape(GtlRectI(idb_shape.get_low_x(), idb_shape.get_low_y(), idb_shape.get_high_x(), idb_shape.get_high_y()));
  net.get_segment_list().push_back(std::move(segment));
}

void RCXInterface::wrapPatch(Net& net, idb::IdbRegularWireSegment* idb_segment)
{
  idb::IdbRect* idb_delta_shape = idb_segment->get_delta_rect();
  idb::IdbCoordinate<int32_t>* idb_anchor_point = idb_segment->get_point(0);
  idb::IdbLayer* idb_layer = idb_segment->get_layer();
  if (idb_delta_shape == nullptr || idb_anchor_point == nullptr || idb_layer == nullptr) {
    return;
  }

  Patch patch;
  patch.set_layer_id(RCXDM.getDatabase().get_layer_table().get_design_id(idb_layer->get_name()));
  patch.set_shape(GtlRectI(idb_anchor_point->get_x() + idb_delta_shape->get_low_x(),
                           idb_anchor_point->get_y() + idb_delta_shape->get_low_y(),
                           idb_anchor_point->get_x() + idb_delta_shape->get_high_x(),
                           idb_anchor_point->get_y() + idb_delta_shape->get_high_y()));
  net.get_patch_list().push_back(std::move(patch));
}

void RCXInterface::wrapViaList(Net& net, idb::IdbRegularWireSegment* idb_segment)
{
  for (idb::IdbVia* idb_via : idb_segment->get_via_list()) {
    wrapVia(net, idb_via);
  }
}

void RCXInterface::wrapVia(Net& net, idb::IdbVia* idb_via)
{
  if (idb_via == nullptr || idb_via->get_coordinate() == nullptr) {
    return;
  }

  idb::IdbLayerShape idb_top_layer_shape = idb_via->get_top_layer_shape();
  idb::IdbLayerShape idb_cut_layer_shape = idb_via->get_cut_layer_shape();
  idb::IdbLayerShape idb_bottom_layer_shape = idb_via->get_bottom_layer_shape();
  if (idb_top_layer_shape.get_layer() == nullptr || idb_cut_layer_shape.get_layer() == nullptr
      || idb_bottom_layer_shape.get_layer() == nullptr
      || idb_top_layer_shape.get_rect_list().size() != 1 || idb_cut_layer_shape.get_rect_list().size() != 1
      || idb_bottom_layer_shape.get_rect_list().size() != 1) {
    return;
  }

  Via via;
  via.set_via_name(getSpefName(idb_via->get_name()));
  via.set_point(GtlPointI(idb_via->get_coordinate()->get_x(), idb_via->get_coordinate()->get_y()));

  idb::IdbRect* idb_top_shape = idb_top_layer_shape.get_rect_list().front();
  idb::IdbRect* idb_cut_shape = idb_cut_layer_shape.get_rect_list().front();
  idb::IdbRect* idb_bottom_shape = idb_bottom_layer_shape.get_rect_list().front();
  via.set_top_layer_shape(
      {RCXDM.getDatabase().get_layer_table().get_design_id(idb_top_layer_shape.get_layer()->get_name()),
       GtlRectI(idb_top_shape->get_low_x(), idb_top_shape->get_low_y(), idb_top_shape->get_high_x(), idb_top_shape->get_high_y())});
  via.set_cut_layer_shape(
      {RCXDM.getDatabase().get_layer_table().get_design_id(idb_cut_layer_shape.get_layer()->get_name()),
       GtlRectI(idb_cut_shape->get_low_x(), idb_cut_shape->get_low_y(), idb_cut_shape->get_high_x(), idb_cut_shape->get_high_y())});
  via.set_bottom_layer_shape(
      {RCXDM.getDatabase().get_layer_table().get_design_id(idb_bottom_layer_shape.get_layer()->get_name()),
       GtlRectI(idb_bottom_shape->get_low_x(),
                idb_bottom_shape->get_low_y(),
                idb_bottom_shape->get_high_x(),
                idb_bottom_shape->get_high_y())});
  net.get_via_list().push_back(std::move(via));
}

void RCXInterface::wrapSpecialNet()
{
  idb::IdbSpecialNetList* idb_special_net_list = dmInst->get_idb_def_service()->get_design()->get_special_net_list();
  if (idb_special_net_list == nullptr) {
    return;
  }

  Net& special_net = RCXDM.getDatabase().get_layout_data().get_special_net();
  for (idb::IdbSpecialNet* idb_special_net : idb_special_net_list->get_net_list()) {
    for (idb::IdbSpecialWire* idb_special_wire : idb_special_net->get_wire_list()->get_wire_list()) {
      for (idb::IdbSpecialWireSegment* idb_special_segment : idb_special_wire->get_segment_list()) {
        if (idb_special_segment->is_via()) {
          continue;
        }

        idb::IdbLayer* idb_layer = idb_special_segment->get_layer();
        idb::IdbRect* idb_shape = idb_special_segment->get_bounding_box();
        idb::IdbCoordinate<int32_t>* idb_start_point = idb_special_segment->get_point_start();
        idb::IdbCoordinate<int32_t>* idb_end_point = idb_special_segment->get_point_second();
        if (idb_layer == nullptr || idb_shape == nullptr || idb_start_point == nullptr || idb_end_point == nullptr) {
          continue;
        }

        Segment segment;
        segment.set_layer_id(RCXDM.getDatabase().get_layer_table().get_design_id(idb_layer->get_name()));
        segment.set_start_point(GtlPointI(idb_start_point->get_x(), idb_start_point->get_y()));
        segment.set_end_point(GtlPointI(idb_end_point->get_x(), idb_end_point->get_y()));
        segment.set_shape(GtlRectI(idb_shape->get_low_x(), idb_shape->get_low_y(), idb_shape->get_high_x(), idb_shape->get_high_y()));
        special_net.get_segment_list().push_back(std::move(segment));
      }
    }
  }
}

#endif

#endif

#if 1  // output

void RCXInterface::output()
{
}

#endif

#endif

#endif

// private

RCXInterface* RCXInterface::_rcx_interface_instance = nullptr;

}  // namespace ircx
