// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of the Mulan PSL v2 at:
//
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "IOPlacer.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ifp {

// public

void IOPlacer::initInst()
{
  if (_iop_instance == nullptr) {
    _iop_instance = new IOPlacer();
  }
}

IOPlacer& IOPlacer::getInst()
{
  if (_iop_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_iop_instance;
}

void IOPlacer::destroyInst()
{
  if (_iop_instance != nullptr) {
    delete _iop_instance;
    _iop_instance = nullptr;
  }
}

// function

void IOPlacer::place()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  IOPModel iop_model;
  placeIOPin();
  placeIOPortList();
  placeIOPad(iop_model);
  placeIOFiller(iop_model);

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

#if 1  // place IO pin

void IOPlacer::placeIOPin()
{
  Config& config = FPDM.getConfig();
  if (!config.io_pin_layer.empty() && config.io_pin_width > 0 && config.io_pin_height > 0) {
    autoPlacePins(config.io_pin_layer, config.io_pin_width, config.io_pin_height, config.io_pin_side_list);
  }
}

void IOPlacer::autoPlacePins(std::string layer_name, int32_t width, int32_t height, std::vector<std::string> side_list)
{
  Database& database = FPDM.getDatabase();
  auto routing_layer_iter = database.get_routing_layer_name_to_idx_map().find(layer_name);
  if (routing_layer_iter == database.get_routing_layer_name_to_idx_map().end()) {
    return;
  }

  RoutingLayer& current_routing_layer = database.get_routing_layer_list()[routing_layer_iter->second];
  std::string horizontal_layer_name;
  std::string vertical_layer_name;
  if (current_routing_layer.get_horizontal()) {
    horizontal_layer_name = current_routing_layer.get_name();
  } else {
    vertical_layer_name = current_routing_layer.get_name();
  }

  std::string neighbor_layer_name = getRoutingLayerNameByIdx(current_routing_layer.get_layer_idx() + 1);
  if (neighbor_layer_name.empty()) {
    neighbor_layer_name = getRoutingLayerNameByIdx(current_routing_layer.get_layer_idx() - 1);
  }
  if (!neighbor_layer_name.empty()) {
    RoutingLayer& neighbor_routing_layer
        = database.get_routing_layer_list()[database.get_routing_layer_name_to_idx_map()[neighbor_layer_name]];
    if (neighbor_routing_layer.get_horizontal()) {
      horizontal_layer_name = neighbor_routing_layer.get_name();
    } else {
      vertical_layer_name = neighbor_routing_layer.get_name();
    }
  }

  std::vector<IOPin>& io_pin_list = database.get_io_pin_list();
  int32_t io_pin_num = static_cast<int32_t>(io_pin_list.size());
  int32_t side_num = side_list.empty() ? 4 : static_cast<int32_t>(side_list.size());
  int32_t edge_pin_num = io_pin_num % side_num == 0 ? io_pin_num / side_num : io_pin_num / side_num + 1;
  int32_t io_pin_idx = 0;

  if (hasSide(side_list, "left")) {
    placeIOPinsOnEdge(IOEdgeType::kLeft, io_pin_list, io_pin_idx, edge_pin_num, horizontal_layer_name, vertical_layer_name, width, height,
                      getTrackPitch(horizontal_layer_name), getTrackPitch(vertical_layer_name), database.get_manufacture_grid());
  }
  if (hasSide(side_list, "right")) {
    placeIOPinsOnEdge(IOEdgeType::kRight, io_pin_list, io_pin_idx, edge_pin_num, horizontal_layer_name, vertical_layer_name, width, height,
                      getTrackPitch(horizontal_layer_name), getTrackPitch(vertical_layer_name), database.get_manufacture_grid());
  }
  if (hasSide(side_list, "bottom")) {
    placeIOPinsOnEdge(IOEdgeType::kBottom, io_pin_list, io_pin_idx, edge_pin_num, horizontal_layer_name, vertical_layer_name, width, height,
                      getTrackPitch(horizontal_layer_name), getTrackPitch(vertical_layer_name), database.get_manufacture_grid());
  }
  if (hasSide(side_list, "top")) {
    placeIOPinsOnEdge(IOEdgeType::kTop, io_pin_list, io_pin_idx, edge_pin_num, horizontal_layer_name, vertical_layer_name, width, height,
                      getTrackPitch(horizontal_layer_name), getTrackPitch(vertical_layer_name), database.get_manufacture_grid());
  }
}

bool IOPlacer::hasSide(std::vector<std::string>& side_list, std::string side_name)
{
  for (std::string& side : side_list) {
    if (side == side_name) {
      return true;
    }
  }
  return side_list.empty();
}

std::string IOPlacer::getRoutingLayerNameByIdx(int32_t layer_idx)
{
  for (RoutingLayer& routing_layer : FPDM.getDatabase().get_routing_layer_list()) {
    if (routing_layer.get_layer_idx() == layer_idx) {
      return routing_layer.get_name();
    }
  }
  return "";
}

int32_t IOPlacer::getTrackPitch(std::string layer_name)
{
  Database& database = FPDM.getDatabase();
  auto routing_layer_iter = database.get_routing_layer_name_to_idx_map().find(layer_name);
  if (routing_layer_iter == database.get_routing_layer_name_to_idx_map().end()) {
    return 0;
  }
  RoutingLayer& routing_layer = database.get_routing_layer_list()[routing_layer_iter->second];
  return std::max(std::max(routing_layer.get_prefer_track_pitch(), routing_layer.get_nonprefer_track_pitch()),
                  std::max(routing_layer.get_pitch_x(), routing_layer.get_pitch_y()));
}

void IOPlacer::placeIOPinsOnEdge(IOEdgeType edge_type, std::vector<IOPin>& io_pin_list, int32_t& io_pin_idx, int32_t edge_pin_num,
                                 std::string horizontal_layer_name, std::string vertical_layer_name, int32_t width, int32_t height,
                                 int32_t horizontal_pitch, int32_t vertical_pitch, int32_t manufacture_grid)
{
  Die& die = FPDM.getDatabase().get_die();
  Core& core = FPDM.getDatabase().get_core();
  int32_t io_pin_num = static_cast<int32_t>(io_pin_list.size());
  int32_t side_pin_num = std::min(edge_pin_num, io_pin_num - io_pin_idx);

  for (int32_t side_pin_idx = 0; side_pin_idx < side_pin_num; side_pin_idx++) {
    int32_t x = -1;
    int32_t y = -1;
    int32_t rect_width = -1;
    int32_t rect_height = -1;
    std::string layer_name;
    switch (edge_type) {
      case IOEdgeType::kLeft:
        x = die.get_ll_x() + height / 2;
        y = getAlongCoord(core.get_ll_y(), core.get_ur_y(), die.get_ll_y(), die.get_ur_y(), width, horizontal_pitch, side_pin_num,
                          side_pin_idx, manufacture_grid);
        rect_width = height;
        rect_height = width;
        layer_name = horizontal_layer_name;
        break;
      case IOEdgeType::kRight:
        x = die.get_ur_x() - height / 2;
        y = getAlongCoord(core.get_ll_y(), core.get_ur_y(), die.get_ll_y(), die.get_ur_y(), width, horizontal_pitch, side_pin_num,
                          side_pin_idx, manufacture_grid);
        rect_width = height;
        rect_height = width;
        layer_name = horizontal_layer_name;
        break;
      case IOEdgeType::kBottom:
        x = getAlongCoord(core.get_ll_x(), core.get_ur_x(), die.get_ll_x(), die.get_ur_x(), width, vertical_pitch, side_pin_num,
                          side_pin_idx, manufacture_grid);
        y = die.get_ll_y() + height / 2;
        rect_width = width;
        rect_height = height;
        layer_name = vertical_layer_name;
        break;
      case IOEdgeType::kTop:
        x = getAlongCoord(core.get_ll_x(), core.get_ur_x(), die.get_ll_x(), die.get_ur_x(), width, vertical_pitch, side_pin_num,
                          side_pin_idx, manufacture_grid);
        y = die.get_ur_y() - height / 2;
        rect_width = width;
        rect_height = height;
        layer_name = vertical_layer_name;
        break;
      default:
        return;
    }
    addIOPinPort(io_pin_list[io_pin_idx++], x, y, rect_width, rect_height, manufacture_grid, layer_name);
  }
}

int32_t IOPlacer::getAlongCoord(int32_t range_low, int32_t range_high, int32_t die_low, int32_t die_high, int32_t pin_span,
                                 int32_t access_pitch, int32_t side_pin_num, int32_t pin_idx, int32_t manufacture_grid)
{
  int32_t legal_low = std::max(range_low, die_low + access_pitch);
  int32_t legal_high = std::min(range_high, die_high - access_pitch);
  int32_t start = alignUp(legal_low + pin_span / 2, manufacture_grid);
  int32_t end = alignDown(legal_high - pin_span / 2, manufacture_grid);

  if (start > end) {
    start = alignUp(range_low + pin_span / 2, manufacture_grid);
    end = alignDown(range_high - pin_span / 2, manufacture_grid);
  }
  if (start > end) {
    return alignNearest((range_low + range_high) / 2, manufacture_grid);
  }
  if (side_pin_num <= 1) {
    return alignNearest((start + end) / 2, manufacture_grid);
  }

  int64_t span = static_cast<int64_t>(end - start);
  int32_t coord
      = start + static_cast<int32_t>((span * pin_idx + (side_pin_num - 1) / 2) / static_cast<int64_t>(side_pin_num - 1));
  coord = alignNearest(coord, manufacture_grid);
  return std::max(start, std::min(end, coord));
}

int32_t IOPlacer::alignDown(int32_t value, int32_t manufacture_grid)
{
  if (manufacture_grid <= 0) {
    return value;
  }
  int32_t remainder = value % manufacture_grid;
  if (remainder == 0) {
    return value;
  }
  return value >= 0 ? value - remainder : value - remainder - manufacture_grid;
}

int32_t IOPlacer::alignUp(int32_t value, int32_t manufacture_grid)
{
  if (manufacture_grid <= 0) {
    return value;
  }
  int32_t remainder = value % manufacture_grid;
  if (remainder == 0) {
    return value;
  }
  return value >= 0 ? value + manufacture_grid - remainder : value - remainder;
}

int32_t IOPlacer::alignNearest(int32_t value, int32_t manufacture_grid)
{
  int32_t lower = alignDown(value, manufacture_grid);
  int32_t upper = alignUp(value, manufacture_grid);
  return value - lower <= upper - value ? lower : upper;
}

void IOPlacer::addIOPinPort(IOPin& io_pin, int32_t x, int32_t y, int32_t rect_width, int32_t rect_height, int32_t manufacture_grid,
                             std::string layer_name)
{
  io_pin.set_placed(true);
  io_pin.set_fixed(false);

  IOPort io_port;
  syncPinLocation(io_pin, io_port, x, y);

  int32_t shape_ll_x = x - rect_width / 2;
  int32_t shape_ll_y = y - rect_height / 2;
  shape_ll_x = (shape_ll_x / manufacture_grid) * manufacture_grid;
  shape_ll_y = (shape_ll_y / manufacture_grid) * manufacture_grid;
  int32_t shape_ur_x = shape_ll_x + rect_width;
  int32_t shape_ur_y = shape_ll_y + rect_height;
  io_port.set_layer_name(layer_name);
  io_port.set_rect(shape_ll_x - x, shape_ll_y - y, shape_ur_x - x, shape_ur_y - y);
  io_pin.get_new_port_list().push_back(io_port);
  io_pin.set_updated(true);
  updateNetIOPin(io_pin);
}

void IOPlacer::syncPinLocation(IOPin& io_pin, IOPort& io_port, int32_t x, int32_t y)
{
  if (io_pin.get_port_exist() || io_pin.get_special_net()) {
    io_port.set_placed(true);
    io_port.set_coord(x, y);
  } else {
    io_pin.set_direct_location(true);
  }
  io_pin.set_coord(x, y);
  io_pin.set_orient_name("N");
}

void IOPlacer::updateNetIOPin(IOPin& io_pin)
{
  for (Net& net : FPDM.getDatabase().get_net_list()) {
    for (NetPin& net_pin : net.get_net_pin_list()) {
      if (net_pin.get_io() && net_pin.get_pin_name() == io_pin.get_name()) {
        net_pin.set_coord(io_pin.get_x(), io_pin.get_y());
        net_pin.set_placed(io_pin.get_placed());
      }
    }
  }
}

#endif

#if 1  // place IO port

void IOPlacer::placeIOPortList()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().io_port_list) {
    if (value_list.size() == 6) {
      placePort(value_list[0], std::stoi(value_list[1]), std::stoi(value_list[2]), std::stoi(value_list[3]), std::stoi(value_list[4]),
                value_list[5]);
    }
  }
}

