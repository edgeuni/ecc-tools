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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "PDNGenerator.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ifp {

// public

void PDNGenerator::initInst()
{
  if (_pg_instance == nullptr) {
    _pg_instance = new PDNGenerator();
  }
}

PDNGenerator& PDNGenerator::getInst()
{
  if (_pg_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pg_instance;
}

void PDNGenerator::destroyInst()
{
  if (_pg_instance != nullptr) {
    delete _pg_instance;
    _pg_instance = nullptr;
  }
}

// function

#if 1  // generate

void PDNGenerator::generate()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  PGModel pg_model;
  generatePDN(pg_model);

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PDNGenerator::generatePDN(PGModel& pg_model)
{
  buildIOPin();
  buildGlobalConnect();
  buildPDNPort();
  buildGrid();
  buildStripe();
  buildLayerConnect(pg_model);
  buildMacroConnect(pg_model);
  buildIOPinConnect();
  buildStripeConnect();
  buildSegmentStripe();
  buildSegmentVia(pg_model);
}

#endif

#if 1  // build net

void PDNGenerator::buildIOPin()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_io_list) {
    if (value_list.size() != 4) {
      continue;
    }
    PGNetType net_type = std::stoi(value_list[3]) != 0 ? PGNetType::kPower : PGNetType::kGround;
    PGNet& pg_net = getPGNet(value_list[1], net_type);
    pg_net.add_io_pin(value_list[0], value_list[2]);
    IOPin& io_pin = getIOPin(value_list[0]);
    io_pin.set_special_net(true);
  }
}

PGNet& PDNGenerator::getPGNet(std::string net_name, PGNetType net_type)
{
  Database& database = FPDM.getDatabase();
  for (PGNet& pg_net : database.get_pg_net_list()) {
    if (pg_net.get_name() == net_name) {
      if (pg_net.get_type() == PGNetType::kNone && net_type != PGNetType::kNone) {
        pg_net.set_type(net_type);
      }
      return pg_net;
    }
  }

  PGNet pg_net;
  pg_net.set_name(net_name);
  pg_net.set_type(net_type);
  database.get_pg_net_list().push_back(pg_net);
  int32_t pg_net_idx = static_cast<int32_t>(database.get_pg_net_list().size()) - 1;
  database.get_pg_net_name_to_idx_map()[net_name] = pg_net_idx;
  return database.get_pg_net_list()[pg_net_idx];
}

IOPin& PDNGenerator::getIOPin(std::string pin_name)
{
  Database& database = FPDM.getDatabase();
  for (IOPin& io_pin : database.get_io_pin_list()) {
    if (io_pin.get_name() == pin_name) {
      return io_pin;
    }
  }

  IOPin io_pin;
  io_pin.set_name(pin_name);
  database.get_io_pin_list().push_back(io_pin);
  int32_t io_pin_idx = static_cast<int32_t>(database.get_io_pin_list().size()) - 1;
  database.get_io_pin_name_to_idx_map()[pin_name] = io_pin_idx;
  return database.get_io_pin_list()[io_pin_idx];
}

void PDNGenerator::buildGlobalConnect()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_global_connect_list) {
    if (value_list.size() != 3) {
      continue;
    }
    PGNetType net_type = std::stoi(value_list[2]) != 0 ? PGNetType::kPower : PGNetType::kGround;
    PGNet& pg_net = getPGNet(value_list[0], net_type);
    pg_net.add_instance_pin(value_list[1]);
  }
}

#endif

#if 1  // build port

