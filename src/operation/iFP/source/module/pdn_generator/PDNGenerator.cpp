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
  buildPGNet(pg_model);
  buildRail(pg_model);
  buildStripe(pg_model);
  buildLayerConnect(pg_model);
  buildMacroConnect(pg_model);
}

void PDNGenerator::buildPGNet(PGModel& pg_model)
{
  Database& database = FPDM.getDatabase();
  for (PGGlobalConnect& pg_connect : FPDM.getConfig().pg_connect_list) {
    std::map<std::string, int32_t>::iterator pg_net_iter = database.get_pg_net_name_to_idx_map().find(pg_connect.get_net_name());
    if (pg_net_iter == database.get_pg_net_name_to_idx_map().end()) {
      PGNet pg_net;
      pg_net.set_name(pg_connect.get_net_name());
      pg_net.set_type(pg_connect.get_net_type());
      int32_t pg_net_idx = static_cast<int32_t>(database.get_pg_net_list().size());
      database.get_pg_net_list().push_back(pg_net);
      database.get_pg_net_name_to_idx_map()[pg_connect.get_net_name()] = pg_net_idx;
      if (pg_connect.get_net_type() == PGNetType::kPower && pg_model.get_default_power_net_name().empty()) {
        pg_model.set_default_power_net_name(pg_connect.get_net_name());
      } else if (pg_connect.get_net_type() == PGNetType::kGround && pg_model.get_default_ground_net_name().empty()) {
        pg_model.set_default_ground_net_name(pg_connect.get_net_name());
      }
    }

    PGNet& pg_net = getPGNet(pg_connect.get_net_name());
    if (database.get_io_pin_name_to_idx_map().find(pg_connect.get_pin_name()) != database.get_io_pin_name_to_idx_map().end()) {
      pg_net.add_io_pin(pg_connect.get_pin_name(), IOPinDirection::kInOut);
    } else {
      pg_net.add_instance_pin(pg_connect.get_pin_name());
    }
  }
}

PGNet& PDNGenerator::getPGNet(std::string net_name)
{
  Database& database = FPDM.getDatabase();
  return database.get_pg_net_list()[database.get_pg_net_name_to_idx_map()[net_name]];
}