void IOPlacer::placePort(std::string pin_name, int32_t x_offset, int32_t y_offset, int32_t rect_width, int32_t rect_height,
                         std::string layer_name)
{
  Database& database = FPDM.getDatabase();
  if (database.get_routing_layer_name_to_idx_map().find(layer_name) == database.get_routing_layer_name_to_idx_map().end()) {
    FPLOG.error(Loc::current(), "Place port failed: cannot find layer ", layer_name, ".");
    return;
  }
  auto io_pin_iter = database.get_io_pin_name_to_idx_map().find(pin_name);
  if (io_pin_iter == database.get_io_pin_name_to_idx_map().end()) {
    FPLOG.error(Loc::current(), "Place port failed: cannot find IO pin ", pin_name, ".");
    return;
  }
  IOPin& io_pin = database.get_io_pin_list()[io_pin_iter->second];
  auto instance_iter = database.get_instance_name_to_idx_map().find(io_pin.get_instance_name());
  if (instance_iter == database.get_instance_name_to_idx_map().end()) {
    FPLOG.error(Loc::current(), "Place port failed: cannot find IO cell.");
    return;
  }

  Instance& io_cell = database.get_instance_list()[instance_iter->second];
  int32_t rect_ll_x = io_cell.get_bounding_rect().get_ll_x() + x_offset;
  int32_t rect_ll_y = io_cell.get_bounding_rect().get_ll_y() + y_offset;
  int32_t rect_ur_x = rect_ll_x + rect_width;
  int32_t rect_ur_y = rect_ll_y + rect_height;
  int32_t pin_x = (rect_ll_x + rect_ur_x) / 2;
  int32_t pin_y = (rect_ll_y + rect_ur_y) / 2;

  io_pin.set_coord(pin_x, pin_y);
  io_pin.set_orient_name("N");
  io_pin.set_direct_location(true);
  io_pin.set_offset(x_offset / 2, y_offset / 2);
  io_pin.set_offset_updated(true);
  io_pin.set_placed(true);
  io_pin.set_fixed(true);
  io_pin.set_port_exist(io_pin.get_port_exist() || !io_pin.get_new_port_list().empty());
  io_pin.set_port_exist_updated(true);

  IOPort io_port;
  io_port.set_layer_name(layer_name);
  io_port.set_coord(pin_x, pin_y);
  io_port.set_rect(-rect_width / 2, -rect_height / 2, rect_width - rect_width / 2, rect_height - rect_height / 2);
  io_pin.get_new_port_list().push_back(io_port);
  io_pin.set_updated(true);
  updateNetIOPin(io_pin);
}