void PDNGenerator::buildPDNPort()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_port_list) {
    if (value_list.size() != 7) {
      continue;
    }
    IOPin& io_pin = getIOPin(value_list[0]);
    Instance* io_cell = findInstance(value_list[1]);
    if (io_cell == nullptr) {
      continue;
    }

    int32_t offset_x = std::stoi(value_list[2]);
    int32_t offset_y = std::stoi(value_list[3]);
    int32_t width = std::stoi(value_list[4]);
    int32_t height = std::stoi(value_list[5]);
    int32_t rect_ll_x = io_cell->get_bounding_rect().get_ll_x() + offset_x;
    int32_t rect_ll_y = io_cell->get_bounding_rect().get_ll_y() + offset_y;
    int32_t rect_ur_x = rect_ll_x + width;
    int32_t rect_ur_y = rect_ll_y + height;
    int32_t pin_x = (rect_ll_x + rect_ur_x) / 2;
    int32_t pin_y = (rect_ll_y + rect_ur_y) / 2;

    io_pin.set_coord(pin_x, pin_y);
    io_pin.set_orient_name("N");
    io_pin.set_direct_location(true);
    io_pin.set_offset(offset_x / 2, offset_y / 2);
    io_pin.set_offset_updated(true);
    io_pin.set_port_exist(true);
    io_pin.set_port_exist_updated(true);
    io_pin.set_special_net(true);
    io_pin.set_placed(true);
    io_pin.set_fixed(true);

    IOPort io_port;
    io_port.set_layer_name(value_list[6]);
    io_port.set_coord(pin_x, pin_y);
    io_port.set_rect(-width / 2, -height / 2, width - width / 2, height - height / 2);
    io_port.set_placed(true);
    io_pin.get_new_port_list().push_back(io_port);
    io_pin.set_updated(true);
  }
}

Instance* PDNGenerator::findInstance(std::string instance_name)
{
  for (Instance& instance : FPDM.getDatabase().get_instance_list()) {
    if (instance.get_name() == instance_name) {
      return &instance;
    }
  }
  return nullptr;
}

#endif

#if 1  // build grid

void PDNGenerator::buildGrid()
{
  Database& database = FPDM.getDatabase();
  Core& core = database.get_core();
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_grid_list) {
    if (value_list.size() != 5) {
      continue;
    }
    RoutingLayer* routing_layer = findRoutingLayer(value_list[2]);
    if (routing_layer == nullptr || !routing_layer->get_horizontal()) {
      continue;
    }

    getPGNet(value_list[0], PGNetType::kPower);
    getPGNet(value_list[1], PGNetType::kGround);
    int32_t width = transMicronToDBU(std::stod(value_list[3]));
    if (width <= 0) {
      continue;
    }

    int32_t row_idx = 0;
    int32_t top_y = core.get_ll_y();
    for (Row& row : database.get_row_list()) {
      std::string net_name = row_idx % 2 == 0 ? value_list[0] : value_list[1];
      addLineSegmentWithBlockage(net_name, routing_layer->get_name(), PGSegmentType::kFollowPin, width, core.get_ll_x(), row.get_y(),
                                  core.get_ur_x(), row.get_y());
      top_y = std::max(top_y, row.get_y());
      row_idx++;
    }
    if (!database.get_row_list().empty()) {
      top_y += database.get_row_list()[0].get_height();
      std::string net_name = row_idx % 2 == 0 ? value_list[0] : value_list[1];
      addLineSegmentWithBlockage(net_name, routing_layer->get_name(), PGSegmentType::kFollowPin, width, core.get_ll_x(), top_y,
                                  core.get_ur_x(), top_y);
    }
  }
}

RoutingLayer* PDNGenerator::findRoutingLayer(std::string layer_name)
{
  Database& database = FPDM.getDatabase();
  std::map<std::string, int32_t>::iterator routing_layer_iter = database.get_routing_layer_name_to_idx_map().find(layer_name);
  if (routing_layer_iter == database.get_routing_layer_name_to_idx_map().end()) {
    return nullptr;
  }
  return &database.get_routing_layer_list()[routing_layer_iter->second];
}

int32_t PDNGenerator::transMicronToDBU(double value)
{
  return static_cast<int32_t>(std::round(value * FPDM.getDatabase().get_micron_dbu()));
}

