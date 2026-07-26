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
  buildIOPin();
  buildGlobalConnect();
  buildGrid();
  buildStripe();
  buildLayerConnect(pg_model);
}

void PDNGenerator::buildIOPin()
{
  Database& database = FPDM.getDatabase();
  for (PGIOPin& pg_io_pin : FPDM.getConfig().pg_io_pin_list) {
    PGNet pg_net;
    pg_net.set_name(pg_io_pin.get_net_name());
    pg_net.set_type(pg_io_pin.get_net_type());
    pg_net.add_io_pin(pg_io_pin.get_pin_name(), pg_io_pin.get_direction());
    int32_t pg_net_idx = static_cast<int32_t>(database.get_pg_net_list().size());
    database.get_pg_net_list().push_back(pg_net);
    database.get_pg_net_name_to_idx_map()[pg_net.get_name()] = pg_net_idx;
  }
}

void PDNGenerator::buildGlobalConnect()
{
  for (PGGlobalConnect& pg_global_connect : FPDM.getConfig().pg_global_connect_list) {
    PGNet& pg_net = getPGNet(pg_global_connect.get_net_name());
    pg_net.add_instance_pin(pg_global_connect.get_instance_pin_name());
  }
}

PGNet& PDNGenerator::getPGNet(std::string net_name)
{
  Database& database = FPDM.getDatabase();
  return database.get_pg_net_list()[database.get_pg_net_name_to_idx_map()[net_name]];
}

void PDNGenerator::buildGrid()
{
  Database& database = FPDM.getDatabase();
  Core& core = database.get_core();
  for (PGGrid& pg_grid : FPDM.getConfig().pg_grid_list) {
    RoutingLayer* routing_layer = findRoutingLayer(pg_grid.get_layer_name());
    if (routing_layer == nullptr || routing_layer->get_prefer_direction() != Direction::kHorizontal) {
      continue;
    }

    int32_t width = FPUTIL.transMicronToDBU(pg_grid.get_width_micron(), database.get_micron_dbu());
    if (width <= 0) {
      continue;
    }

    int32_t row_idx = 0;
    int32_t top_y = core.get_ll_y();
    for (Row& row : database.get_row_list()) {
      std::string net_name = row_idx % 2 == 0 ? pg_grid.get_power_net_name() : pg_grid.get_ground_net_name();
      addLineSegment(net_name, routing_layer->get_name(), PGSegmentType::kFollowPin, width, core.get_ll_x(), row.get_y(), core.get_ur_x(), row.get_y());
      top_y = std::max(top_y, row.get_y());
      row_idx++;
    }
    if (!database.get_row_list().empty()) {
      top_y += database.get_row_list()[0].get_height();
      std::string net_name = row_idx % 2 == 0 ? pg_grid.get_power_net_name() : pg_grid.get_ground_net_name();
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

void PDNGenerator::buildStripe()
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
        addLineSegment(pg_stripe.get_power_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, core.get_ll_x(), power_coord,
                       core.get_ur_x(), power_coord);
      } else {
        addLineSegment(pg_stripe.get_power_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, power_coord, core.get_ll_y(), power_coord,
                       core.get_ur_y());
      }

      int32_t ground_coord = power_coord + half_pitch;
      if (ground_coord + width / 2 > line_end) {
        continue;
      }
      if (routing_layer->get_prefer_direction() == Direction::kHorizontal) {
        addLineSegment(pg_stripe.get_ground_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, core.get_ll_x(), ground_coord,
                       core.get_ur_x(), ground_coord);
      } else {
        addLineSegment(pg_stripe.get_ground_net_name(), routing_layer->get_name(), PGSegmentType::kStripe, width, ground_coord, core.get_ll_y(), ground_coord,
                       core.get_ur_y());
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

// private

PDNGenerator* PDNGenerator::_pg_instance = nullptr;

}  // namespace ifp