#endif

#if 1  // place IO pad

void IOPlacer::placeIOPad(IOPModel& iop_model)
{
  Config& config = FPDM.getConfig();
  if (!config.io_pad_master_list.empty()) {
    autoPlacePad(iop_model, config.io_pad_master_list, config.io_corner_master_list);
  }
}

void IOPlacer::autoPlacePad(IOPModel& iop_model, std::vector<std::string> pad_master_list,
                            std::vector<std::string> corner_master_list)
{
  Database& database = FPDM.getDatabase();
  auto io_site_iter = database.get_site_map().find(database.get_core().get_io_site_name());
  if (io_site_iter == database.get_site_map().end()) {
    return;
  }
  std::vector<int32_t> pad_idx_list = getIOPadIdxList(pad_master_list);
  if (pad_idx_list.empty()) {
    return;
  }

  setPadCoordList(iop_model, corner_master_list);
  int32_t range_total_length = 0;
  for (IOPadCoord& pad_coord : iop_model.get_io_pad_coord_list()) {
    range_total_length += pad_coord.get_end_coord() - pad_coord.get_begin_coord();
  }
  int32_t pad_total_length = 0;
  for (int32_t pad_idx : pad_idx_list) {
    pad_total_length += database.get_instance_list()[pad_idx].get_width();
  }
  int32_t site_step
      = (range_total_length - pad_total_length) / (static_cast<int32_t>(pad_idx_list.size()) + 8) / io_site_iter->second.get_width()
        * io_site_iter->second.get_width();

  int32_t pad_idx = 0;
  for (IOPadCoord& pad_coord : iop_model.get_io_pad_coord_list()) {
    placePad(pad_idx_list, pad_idx, pad_coord, site_step);
  }
}