void PDNGenerator::addLineSegmentWithBlockage(std::string net_name, std::string layer_name, PGSegmentType segment_type, int32_t width,
                                               int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y)
{
  if (start_x != end_x && start_y != end_y) {
    addLineSegment(net_name, layer_name, segment_type, width, start_x, start_y, end_x, end_y);
    return;
  }

  std::vector<std::pair<int32_t, int32_t>> blockage_interval_list;
  int32_t segment_ll_x = std::min(start_x, end_x) - width / 2;
  int32_t segment_ll_y = std::min(start_y, end_y) - width / 2;
  int32_t segment_ur_x = std::max(start_x, end_x) + width / 2;
  int32_t segment_ur_y = std::max(start_y, end_y) + width / 2;
  for (Blockage& blockage : FPDM.getDatabase().get_routing_blockage_list()) {
    if (blockage.get_except_pg_net()
        || std::find(blockage.get_layer_name_list().begin(), blockage.get_layer_name_list().end(), layer_name)
               == blockage.get_layer_name_list().end()) {
      continue;
    }
    if (segment_ur_x <= blockage.get_ll_x() || blockage.get_ur_x() <= segment_ll_x || segment_ur_y <= blockage.get_ll_y()
        || blockage.get_ur_y() <= segment_ll_y) {
      continue;
    }

    int32_t interval_begin = -1;
    int32_t interval_end = -1;
    if (start_y == end_y) {
      interval_begin = std::max(std::min(start_x, end_x), blockage.get_ll_x());
      interval_end = std::min(std::max(start_x, end_x), blockage.get_ur_x());
    } else {
      interval_begin = std::max(std::min(start_y, end_y), blockage.get_ll_y());
      interval_end = std::min(std::max(start_y, end_y), blockage.get_ur_y());
    }
    if (interval_begin < interval_end) {
      blockage_interval_list.emplace_back(interval_begin, interval_end);
    }
  }

  if (blockage_interval_list.empty()) {
    addLineSegment(net_name, layer_name, segment_type, width, start_x, start_y, end_x, end_y);
    return;
  }

  std::sort(blockage_interval_list.begin(), blockage_interval_list.end(),
            [](const std::pair<int32_t, int32_t>& first, const std::pair<int32_t, int32_t>& second) { return first.first < second.first; });
  std::vector<std::pair<int32_t, int32_t>> merged_interval_list;
  for (std::pair<int32_t, int32_t>& interval : blockage_interval_list) {
    if (merged_interval_list.empty() || merged_interval_list.back().second < interval.first) {
      merged_interval_list.push_back(interval);
    } else {
      merged_interval_list.back().second = std::max(merged_interval_list.back().second, interval.second);
    }
  }

  int32_t line_begin = start_y == end_y ? std::min(start_x, end_x) : std::min(start_y, end_y);
  int32_t line_end = start_y == end_y ? std::max(start_x, end_x) : std::max(start_y, end_y);
  int32_t cursor = line_begin;
  for (std::pair<int32_t, int32_t>& interval : merged_interval_list) {
    if (cursor < interval.first) {
      if (start_y == end_y) {
        addLineSegment(net_name, layer_name, segment_type, width, cursor, start_y, interval.first, start_y);
      } else {
        addLineSegment(net_name, layer_name, segment_type, width, start_x, cursor, start_x, interval.first);
      }
    }
    cursor = std::max(cursor, interval.second);
  }
  if (cursor < line_end) {
    if (start_y == end_y) {
      addLineSegment(net_name, layer_name, segment_type, width, cursor, start_y, line_end, start_y);
    } else {
      addLineSegment(net_name, layer_name, segment_type, width, start_x, cursor, start_x, line_end);
    }
  }
}

void PDNGenerator::addLineSegment(std::string net_name, std::string layer_name, PGSegmentType segment_type, int32_t width, int32_t start_x,
                                  int32_t start_y, int32_t end_x, int32_t end_y)
{
  if (width <= 0 || (start_x == end_x && start_y == end_y)) {
    return;
  }
  PGSegment pg_segment;
  pg_segment.set_net_name(net_name);
  pg_segment.set_layer_name(layer_name);
  pg_segment.set_type(segment_type);
  pg_segment.set_width(width);
  pg_segment.set_start_coord(start_x, start_y);
  pg_segment.set_end_coord(end_x, end_y);
  pg_segment.set_generated(true);
  FPDM.getDatabase().get_pg_segment_list().push_back(pg_segment);
}

#endif

#if 1  // build stripe