void PDNGenerator::buildRail(PGModel& pg_model)
{
  Database& database = FPDM.getDatabase();
  Core& core = database.get_core();
  for (PGRail& pg_rail : FPDM.getConfig().pg_rail_list) {
    RoutingLayer* routing_layer = findRoutingLayer(pg_rail.get_layer_name());
    if (routing_layer == nullptr || routing_layer->get_prefer_direction() != Direction::kHorizontal) {
      continue;
    }

    int32_t width = FPUTIL.transMicronToDBU(pg_rail.get_width_micron(), database.get_micron_dbu());
    if (width <= 0) {
      continue;
    }

    int32_t row_idx = -1;
    int32_t previous_y = INT32_MIN;
    int32_t top_y = core.get_ll_y();
    for (Row& row : database.get_row_list()) {
      if (row.get_y() != previous_y) {
        row_idx++;
        previous_y = row.get_y();
      }
      std::string net_name = row_idx % 2 == 0 ? pg_model.get_default_power_net_name() : pg_model.get_default_ground_net_name();
      addLineSegment(net_name, routing_layer->get_name(), PGSegmentType::kFollowPin, width, row.get_ll_x(), row.get_y(), row.get_ur_x(),
                     row.get_y());
      top_y = std::max(top_y, row.get_y());
    }
    if (!database.get_row_list().empty()) {
      top_y += database.get_row_list()[0].get_height();
      std::string net_name = (row_idx + 1) % 2 == 0 ? pg_model.get_default_power_net_name() : pg_model.get_default_ground_net_name();
      addLineSegment(net_name, routing_layer->get_name(), PGSegmentType::kFollowPin, width, core.get_ll_x(), top_y, core.get_ur_x(), top_y);
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

void PDNGenerator::addLineSegment(std::string net_name, std::string layer_name, PGSegmentType segment_type, int32_t width, int32_t start_x,
                                  int32_t start_y, int32_t end_x, int32_t end_y)
{
  if (width <= 0 || (start_x == end_x && start_y == end_y)) {
    return;
  }

  std::vector<std::pair<int32_t, int32_t>> blockage_interval_list
      = getMacroBlockageIntervalList(layer_name, width, start_x, start_y, end_x, end_y);
  if (blockage_interval_list.empty()) {
    addUnblockedLineSegment(net_name, layer_name, segment_type, width, start_x, start_y, end_x, end_y);
    return;
  }

  bool horizontal = start_y == end_y;
  int32_t line_begin = horizontal ? std::min(start_x, end_x) : std::min(start_y, end_y);
  int32_t line_end = horizontal ? std::max(start_x, end_x) : std::max(start_y, end_y);
  int32_t current_coord = line_begin;
  for (std::pair<int32_t, int32_t>& blockage_interval : blockage_interval_list) {
    if (blockage_interval.second <= current_coord) {
      continue;
    }
    if (blockage_interval.first > current_coord) {
      if (horizontal) {
        addUnblockedLineSegment(net_name, layer_name, segment_type, width, current_coord, start_y, blockage_interval.first, start_y);
      } else {
        addUnblockedLineSegment(net_name, layer_name, segment_type, width, start_x, current_coord, start_x, blockage_interval.first);
      }
    }
    current_coord = std::max(current_coord, blockage_interval.second);
    if (current_coord >= line_end) {
      return;
    }
  }
  if (current_coord < line_end) {
    if (horizontal) {
      addUnblockedLineSegment(net_name, layer_name, segment_type, width, current_coord, start_y, line_end, start_y);
    } else {
      addUnblockedLineSegment(net_name, layer_name, segment_type, width, start_x, current_coord, start_x, line_end);
    }
  }
}

void PDNGenerator::addUnblockedLineSegment(std::string net_name, std::string layer_name, PGSegmentType segment_type, int32_t width,
                                           int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y)
{
  if (start_x == end_x && start_y == end_y) {
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

std::vector<std::pair<int32_t, int32_t>> PDNGenerator::getMacroBlockageIntervalList(std::string layer_name, int32_t width,
                                                                                      int32_t start_x, int32_t start_y, int32_t end_x,
                                                                                      int32_t end_y)
{
  RoutingLayer* routing_layer = findRoutingLayer(layer_name);
  std::vector<std::pair<int32_t, int32_t>> blockage_interval_list;
  bool horizontal = start_y == end_y;
  int32_t line_begin = horizontal ? std::min(start_x, end_x) : std::min(start_y, end_y);
  int32_t line_end = horizontal ? std::max(start_x, end_x) : std::max(start_y, end_y);
  int32_t line_coord = horizontal ? start_y : start_x;
  int32_t half_width = width / 2;
  for (Instance& instance : FPDM.getDatabase().get_instance_list()) {
    if (!instance.get_macro() || !instance.get_placed() || routing_layer->get_order() > getMacroTopLayerOrder(instance)) {
      continue;
    }
    PlanarRect& routing_halo_rect = instance.get_routing_halo_rect();
    if (horizontal) {
      if (line_coord + half_width <= routing_halo_rect.get_ll_y() || routing_halo_rect.get_ur_y() <= line_coord - half_width) {
        continue;
      }
      int32_t start_coord = std::max(line_begin, routing_halo_rect.get_ll_x() - half_width);
      int32_t end_coord = std::min(line_end, routing_halo_rect.get_ur_x() + half_width);
      if (start_coord < end_coord) {
        blockage_interval_list.emplace_back(start_coord, end_coord);
      }
    } else {
      if (line_coord + half_width <= routing_halo_rect.get_ll_x() || routing_halo_rect.get_ur_x() <= line_coord - half_width) {
        continue;
      }
      int32_t start_coord = std::max(line_begin, routing_halo_rect.get_ll_y() - half_width);
      int32_t end_coord = std::min(line_end, routing_halo_rect.get_ur_y() + half_width);
      if (start_coord < end_coord) {
        blockage_interval_list.emplace_back(start_coord, end_coord);
      }
    }
  }
  std::sort(blockage_interval_list.begin(), blockage_interval_list.end(),
            [](const std::pair<int32_t, int32_t>& first, const std::pair<int32_t, int32_t>& second) { return first.first < second.first; });
  return blockage_interval_list;
}

int32_t PDNGenerator::getMacroTopLayerOrder(Instance& instance)
{
  int32_t top_layer_order = -1;
  for (InstancePinShape& pin_shape : instance.get_pin_shape_list()) {
    RoutingLayer* routing_layer = findRoutingLayer(pin_shape.get_layer_name());
    if (routing_layer != nullptr) {
      top_layer_order = std::max(top_layer_order, routing_layer->get_order());
    }
  }
  return top_layer_order;
}

void PDNGenerator::buildStripe(PGModel& pg_model)
{
  Core& core = FPDM.getDatabase().get_core();
  for (PGStripe& pg_stripe : FPDM.getConfig().pg_stripe_list) {
    RoutingLayer* routing_layer = findRoutingLayer(pg_stripe.get_layer_name());
    if (routing_layer == nullptr) {
      continue;
    }

    int32_t width = FPUTIL.transMicronToDBU(pg_stripe.get_width_micron(), FPDM.getDatabase().get_micron_dbu());
    int32_t pitch = FPUTIL.transMicronToDBU(pg_stripe.get_pitch_micron(), FPDM.getDatabase().get_micron_dbu());
    int32_t offset = FPUTIL.transMicronToDBU(pg_stripe.get_offset_micron(), FPDM.getDatabase().get_micron_dbu());
    if (width <= 0 || pitch <= 0) {
      continue;
    }

    int32_t line_begin = routing_layer->get_prefer_direction() == Direction::kHorizontal ? core.get_ll_y() : core.get_ll_x();
    int32_t line_end = routing_layer->get_prefer_direction() == Direction::kHorizontal ? core.get_ur_y() : core.get_ur_x();
    int32_t start = line_begin + offset + width / 2;
    int32_t half_pitch = pitch / 2;
    int32_t track_pitch = routing_layer->get_prefer_track_pitch();
    int32_t track_offset = routing_layer->get_prefer_track_offset();
    for (int32_t coord = start; coord <= line_end; coord += pitch) {
      int32_t power_coord = coord;
      if (width <= track_pitch && track_pitch > 0) {
        power_coord = (power_coord - track_offset) / track_pitch * track_pitch + track_offset;
      }
      if (routing_layer->get_prefer_direction() == Direction::kHorizontal) {
        addLineSegment(pg_model.get_default_power_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, core.get_ll_x(),
                       power_coord, core.get_ur_x(), power_coord);
      } else {
        addLineSegment(pg_model.get_default_power_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, power_coord,
                       core.get_ll_y(), power_coord, core.get_ur_y());
      }

      int32_t ground_coord = power_coord + half_pitch;
      if (ground_coord + width / 2 > line_end) {
        continue;
      }
      if (routing_layer->get_prefer_direction() == Direction::kHorizontal) {
        addLineSegment(pg_model.get_default_ground_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, core.get_ll_x(),
                       ground_coord, core.get_ur_x(), ground_coord);
      } else {
        addLineSegment(pg_model.get_default_ground_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, ground_coord,
                       core.get_ll_y(), ground_coord, core.get_ur_y());
      }
    }
  }
}

void PDNGenerator::buildLayerConnect(PGModel& pg_model)
{
  Database& database = FPDM.getDatabase();
  for (PGLayerPair& pg_layer_pair : FPDM.getConfig().pg_layer_pair_list) {
    RoutingLayer* first_layer = findRoutingLayer(pg_layer_pair.get_first_layer_name());
    RoutingLayer* second_layer = findRoutingLayer(pg_layer_pair.get_second_layer_name());
    if (first_layer == nullptr || second_layer == nullptr || first_layer->get_prefer_direction() == second_layer->get_prefer_direction()) {
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
  std::string via_key = FPUTIL.getString(net_name, "|", bottom_layer_name, "|", top_layer_name, "|", cut_layer_name, "|", x, "|", y, "|",
                                         width, "|", height);
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

void PDNGenerator::buildMacroConnect(PGModel& pg_model)
{
  Database& database = FPDM.getDatabase();
  for (PGNet& pg_net : database.get_pg_net_list()) {
    for (Instance& instance : database.get_instance_list()) {
      if (!instance.get_macro() || !instance.get_placed()) {
        continue;
      }
      for (InstancePinShape& pin_shape : instance.get_pin_shape_list()) {
        if (std::find(pg_net.get_instance_pin_name_list().begin(), pg_net.get_instance_pin_name_list().end(), pin_shape.get_pin_name())
            == pg_net.get_instance_pin_name_list().end()) {
          continue;
        }
        connectMacroPin(pg_model, pg_net, pin_shape);
      }
    }
  }
}

void PDNGenerator::connectMacroPin(PGModel& pg_model, PGNet& pg_net, InstancePinShape& pin_shape)
{
  RoutingLayer* pin_layer = findRoutingLayer(pin_shape.get_layer_name());
  if (pin_layer == nullptr) {
    return;
  }

  Database& database = FPDM.getDatabase();
  PlanarRect pin_rect(pin_shape.get_ll_x(), pin_shape.get_ll_y(), pin_shape.get_ur_x(), pin_shape.get_ur_y());
  std::vector<PGSegment> pg_segment_list = database.get_pg_segment_list();
  int32_t connect_layer_order = INT32_MAX;
  for (PGSegment& pg_segment : pg_segment_list) {
    if (!pg_segment.is_line() || pg_segment.get_net_name() != pg_net.get_name()) {
      continue;
    }
    RoutingLayer* routing_layer = findRoutingLayer(pg_segment.get_layer_name());
    if (routing_layer == nullptr || routing_layer->get_order() <= pin_layer->get_order()) {
      continue;
    }
    PlanarRect segment_rect(pg_segment.get_ll_x(), pg_segment.get_ll_y(), pg_segment.get_ur_x(), pg_segment.get_ur_y());
    PlanarRect overlap_rect = getOverlapRect(pin_rect, segment_rect);
    if (overlap_rect.get_width() <= 0 || overlap_rect.get_height() <= 0) {
      continue;
    }
    connect_layer_order = std::min(connect_layer_order, routing_layer->get_order());
  }
  if (connect_layer_order == INT32_MAX) {
    return;
  }

  for (PGSegment& pg_segment : pg_segment_list) {
    if (!pg_segment.is_line() || pg_segment.get_net_name() != pg_net.get_name()) {
      continue;
    }
    RoutingLayer* routing_layer = findRoutingLayer(pg_segment.get_layer_name());
    if (routing_layer == nullptr || routing_layer->get_order() != connect_layer_order) {
      continue;
    }
    PlanarRect segment_rect(pg_segment.get_ll_x(), pg_segment.get_ll_y(), pg_segment.get_ur_x(), pg_segment.get_ur_y());
    PlanarRect overlap_rect = getOverlapRect(pin_rect, segment_rect);
    if (overlap_rect.get_width() <= 0 || overlap_rect.get_height() <= 0) {
      continue;
    }
    addViaSegment(pg_model, pg_net.get_name(), pin_layer->get_name(), routing_layer->get_name(), "",
                  (overlap_rect.get_ll_x() + overlap_rect.get_ur_x()) / 2, (overlap_rect.get_ll_y() + overlap_rect.get_ur_y()) / 2,
                  overlap_rect.get_width(), overlap_rect.get_height());
  }
}

// private

PDNGenerator* PDNGenerator::_pg_instance = nullptr;

}  // namespace ifp