std::vector<int32_t> IOPlacer::getIOPadIdxList(std::vector<std::string> pad_master_list)
{
  Database& database = FPDM.getDatabase();
  std::vector<int32_t> pad_idx_list;
  if (pad_master_list.empty()) {
    for (int32_t instance_idx = 0; instance_idx < static_cast<int32_t>(database.get_instance_list().size()); instance_idx++) {
      Instance& instance = database.get_instance_list()[instance_idx];
      auto cell_master_iter = database.get_cell_master_map().find(instance.get_master_name());
      if (cell_master_iter != database.get_cell_master_map().end() && cell_master_iter->second.get_pad()
          && !cell_master_iter->second.get_spacer()) {
        pad_idx_list.push_back(instance_idx);
      }
    }
  } else {
    for (std::string& pad_master_name : pad_master_list) {
      for (int32_t instance_idx = 0; instance_idx < static_cast<int32_t>(database.get_instance_list().size()); instance_idx++) {
        if (database.get_instance_list()[instance_idx].get_master_name() == pad_master_name) {
          pad_idx_list.push_back(instance_idx);
        }
      }
    }
  }
  return pad_idx_list;
}

void IOPlacer::setPadCoordList(IOPModel& iop_model, std::vector<std::string> corner_master_list)
{
  Database& database = FPDM.getDatabase();
  std::vector<IOPadCoord>& pad_coord_list = iop_model.get_io_pad_coord_list();
  pad_coord_list.clear();
  pad_coord_list.resize(4);

  std::vector<int32_t> corner_idx_list;
  if (corner_master_list.empty()) {
    for (int32_t instance_idx = 0; instance_idx < static_cast<int32_t>(database.get_instance_list().size()); instance_idx++) {
      Instance& instance = database.get_instance_list()[instance_idx];
      auto cell_master_iter = database.get_cell_master_map().find(instance.get_master_name());
      if (cell_master_iter != database.get_cell_master_map().end() && cell_master_iter->second.get_corner()) {
        corner_idx_list.push_back(instance_idx);
      }
    }
  } else {
    for (std::string& corner_master_name : corner_master_list) {
      for (int32_t instance_idx = 0; instance_idx < static_cast<int32_t>(database.get_instance_list().size()); instance_idx++) {
        if (database.get_instance_list()[instance_idx].get_master_name() == corner_master_name) {
          corner_idx_list.push_back(instance_idx);
        }
      }
    }
  }

  if (corner_idx_list.empty()) {
    Die& die = database.get_die();
    Site& io_site = database.get_site_map()[database.get_core().get_io_site_name()];
    int32_t site_height = io_site.get_height();

    pad_coord_list[0].set_edge_type(IOEdgeType::kBottom);
    pad_coord_list[0].set_orient_name(getOrientByEdge(IOEdgeType::kBottom));
    pad_coord_list[0].set_begin_coord(site_height);
    pad_coord_list[0].set_end_coord(die.get_width() - site_height);
    pad_coord_list[0].set_coord(0);

    pad_coord_list[1].set_edge_type(IOEdgeType::kLeft);
    pad_coord_list[1].set_orient_name(getOrientByEdge(IOEdgeType::kLeft));
    pad_coord_list[1].set_begin_coord(site_height);
    pad_coord_list[1].set_end_coord(die.get_height() - site_height);
    pad_coord_list[1].set_coord(0);

    pad_coord_list[2].set_edge_type(IOEdgeType::kTop);
    pad_coord_list[2].set_orient_name(getOrientByEdge(IOEdgeType::kTop));
    pad_coord_list[2].set_begin_coord(site_height);
    pad_coord_list[2].set_end_coord(die.get_width() - site_height);
    pad_coord_list[2].set_coord(die.get_height() - site_height);

    pad_coord_list[3].set_edge_type(IOEdgeType::kRight);
    pad_coord_list[3].set_orient_name(getOrientByEdge(IOEdgeType::kRight));
    pad_coord_list[3].set_begin_coord(site_height);
    pad_coord_list[3].set_end_coord(die.get_height() - site_height);
    pad_coord_list[3].set_coord(die.get_width() - site_height);
    return;
  }

  std::set<int32_t> coord_x_set;
  std::set<int32_t> coord_y_set;
  for (int32_t corner_idx : corner_idx_list) {
    PlanarRect& bounding_rect = database.get_instance_list()[corner_idx].get_bounding_rect();
    coord_x_set.insert(bounding_rect.get_ll_x());
    coord_x_set.insert(bounding_rect.get_ur_x());
    coord_y_set.insert(bounding_rect.get_ll_y());
    coord_y_set.insert(bounding_rect.get_ur_y());
  }
  std::vector<int32_t> coord_x_list;
  std::vector<int32_t> coord_y_list;
  std::set<int32_t>::iterator coord_x_iter = coord_x_set.begin();
  std::set<int32_t>::iterator coord_y_iter = coord_y_set.begin();
  for (int32_t coord_idx = 0; coord_idx < 4; coord_idx++) {
    coord_x_list.push_back(*coord_x_iter++);
    coord_y_list.push_back(*coord_y_iter++);
  }

  pad_coord_list[0].set_edge_type(IOEdgeType::kBottom);
  pad_coord_list[0].set_orient_name(getOrientByEdge(IOEdgeType::kBottom));
  pad_coord_list[0].set_begin_coord(coord_x_list[1]);
  pad_coord_list[0].set_end_coord(coord_x_list[2]);
  pad_coord_list[0].set_coord(coord_y_list[0]);

  pad_coord_list[1].set_edge_type(IOEdgeType::kLeft);
  pad_coord_list[1].set_orient_name(getOrientByEdge(IOEdgeType::kLeft));
  pad_coord_list[1].set_begin_coord(coord_y_list[1]);
  pad_coord_list[1].set_end_coord(coord_y_list[2]);
  pad_coord_list[1].set_coord(coord_x_list[0]);

  pad_coord_list[2].set_edge_type(IOEdgeType::kTop);
  pad_coord_list[2].set_orient_name(getOrientByEdge(IOEdgeType::kTop));
  pad_coord_list[2].set_begin_coord(coord_x_list[1]);
  pad_coord_list[2].set_end_coord(coord_x_list[2]);
  pad_coord_list[2].set_coord(coord_y_list[2]);

  pad_coord_list[3].set_edge_type(IOEdgeType::kRight);
  pad_coord_list[3].set_orient_name(getOrientByEdge(IOEdgeType::kRight));
  pad_coord_list[3].set_begin_coord(coord_y_list[1]);
  pad_coord_list[3].set_end_coord(coord_y_list[2]);
  pad_coord_list[3].set_coord(coord_x_list[2]);
}