void PDNGenerator::buildStripe()
{
  Core& core = FPDM.getDatabase().get_core();
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_stripe_list) {
    if (value_list.size() != 6) {
      continue;
    }
    RoutingLayer* routing_layer = findRoutingLayer(value_list[2]);
    if (routing_layer == nullptr) {
      continue;
    }

    getPGNet(value_list[0], PGNetType::kPower);
    getPGNet(value_list[1], PGNetType::kGround);
    int32_t width = transMicronToDBU(std::stod(value_list[3]));
    int32_t pitch = transMicronToDBU(std::stod(value_list[4]));
    int32_t offset = transMicronToDBU(std::stod(value_list[5]));
    if (width <= 0 || pitch <= 0) {
      continue;
    }

    int32_t line_begin = routing_layer->get_horizontal() ? core.get_ll_y() : core.get_ll_x();
    int32_t line_end = routing_layer->get_horizontal() ? core.get_ur_y() : core.get_ur_x();
    int32_t start = line_begin + offset + width / 2;
    int32_t half_pitch = pitch / 2;
    int32_t track_pitch = routing_layer->get_prefer_track_pitch();
    int32_t track_offset = routing_layer->get_prefer_track_offset();
    for (int32_t coord = start; coord <= line_end; coord += pitch) {
      int32_t power_coord = coord;
      if (width <= track_pitch && track_pitch > 0) {
        power_coord = (power_coord - track_offset) / track_pitch * track_pitch + track_offset;
      }
      if (routing_layer->get_horizontal()) {
        addLineSegmentWithBlockage(value_list[0], routing_layer->get_name(), PGSegmentType::kStripe, width, core.get_ll_x(), power_coord,
                                    core.get_ur_x(), power_coord);
      } else {
        addLineSegmentWithBlockage(value_list[0], routing_layer->get_name(), PGSegmentType::kStripe, width, power_coord, core.get_ll_y(),
                                    power_coord, core.get_ur_y());
      }

      int32_t ground_coord = power_coord + half_pitch;
      if (ground_coord + width / 2 > line_end) {
        continue;
      }
      if (routing_layer->get_horizontal()) {
        addLineSegmentWithBlockage(value_list[1], routing_layer->get_name(), PGSegmentType::kStripe, width, core.get_ll_x(), ground_coord,
                                    core.get_ur_x(), ground_coord);
      } else {
        addLineSegmentWithBlockage(value_list[1], routing_layer->get_name(), PGSegmentType::kStripe, width, ground_coord, core.get_ll_y(),
                                    ground_coord, core.get_ur_y());
      }
    }
  }
}

#endif

#if 1  // build via

void PDNGenerator::buildLayerConnect(PGModel& pg_model)
{
  Database& database = FPDM.getDatabase();
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_layer_list) {
    for (int32_t layer_idx = 0; layer_idx + 1 < static_cast<int32_t>(value_list.size()); layer_idx += 2) {
      RoutingLayer* first_layer = findRoutingLayer(value_list[layer_idx]);
      RoutingLayer* second_layer = findRoutingLayer(value_list[layer_idx + 1]);
      if (first_layer == nullptr || second_layer == nullptr || first_layer->get_horizontal() == second_layer->get_horizontal()) {
        continue;
      }
      RoutingLayer* bottom_layer = first_layer;
      RoutingLayer* top_layer = second_layer;
      if (top_layer->get_order() < bottom_layer->get_order()) {
        std::swap(bottom_layer, top_layer);
      }

      std::vector<PGSegment> pg_segment_list = database.get_pg_segment_list();
      for (PGNet& pg_net : database.get_pg_net_list()) {
        std::vector<PGSegment> bottom_segment_list;
        std::vector<PGSegment> top_segment_list;
        for (PGSegment& pg_segment : pg_segment_list) {
          if (!pg_segment.is_line() || pg_segment.get_net_name() != pg_net.get_name()) {
            continue;
          }
          if (pg_segment.get_layer_name() == bottom_layer->get_name()) {
            bottom_segment_list.push_back(pg_segment);
          } else if (pg_segment.get_layer_name() == top_layer->get_name()) {
            top_segment_list.push_back(pg_segment);
          }
        }
        for (PGSegment& bottom_segment : bottom_segment_list) {
          PlanarRect bottom_rect(bottom_segment.get_ll_x(), bottom_segment.get_ll_y(), bottom_segment.get_ur_x(), bottom_segment.get_ur_y());
          for (PGSegment& top_segment : top_segment_list) {
            PlanarRect top_rect(top_segment.get_ll_x(), top_segment.get_ll_y(), top_segment.get_ur_x(), top_segment.get_ur_y());
            PlanarRect overlap_rect = getOverlapRect(bottom_rect, top_rect);
            if (overlap_rect.get_width() <= 0 || overlap_rect.get_height() <= 0) {
              continue;
            }
            addViaSegment(pg_model, pg_net.get_name(), bottom_layer->get_name(), top_layer->get_name(), "",
                          (overlap_rect.get_ll_x() + overlap_rect.get_ur_x()) / 2, (overlap_rect.get_ll_y() + overlap_rect.get_ur_y()) / 2,
                          overlap_rect.get_width(), overlap_rect.get_height());
          }
        }
      }
    }
  }
}

