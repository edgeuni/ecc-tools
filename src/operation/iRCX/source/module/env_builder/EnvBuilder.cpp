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
/**
 * @file EnvBuilder.cpp
 * @brief iRCX env-builder module implementation.
 */
#include "EnvBuilder.hpp"

#include "EnvParallel.hpp"
#include "EnvTrackOverlapMerge.hpp"
#include "Utility.hpp"

namespace ircx {

// public

void EnvBuilder::initInst()
{
  if (_eb_instance == nullptr) {
    _eb_instance = new EnvBuilder();
  }
}

EnvBuilder& EnvBuilder::getInst()
{
  if (_eb_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_eb_instance;
}

void EnvBuilder::destroyInst()
{
  if (_eb_instance != nullptr) {
    delete _eb_instance;
    _eb_instance = nullptr;
  }
}

void EnvBuilder::build()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  EBModel eb_model;
  buildEBModel(eb_model);

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

EnvBuilder* EnvBuilder::_eb_instance = nullptr;

void EnvBuilder::buildEBModel(EBModel& eb_model)
{
  if (!buildNetEnvs(eb_model)) {
    RCXLOG.error(Loc::current(), "Build net env failed!");
  }
}

bool EnvBuilder::buildNetEnvs(EBModel& eb_model)
{
  Database& database = RCXDM.getDatabase();
  LayoutData& layout_data = database.get_layout_data();
  TopoPool& topo_pool = database.get_topo_pool();

  if (!buildTracks(eb_model) || !buildPixels(eb_model)) {
    return false;
  }
  buildSearchTrackNumMap(eb_model);

  size_t net_num = layout_data.get_regular_net_count();
  std::vector<NetEnv>& net_envs = RCXDM.getDatabase().get_net_env_list();
  net_envs.clear();
  net_envs.resize(net_num);

  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  std::map<size_t, EnvTrack>& layer_to_track_prefer_dir = eb_model.get_layer_to_track_prefer_dir();
  std::map<size_t, EnvTrack>& layer_to_track_nonprefer_dir = eb_model.get_layer_to_track_nonprefer_dir();
  std::map<size_t, int32_t>& layer_to_search_track_num = eb_model.get_layer_to_search_track_num();

  const int32_t net_threads = env_parallel::threadCount(net_num);
#pragma omp parallel for schedule(dynamic) num_threads(net_threads)
  for (size_t nid = 0; nid < net_num; nid++) {
    EnvTrackOverlapMerge track_merger;
    EnvPixelOverlapMerge pixel_merger;
    NetEnv& env = net_envs[nid];

    for (TopoEdge& edge : topo_pool.get_net_edge_list(nid)) {
      if (edge.get_is_via()) {
        env.append_edge_interval_list({});  // placeholder to keep index aligned with TopoPool
        continue;
      }

      const size_t lid = edge.get_layer_id();
      LineSegment& query_seg = edge.get_line_segment();

      std::vector<EnvTrackOverlap> track_ov_up;
      std::vector<EnvTrackOverlap> track_ov_dn;
      const bool layer_is_horz = routing_layers.at(lid).get_is_prefer_horizontal();
      std::map<size_t, EnvTrack>& track_map
          = edge.get_line_segment().get_is_horizontal() == layer_is_horz ? layer_to_track_prefer_dir : layer_to_track_nonprefer_dir;
      if (const auto track_it = track_map.find(lid); track_it != track_map.end()) {
        track_ov_up = track_it->second.overlap(query_seg, layer_to_search_track_num[lid]);
        track_ov_dn = track_it->second.overlap(query_seg, -layer_to_search_track_num[lid]);
      }

      std::vector<EdgeEnvInterval> out;
      track_merger.compute(query_seg.get_lower(), query_seg.get_upper(), track_ov_dn, track_ov_up, out);

      std::vector<EnvLayerPixelOverlaps> dn_inputs = collectCrossSide(eb_model, query_seg, lid, false);
      std::vector<EnvLayerPixelOverlaps> up_inputs = collectCrossSide(eb_model, query_seg, lid, true);

      std::vector<CrossOverlapSub> cross_full;
      pixel_merger.compute(query_seg.get_lower(), query_seg.get_upper(), dn_inputs, up_inputs, cross_full);

      for (EdgeEnvInterval& interval : out) {
        interval.set_cross_overlap_sub_list(clipCrossSegments(cross_full, interval.get_start_coordinate(), interval.get_end_coordinate()));
      }

      env.append_edge_interval_list(std::move(out));
    }
  }

  return true;
}

std::vector<CrossOverlapSub> EnvBuilder::clipCrossSegments(const std::vector<CrossOverlapSub>& cross_overlap_sub_list, int32_t a0,
                                                           int32_t a1)
{
  std::vector<CrossOverlapSub> clipped_cross_overlap_sub_list;
  for (const CrossOverlapSub& cross_overlap_sub : cross_overlap_sub_list) {
    int32_t clipped_a0 = std::max(a0, cross_overlap_sub.get_start_coordinate());
    int32_t clipped_a1 = std::min(a1, cross_overlap_sub.get_end_coordinate());
    if (!(clipped_a0 < clipped_a1)) {
      continue;
    }
    if (!clipped_cross_overlap_sub_list.empty() && clipped_cross_overlap_sub_list.back().get_end_coordinate() == clipped_a0
        && clipped_cross_overlap_sub_list.back().get_below_layer_id() == cross_overlap_sub.get_below_layer_id()
        && clipped_cross_overlap_sub_list.back().get_above_layer_id() == cross_overlap_sub.get_above_layer_id()) {
      clipped_cross_overlap_sub_list.back().set_end_coordinate(clipped_a1);
      continue;
    }
    CrossOverlapSub clipped_cross_overlap_sub;
    clipped_cross_overlap_sub.set_start_coordinate(clipped_a0);
    clipped_cross_overlap_sub.set_end_coordinate(clipped_a1);
    clipped_cross_overlap_sub.set_below_layer_id(cross_overlap_sub.get_below_layer_id());
    clipped_cross_overlap_sub.set_above_layer_id(cross_overlap_sub.get_above_layer_id());
    clipped_cross_overlap_sub_list.push_back(std::move(clipped_cross_overlap_sub));
  }
  return clipped_cross_overlap_sub_list;
}

std::vector<EnvLayerPixelOverlaps> EnvBuilder::collectCrossSide(EBModel& eb_model, const LineSegment& line_segment, size_t base_lid,
                                                                bool search_up)
{
  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  const size_t min_lid = routing_layers.empty() ? 0 : routing_layers.begin()->first;
  const size_t max_lid = routing_layers.empty() ? 0 : routing_layers.rbegin()->first;
  std::map<size_t, EnvPixel>& layer_to_pixel_prefer_dir = eb_model.get_layer_to_pixel_prefer_dir();
  std::map<size_t, EnvPixel>& layer_to_pixel_nonprefer_dir = eb_model.get_layer_to_pixel_nonprefer_dir();
  std::vector<EnvLayerPixelOverlaps> layer_pixel_overlap_list;

  for (size_t delta = 1; delta <= eb_model.get_cross_layer(); ++delta) {
    size_t candidate_layer_id = 0;
    if (search_up) {
      if (base_lid > max_lid || max_lid - base_lid < delta) {
        break;
      }
      candidate_layer_id = base_lid + delta;
    } else {
      if (base_lid < min_lid || base_lid - min_lid < delta) {
        break;
      }
      candidate_layer_id = base_lid - delta;
    }

    std::map<size_t, RoutingLayer>::iterator layer_iter = routing_layers.find(candidate_layer_id);
    if (layer_iter == routing_layers.end()) {
      continue;
    }

    std::map<size_t, EnvPixel>& pixel_map = (layer_iter->second.get_is_prefer_horizontal() != line_segment.get_is_horizontal())
                                                ? layer_to_pixel_prefer_dir
                                                : layer_to_pixel_nonprefer_dir;
    std::map<size_t, EnvPixel>::iterator pixel_iter = pixel_map.find(candidate_layer_id);
    if (pixel_iter == pixel_map.end()) {
      continue;
    }

    std::vector<EnvPixelOverlap> pixel_overlap_list = pixel_iter->second.overlap(line_segment);
    if (pixel_overlap_list.empty()) {
      continue;
    }

    EnvLayerPixelOverlaps layer_pixel_overlaps;
    layer_pixel_overlaps.set_layer_id(candidate_layer_id);
    layer_pixel_overlaps.set_pixel_overlap_list(std::move(pixel_overlap_list));
    layer_pixel_overlap_list.push_back(std::move(layer_pixel_overlaps));
  }
  return layer_pixel_overlap_list;
}

bool EnvBuilder::buildTracks(EBModel& eb_model)
{
  Database& database = RCXDM.getDatabase();
  LayoutData& layout_data = database.get_layout_data();
  TopoPool& topo_pool = database.get_topo_pool();

  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  GTLRectInt& rect = layout_data.get_die_shape();
  int32_t bucket_dlt = static_cast<int32_t>(eb_model.get_bucket_size_um() * layout_data.get_dbu_per_micron());
  std::map<size_t, EnvTrack>& layer_to_track_prefer_dir = eb_model.get_layer_to_track_prefer_dir();
  std::map<size_t, EnvTrack>& layer_to_track_nonprefer_dir = eb_model.get_layer_to_track_nonprefer_dir();

  layer_to_track_prefer_dir.clear();
  layer_to_track_nonprefer_dir.clear();

  for (auto& [lid, layer] : routing_layers) {
    TrackInfo& track_info = layer.get_track_info();

    EnvTrack prefer_track;
    if (!initTrackForDirection(prefer_track, track_info, rect, bucket_dlt, layer.get_is_prefer_horizontal())) {
      return false;
    }
    layer_to_track_prefer_dir[lid] = std::move(prefer_track);

    EnvTrack nonprefer_track;
    if (!initTrackForDirection(nonprefer_track, track_info, rect, bucket_dlt, !layer.get_is_prefer_horizontal())) {
      return false;
    }
    layer_to_track_nonprefer_dir[lid] = std::move(nonprefer_track);
  }

  for (TopoEdge& edge : topo_pool.get_edge_pool()) {
    addTrackEdge(eb_model, edge);
  }
  for (TopoEdge& edge : topo_pool.get_special_edge_pool()) {
    addTrackEdge(eb_model, edge);
  }

  return true;
}

void EnvBuilder::addTrackEdge(EBModel& eb_model, TopoEdge& edge)
{
  if (edge.get_is_via()) {
    return;
  }

  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  std::map<size_t, EnvTrack>& layer_to_track_prefer_dir = eb_model.get_layer_to_track_prefer_dir();
  std::map<size_t, EnvTrack>& layer_to_track_nonprefer_dir = eb_model.get_layer_to_track_nonprefer_dir();
  const size_t layer_id = edge.get_layer_id();
  const bool layer_is_horz = routing_layers.at(layer_id).get_is_prefer_horizontal();
  std::map<size_t, EnvTrack>& track_map
      = edge.get_line_segment().get_is_horizontal() == layer_is_horz ? layer_to_track_prefer_dir : layer_to_track_nonprefer_dir;
  track_map.at(layer_id).addEdge(edge);
}

bool EnvBuilder::initTrackForDirection(EnvTrack& track, TrackInfo& track_info, GTLRectInt& rect, int32_t bucket_dlt, bool is_horz)
{
  const int32_t die_x0 = RCXUTIL.minX(rect);
  const int32_t die_y0 = RCXUTIL.minY(rect);
  const int32_t die_x1 = RCXUTIL.maxX(rect);
  const int32_t die_y1 = RCXUTIL.maxY(rect);
  const int32_t die_dx = RCXUTIL.deltaX(rect);
  const int32_t die_dy = RCXUTIL.deltaY(rect);

  const int32_t track_ori = is_horz ? track_info.get_y_start() : track_info.get_x_start();
  const int32_t track_num = is_horz ? static_cast<int32_t>(track_info.get_y_count()) : static_cast<int32_t>(track_info.get_x_count());
  const int32_t track_dlt = is_horz ? track_info.get_y_step() : track_info.get_x_step();
  const int32_t axis_lo = is_horz ? die_y0 : die_x0;
  const int32_t axis_hi = is_horz ? die_y1 : die_x1;
  EnvAxis track_axis = coverAxis(track_ori, track_num, track_dlt, axis_lo, axis_hi);

  track.set_track_origin(track_axis.get_origin());
  track.set_track_count(track_axis.get_count());
  track.set_track_step(track_axis.get_step());
  track.set_bucket_origin(is_horz ? die_x0 : die_y0);
  track.set_bucket_count(ceilDivPositive(is_horz ? die_dx : die_dy, bucket_dlt));
  track.set_bucket_step(bucket_dlt);
  return track.initTrack();
}

EnvAxis EnvBuilder::coverAxis(int32_t origin, int32_t count, int32_t step, int32_t lo, int32_t hi)
{
  if (step <= 0) {
    return EnvAxis(origin, count, step);
  }

  int32_t axis_origin = origin;
  int32_t axis_count = count;
  const int32_t axis_step = step;

  if (axis_origin > lo) {
    const int32_t shift = (axis_origin - lo + axis_step - 1) / axis_step;
    axis_origin -= shift * axis_step;
    axis_count += shift;
  }

  const int32_t covered_hi = axis_origin + axis_step * axis_count;
  if (covered_hi <= hi) {
    axis_count += (hi - covered_hi) / axis_step + 1;
  }

  return EnvAxis(axis_origin, axis_count, step);
}

int32_t EnvBuilder::ceilDivPositive(int32_t value, int32_t divisor)
{
  if (value <= 0 || divisor <= 0) {
    return 0;
  }
  return (value + divisor - 1) / divisor;
}

bool EnvBuilder::buildPixels(EBModel& eb_model)
{
  Database& database = RCXDM.getDatabase();
  LayoutData& layout_data = database.get_layout_data();
  TopoPool& topo_pool = database.get_topo_pool();

  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  GTLRectInt& rect = layout_data.get_die_shape();
  int32_t die_x0 = RCXUTIL.minX(rect);
  int32_t die_y0 = RCXUTIL.minY(rect);
  int32_t die_x1 = RCXUTIL.maxX(rect);
  int32_t die_y1 = RCXUTIL.maxY(rect);
  std::map<size_t, EnvPixel>& layer_to_pixel_prefer_dir = eb_model.get_layer_to_pixel_prefer_dir();
  std::map<size_t, EnvPixel>& layer_to_pixel_nonprefer_dir = eb_model.get_layer_to_pixel_nonprefer_dir();

  layer_to_pixel_prefer_dir.clear();
  layer_to_pixel_nonprefer_dir.clear();

  for (auto& [lid, layer] : routing_layers) {
    TrackInfo& track_info = layer.get_track_info();
    EnvPixel pixel;

    int32_t x0 = track_info.get_x_start();
    int32_t y0 = track_info.get_y_start();
    int32_t nx = static_cast<int32_t>(track_info.get_x_count());
    int32_t ny = static_cast<int32_t>(track_info.get_y_count());
    int32_t dx = track_info.get_x_step();
    int32_t dy = track_info.get_y_step();

    EnvAxis x_axis = coverAxis(x0, nx, dx, die_x0, die_x1);
    EnvAxis y_axis = coverAxis(y0, ny, dy, die_y0, die_y1);

    pixel.set_x0(x_axis.get_origin());
    pixel.set_nx(x_axis.get_count());
    pixel.set_dx(x_axis.get_step());
    pixel.set_y0(y_axis.get_origin());
    pixel.set_ny(y_axis.get_count());
    pixel.set_dy(y_axis.get_step());

    if (!pixel.initPixel()) {
      return false;
    }
    layer_to_pixel_prefer_dir[lid] = pixel;
    layer_to_pixel_nonprefer_dir[lid] = std::move(pixel);
  }

  for (TopoEdge& edge : topo_pool.get_edge_pool()) {
    addPixelEdge(eb_model, edge);
  }
  for (TopoEdge& edge : topo_pool.get_special_edge_pool()) {
    addPixelEdge(eb_model, edge);
  }

  return true;
}

void EnvBuilder::addPixelEdge(EBModel& eb_model, TopoEdge& edge)
{
  if (edge.get_is_via()) {
    return;
  }

  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  std::map<size_t, EnvPixel>& layer_to_pixel_prefer_dir = eb_model.get_layer_to_pixel_prefer_dir();
  std::map<size_t, EnvPixel>& layer_to_pixel_nonprefer_dir = eb_model.get_layer_to_pixel_nonprefer_dir();
  const size_t layer_id = edge.get_layer_id();
  const bool layer_is_horz = routing_layers.at(layer_id).get_is_prefer_horizontal();

  if (edge.get_line_segment().get_is_horizontal() == layer_is_horz) {
    layer_to_pixel_prefer_dir.at(layer_id).addEdge(edge);
  } else {
    layer_to_pixel_nonprefer_dir.at(layer_id).addEdge(edge);
  }
}

void EnvBuilder::buildSearchTrackNumMap(EBModel& eb_model)
{
  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  std::map<size_t, RoutingLayer>& routing_layers = layout_data.get_routing_layer_map();
  std::map<size_t, int32_t>& layer_to_search_track_num = eb_model.get_layer_to_search_track_num();

  layer_to_search_track_num.clear();

  for (auto& [lid, layer] : routing_layers) {
    layer_to_search_track_num[lid] = 10;
  }
}

}  // namespace ircx