std::string IOPlacer::getOrientByEdge(IOEdgeType edge_type)
{
  switch (edge_type) {
    case IOEdgeType::kBottom:
      return "N";
    case IOEdgeType::kLeft:
      return "E";
    case IOEdgeType::kTop:
      return "S";
    case IOEdgeType::kRight:
      return "W";
    default:
      return "";
  }
}

void IOPlacer::placePad(std::vector<int32_t>& pad_idx_list, int32_t& pad_idx, IOPadCoord& pad_coord, int32_t step)
{
  Database& database = FPDM.getDatabase();
  int32_t coord_offset = pad_coord.get_begin_coord() + step;
  for (; pad_idx < static_cast<int32_t>(pad_idx_list.size()) && coord_offset < pad_coord.get_end_coord(); pad_idx++) {
    Instance& pad = database.get_instance_list()[pad_idx_list[pad_idx]];
    int32_t pad_width = pad.get_width();
    if (coord_offset + pad_width > pad_coord.get_end_coord()) {
      return;
    }

    int32_t x = pad_coord.get_edge_type() == IOEdgeType::kBottom || pad_coord.get_edge_type() == IOEdgeType::kTop ? coord_offset
                                                                                                                   : pad_coord.get_coord();
    int32_t y = pad_coord.get_edge_type() == IOEdgeType::kBottom || pad_coord.get_edge_type() == IOEdgeType::kTop ? pad_coord.get_coord()
                                                                                                                   : coord_offset;
    pad.set_coord(x, y);
    pad.set_orient_name(pad_coord.get_orient_name());
    pad.set_placed(true);
    pad.set_fixed(false);
    pad.set_placement_updated(true);
    if (pad_coord.get_orient_name() == "N" || pad_coord.get_orient_name() == "S") {
      pad.set_bounding_rect(x, y, x + pad.get_width(), y + pad.get_height());
    } else {
      pad.set_bounding_rect(x, y, x + pad.get_height(), y + pad.get_width());
    }
    coord_offset += pad_width + step;
  }
}