PlanarRect PDNGenerator::getOverlapRect(PlanarRect first_rect, PlanarRect second_rect)
{
  PlanarRect overlap_rect;
  overlap_rect.set_rect(std::max(first_rect.get_ll_x(), second_rect.get_ll_x()), std::max(first_rect.get_ll_y(), second_rect.get_ll_y()),
                        std::min(first_rect.get_ur_x(), second_rect.get_ur_x()), std::min(first_rect.get_ur_y(), second_rect.get_ur_y()));
  return overlap_rect;
}

void PDNGenerator::addViaSegment(PGModel& pg_model, std::string net_name, std::string bottom_layer_name, std::string top_layer_name,
                                 std::string cut_layer_name, int32_t x, int32_t y, int32_t width, int32_t height)
{
  std::string via_key
      = FPUTIL.getString(net_name, "|", bottom_layer_name, "|", top_layer_name, "|", cut_layer_name, "|", x, "|", y, "|", width, "|", height);
  if (pg_model.get_via_key_set().contains(via_key)) {
    return;
  }
  pg_model.get_via_key_set().insert(via_key);

  PGSegment pg_segment;
  pg_segment.set_net_name(net_name);
  pg_segment.set_type(PGSegmentType::kVia);
  pg_segment.set_bottom_layer_name(bottom_layer_name);
  pg_segment.set_top_layer_name(top_layer_name);
  pg_segment.set_cut_layer_name(cut_layer_name);
  pg_segment.set_start_coord(x, y);
  pg_segment.set_via_width(std::max(width, 1));
  pg_segment.set_via_height(std::max(height, 1));
  pg_segment.set_generated(true);
  FPDM.getDatabase().get_pg_segment_list().push_back(pg_segment);
}

#endif

#if 1  // build macro connection

void PDNGenerator::buildMacroConnect(PGModel& pg_model)
{
  Database& database = FPDM.getDatabase();
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_macro_connect_list) {
    if (value_list.size() != 5) {
      continue;
    }
    std::vector<std::string> power_pin_name_list = getNameList(value_list[3]);
    std::vector<std::string> ground_pin_name_list = getNameList(value_list[4]);
    std::string power_net_name = getPGNetName(PGNetType::kPower, power_pin_name_list);
    std::string ground_net_name = getPGNetName(PGNetType::kGround, ground_pin_name_list);
    if (power_net_name.empty() || ground_net_name.empty()) {
      continue;
    }
    getPGNet(power_net_name, PGNetType::kPower);
    getPGNet(ground_net_name, PGNetType::kGround);

    std::vector<PGSegment> pg_segment_list = database.get_pg_segment_list();
    for (Instance& instance : database.get_instance_list()) {
      if (!instance.get_macro() || (!instance.get_fixed() && !instance.get_placed())) {
        continue;
      }
      if (!value_list[2].empty() && value_list[2].find(instance.get_orient_name()) == std::string::npos) {
        continue;
      }
      for (InstancePinShape& pin_shape : instance.get_pin_shape_list()) {
        if (pin_shape.get_layer_name() != value_list[0]) {
          continue;
        }

        std::string net_name;
        if (std::find(power_pin_name_list.begin(), power_pin_name_list.end(), pin_shape.get_pin_name()) != power_pin_name_list.end()) {
          net_name = power_net_name;
        } else if (std::find(ground_pin_name_list.begin(), ground_pin_name_list.end(), pin_shape.get_pin_name())
                   != ground_pin_name_list.end()) {
          net_name = ground_net_name;
        } else {
          continue;
        }

        for (PGSegment& pg_segment : pg_segment_list) {
          if (!pg_segment.is_line() || pg_segment.get_net_name() != net_name || pg_segment.get_layer_name() != value_list[1]) {
            continue;
          }
          PlanarRect pg_rect(pg_segment.get_ll_x(), pg_segment.get_ll_y(), pg_segment.get_ur_x(), pg_segment.get_ur_y());
          PlanarRect overlap_rect = getOverlapRect(pin_shape, pg_rect);
          if (overlap_rect.get_width() <= 0 || overlap_rect.get_height() <= 0) {
            continue;
          }
          addViaSegment(pg_model, net_name, value_list[0], value_list[1], "", (overlap_rect.get_ll_x() + overlap_rect.get_ur_x()) / 2,
                        (overlap_rect.get_ll_y() + overlap_rect.get_ur_y()) / 2, overlap_rect.get_width(), overlap_rect.get_height());
        }
      }
    }
  }
}

std::vector<std::string> PDNGenerator::getNameList(std::string name_list)
{
  std::replace(name_list.begin(), name_list.end(), ',', ' ');
  return FPUTIL.splitString(name_list, ' ');
}

std::string PDNGenerator::getPGNetName(PGNetType net_type, std::vector<std::string>& pin_name_list)
{
  for (PGNet& pg_net : FPDM.getDatabase().get_pg_net_list()) {
    if (pg_net.get_type() == net_type) {
      return pg_net.get_name();
    }
  }
  if (!pin_name_list.empty()) {
    return pin_name_list[0];
  }
  return "";
}

#endif

#if 1  // build IO connection

void PDNGenerator::buildIOPinConnect()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_io_pin_connect_list) {
    if (value_list.size() < 5) {
      continue;
    }
    std::vector<std::pair<int32_t, int32_t>> coord_list = getCoordList(value_list, 1);
    if (coord_list.size() < 2) {
      continue;
    }
    std::string pin_name = getIOPinNameByCoord(coord_list.front().first, coord_list.front().second, value_list[0]);
    if (pin_name.empty()) {
      pin_name = getIOPinNameByCoord(coord_list.back().first, coord_list.back().second, value_list[0]);
    }
    std::string net_name = getPGNetNameByIOPin(pin_name);
    RoutingLayer* routing_layer = findRoutingLayer(value_list[0]);
    if (net_name.empty() || routing_layer == nullptr) {
      continue;
    }

    if (coord_list.size() == 2 && coord_list[0].first != coord_list[1].first && coord_list[0].second != coord_list[1].second) {
      IOPin& io_pin = getIOPin(pin_name);
      Core& core = FPDM.getDatabase().get_core();
      if (io_pin.get_x() <= core.get_ll_x() || io_pin.get_x() >= core.get_ur_x()) {
        int32_t middle_x = (coord_list[0].first + coord_list[1].first) / 2;
        coord_list.insert(coord_list.begin() + 1, std::make_pair(middle_x, coord_list[0].second));
        coord_list.insert(coord_list.begin() + 2, std::make_pair(middle_x, coord_list[1].second));
      } else {
        int32_t middle_y = (coord_list[0].second + coord_list[1].second) / 2;
        coord_list.insert(coord_list.begin() + 1, std::make_pair(coord_list[0].first, middle_y));
        coord_list.insert(coord_list.begin() + 2, std::make_pair(coord_list[1].first, middle_y));
      }
    }
    int32_t width = routing_layer->get_spacing() > 0 ? routing_layer->get_spacing() : routing_layer->get_prefer_track_pitch();
    addPolyline(coord_list, net_name, routing_layer->get_name(), width);
  }
}

std::vector<std::pair<int32_t, int32_t>> PDNGenerator::getCoordList(std::vector<std::string>& value_list, int32_t begin_idx)
{
  std::vector<std::pair<int32_t, int32_t>> coord_list;
  for (int32_t value_idx = begin_idx; value_idx + 1 < static_cast<int32_t>(value_list.size()); value_idx += 2) {
    coord_list.emplace_back(transMicronToDBU(std::stod(value_list[value_idx])), transMicronToDBU(std::stod(value_list[value_idx + 1])));
  }
  return coord_list;
}