#endif

#if 1  // place IO filler

void IOPlacer::placeIOFiller(IOPModel& iop_model)
{
  Config& config = FPDM.getConfig();
  if (!config.io_filler_name_list.empty()) {
    std::string prefix = config.io_filler_prefix.empty() ? "IOFill" : config.io_filler_prefix;
    autoPlaceFiller(iop_model, config.io_filler_name_list, prefix);
  }
}

void IOPlacer::autoPlaceFiller(IOPModel& iop_model, std::vector<std::string> filler_name_list, std::string prefix)
{
  Database& database = FPDM.getDatabase();
  std::vector<std::string> filler_master_name_list;
  for (std::string& filler_name : filler_name_list) {
    if (database.get_cell_master_map().find(filler_name) != database.get_cell_master_map().end()) {
      filler_master_name_list.push_back(filler_name);
    }
  }
  if (filler_master_name_list.empty()) {
    return;
  }
  std::sort(filler_master_name_list.begin(), filler_master_name_list.end(), [](const std::string& filler_name_a, const std::string& filler_name_b) {
    return FPDM.getDatabase().get_cell_master_map()[filler_name_a].get_width()
           > FPDM.getDatabase().get_cell_master_map()[filler_name_b].get_width();
  });

  setPadCoordList(iop_model, {});
  for (IOPadCoord& pad_coord : iop_model.get_io_pad_coord_list()) {
    placeFiller(iop_model, filler_master_name_list, prefix, pad_coord);
  }
}