std::string PDNGenerator::getIOPinNameByCoord(int32_t x, int32_t y, std::string layer_name)
{
  for (IOPin& io_pin : FPDM.getDatabase().get_io_pin_list()) {
    for (IOPort& io_port : io_pin.get_port_list()) {
      if (io_port.get_layer_name() == layer_name && io_port.get_ll_x() <= x && x <= io_port.get_ur_x() && io_port.get_ll_y() <= y
          && y <= io_port.get_ur_y()) {
        return io_pin.get_name();
      }
    }
    for (IOPort& io_port : io_pin.get_new_port_list()) {
      int32_t ll_x = io_port.get_x() + io_port.get_ll_x();
      int32_t ll_y = io_port.get_y() + io_port.get_ll_y();
      int32_t ur_x = io_port.get_x() + io_port.get_ur_x();
      int32_t ur_y = io_port.get_y() + io_port.get_ur_y();
      if (io_port.get_layer_name() == layer_name && ll_x <= x && x <= ur_x && ll_y <= y && y <= ur_y) {
        return io_pin.get_name();
      }
    }
    if (io_pin.get_x() == x && io_pin.get_y() == y) {
      return io_pin.get_name();
    }
  }
  return "";
}

std::string PDNGenerator::getPGNetNameByIOPin(std::string pin_name)
{
  for (PGNet& pg_net : FPDM.getDatabase().get_pg_net_list()) {
    if (pg_net.get_io_pin_name_to_direction_map().contains(pin_name)) {
      return pg_net.get_name();
    }
  }
  return "";
}

void PDNGenerator::addPolyline(std::vector<std::pair<int32_t, int32_t>>& coord_list, std::string net_name, std::string layer_name,
                               int32_t width)
{
  for (int32_t coord_idx = 0; coord_idx + 1 < static_cast<int32_t>(coord_list.size()); coord_idx++) {
    addLineSegmentWithBlockage(net_name, layer_name, PGSegmentType::kStripe, width, coord_list[coord_idx].first, coord_list[coord_idx].second,
                                coord_list[coord_idx + 1].first, coord_list[coord_idx + 1].second);
  }
}

#endif

#if 1  // build segment

void PDNGenerator::buildStripeConnect()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_stripe_connect_list) {
    if (value_list.size() < 7) {
      continue;
    }
    std::vector<std::pair<int32_t, int32_t>> coord_list = getCoordList(value_list, 3);
    addPolyline(coord_list, value_list[0], value_list[1], std::stoi(value_list[2]));
  }
}

void PDNGenerator::buildSegmentStripe()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_segment_stripe_list) {
    if (value_list.size() < 7) {
      continue;
    }
    std::vector<std::pair<int32_t, int32_t>> coord_list = getCoordList(value_list, 3);
    addPolyline(coord_list, value_list[0], value_list[1], std::stoi(value_list[2]));
  }
}

void PDNGenerator::buildSegmentVia(PGModel& pg_model)
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().pdn_segment_via_list) {
    if (value_list.size() == 6) {
      getPGNet(value_list[0], PGNetType::kNone);
      addViaSegment(pg_model, value_list[0], "", "", value_list[1], std::stoi(value_list[2]), std::stoi(value_list[3]),
                    std::stoi(value_list[4]), std::stoi(value_list[5]));
    } else if (value_list.size() == 7) {
      getPGNet(value_list[0], PGNetType::kNone);
      std::string top_layer_name = value_list[1];
      std::string bottom_layer_name = value_list[2];
      RoutingLayer* top_layer = findRoutingLayer(top_layer_name);
      RoutingLayer* bottom_layer = findRoutingLayer(bottom_layer_name);
      if (top_layer != nullptr && bottom_layer != nullptr && top_layer->get_order() < bottom_layer->get_order()) {
        std::swap(top_layer_name, bottom_layer_name);
      }
      addViaSegment(pg_model, value_list[0], bottom_layer_name, top_layer_name, "", std::stoi(value_list[3]), std::stoi(value_list[4]),
                    std::stoi(value_list[5]), std::stoi(value_list[6]));
    }
  }
}

#endif

// private

PDNGenerator* PDNGenerator::_pg_instance = nullptr;

}  // namespace ifp