void IOPlacer::placeFiller(IOPModel& iop_model, std::vector<std::string>& filler_name_list, std::string prefix, IOPadCoord& pad_coord)
{
  Database& database = FPDM.getDatabase();
  iop_model.set_io_filler_idx(-1);

  std::vector<IOInterval> used_interval_list;
  std::vector<IOInterval> need_filler_interval_list;
  for (Instance& io_instance : database.get_instance_list()) {
    auto cell_master_iter = database.get_cell_master_map().find(io_instance.get_master_name());
    if (cell_master_iter == database.get_cell_master_map().end()) {
      continue;
    }
    CellMaster& cell_master = cell_master_iter->second;
    if ((io_instance.get_placed() || io_instance.get_fixed() || io_instance.get_cover())
        && cell_master.get_pad() && !cell_master.get_spacer()
        && isSameEdgeAndOrient(pad_coord.get_edge_type(), io_instance.get_orient_name())) {
      int32_t width = cell_master.get_width();
      IOInterval used_interval;
      used_interval.set_edge_type(pad_coord.get_edge_type());
      if (pad_coord.get_edge_type() == IOEdgeType::kLeft || pad_coord.get_edge_type() == IOEdgeType::kRight) {
        used_interval.set_begin_coord(io_instance.get_bounding_rect().get_ll_y());
        used_interval.set_end_coord(io_instance.get_bounding_rect().get_ll_y() + width);
      } else {
        used_interval.set_begin_coord(io_instance.get_bounding_rect().get_ll_x());
        used_interval.set_end_coord(io_instance.get_bounding_rect().get_ll_x() + width);
      }
      used_interval_list.push_back(used_interval);
      if (cell_master.get_pad_filler()) {
        iop_model.set_io_filler_idx(iop_model.get_io_filler_idx() + 1);
      }
    }
  }

  if (used_interval_list.empty()) {
    need_filler_interval_list.emplace_back(pad_coord.get_edge_type(), pad_coord.get_begin_coord(), pad_coord.get_end_coord());
  } else {
    std::sort(used_interval_list.begin(), used_interval_list.end(),
              [](const IOInterval& interval_a, const IOInterval& interval_b) { return interval_a.get_begin_coord() < interval_b.get_begin_coord(); });

    int32_t start_idx = 0;
    int32_t end_idx = 0;
    for (int32_t used_idx = 0; used_idx < static_cast<int32_t>(used_interval_list.size()); used_idx++) {
      if (used_interval_list[used_idx].get_end_coord() > pad_coord.get_begin_coord()) {
        start_idx = used_idx;
        break;
      }
    }
    for (int32_t used_idx = static_cast<int32_t>(used_interval_list.size()) - 1; used_idx != -1; used_idx--) {
      if (used_interval_list[used_idx].get_begin_coord() < pad_coord.get_end_coord()) {
        end_idx = used_idx;
        break;
      }
    }

    int32_t need_start_coord = pad_coord.get_begin_coord();
    for (int32_t used_idx = start_idx; used_idx <= end_idx; used_idx++) {
      if (used_interval_list[used_idx].get_begin_coord() < pad_coord.get_begin_coord()) {
        need_start_coord = used_interval_list[used_idx].get_end_coord();
        continue;
      }
      if (need_start_coord == used_interval_list[used_idx].get_begin_coord()) {
        need_start_coord = used_interval_list[used_idx].get_end_coord();
        continue;
      }
      need_filler_interval_list.emplace_back(pad_coord.get_edge_type(), need_start_coord, used_interval_list[used_idx].get_begin_coord());
      need_start_coord = used_interval_list[used_idx].get_end_coord();
    }
    if (used_interval_list[end_idx].get_end_coord() < pad_coord.get_end_coord()) {
      need_filler_interval_list.emplace_back(pad_coord.get_edge_type(), used_interval_list[end_idx].get_end_coord(), pad_coord.get_end_coord());
    }
  }

  for (IOInterval& interval : need_filler_interval_list) {
    fillInterval(iop_model, interval, filler_name_list, prefix, pad_coord);
  }
}

bool IOPlacer::isSameEdgeAndOrient(IOEdgeType edge_type, std::string orient_name)
{
  return (edge_type == IOEdgeType::kBottom && orient_name == "N") || (edge_type == IOEdgeType::kRight && orient_name == "W")
         || (edge_type == IOEdgeType::kTop && orient_name == "S") || (edge_type == IOEdgeType::kLeft && orient_name == "E");
}

void IOPlacer::fillInterval(IOPModel& iop_model, IOInterval& interval, std::vector<std::string>& filler_name_list, std::string prefix,
                            IOPadCoord& pad_coord)
{
  Database& database = FPDM.getDatabase();
  int32_t begin_coord = interval.get_begin_coord();
  int32_t length = interval.get_length();
  while (length > 0) {
    std::string filler_name = getFiller(length, filler_name_list);
    if (filler_name.empty()) {
      FPLOG.error(Loc::current(), "IO filler placement failed at ", pad_coord.get_orient_name(), " edge coordinate ", begin_coord, ".");
      return;
    }

    CellMaster& filler = database.get_cell_master_map()[filler_name];
    std::string instance_name = buildFillerInstName(prefix, pad_coord, iop_model.get_io_filler_idx());
    int32_t x = pad_coord.get_orient_name() == "N" || pad_coord.get_orient_name() == "S" ? begin_coord : pad_coord.get_coord();
    int32_t y = pad_coord.get_orient_name() == "N" || pad_coord.get_orient_name() == "S" ? pad_coord.get_coord() : begin_coord;

    Instance instance;
    instance.set_name(instance_name);
    instance.set_master_name(filler_name);
    instance.set_orient_name(pad_coord.get_orient_name());
    instance.set_coord(x, y);
    instance.set_width(filler.get_width());
    instance.set_height(filler.get_height());
    instance.set_fixed(true);
    instance.set_placed(true);
    instance.set_new_instance(true);
    if (pad_coord.get_orient_name() == "N" || pad_coord.get_orient_name() == "S") {
      instance.set_bounding_rect(x, y, x + filler.get_width(), y + filler.get_height());
    } else {
      instance.set_bounding_rect(x, y, x + filler.get_height(), y + filler.get_width());
    }
    int32_t instance_idx = static_cast<int32_t>(database.get_instance_list().size());
    database.get_instance_list().push_back(instance);
    database.get_instance_name_to_idx_map()[instance_name] = instance_idx;

    iop_model.set_io_filler_idx(iop_model.get_io_filler_idx() + 1);
    begin_coord += filler.get_width();
    length -= filler.get_width();
  }
}

std::string IOPlacer::getFiller(int32_t length, std::vector<std::string>& filler_name_list)
{
  for (std::string& filler_name : filler_name_list) {
    if (FPDM.getDatabase().get_cell_master_map()[filler_name].get_width() <= length) {
      return filler_name;
    }
  }
  return "";
}

std::string IOPlacer::buildFillerInstName(std::string prefix, IOPadCoord& pad_coord, int32_t filler_idx)
{
  if (pad_coord.get_orient_name() == "N") {
    return prefix + "_S_" + std::to_string(filler_idx);
  }
  if (pad_coord.get_orient_name() == "S") {
    return prefix + "_N_" + std::to_string(filler_idx);
  }
  if (pad_coord.get_orient_name() == "W") {
    return prefix + "_E_" + std::to_string(filler_idx);
  }
  if (pad_coord.get_orient_name() == "E") {
    return prefix + "_W_" + std::to_string(filler_idx);
  }
  return prefix + "_" + std::to_string(filler_idx);
}

#endif

// private

IOPlacer* IOPlacer::_iop_instance = nullptr;

}  // namespace ifp
