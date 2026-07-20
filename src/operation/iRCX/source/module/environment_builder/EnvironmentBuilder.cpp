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
 * @file EnvironmentBuilder.cpp
 * @brief iRCX environment-builder module implementation.
 */
#include "EnvironmentBuilder.hpp"

#include "EnvironmentGeometry.hpp"
#include "EnvironmentIntervalEngine.hpp"
#include "EnvironmentIntervalUtils.hpp"
#include "EnvironmentParallel.hpp"

namespace ircx {

// public

void EnvironmentBuilder::initInst()
{
  if (_eb_instance == nullptr) {
    _eb_instance = new EnvironmentBuilder();
  }
}

EnvironmentBuilder& EnvironmentBuilder::getInst()
{
  if (_eb_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_eb_instance;
}

void EnvironmentBuilder::destroyInst()
{
  if (_eb_instance != nullptr) {
    delete _eb_instance;
    _eb_instance = nullptr;
  }
}

void EnvironmentBuilder::build()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  EBModel eb_model = initEBModel();
  buildEBModel(eb_model);

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

EnvironmentBuilder* EnvironmentBuilder::_eb_instance = nullptr;

EBModel EnvironmentBuilder::initEBModel()
{
  Database& database = RCXDM.getDatabase();
  EBModel eb_model;
  eb_model.set_temp_directory_path(RCXDM.getConfig().eb_temp_directory_path);
  eb_model.set_layout_data(&database.get_layout_data());
  eb_model.set_topo_pool(&database.get_topo_pool());
  return eb_model;
}

void EnvironmentBuilder::buildEBModel(EBModel& eb_model)
{
  if (!buildNetEnvironments(eb_model)) {
    RCXLOG.error(Loc::current(), "Build net environment failed!");
  }
}

bool EnvironmentBuilder::buildNetEnvironments(EBModel& eb_model)
{
  LayoutData* layout_data = eb_model.get_layout_data();
  TopoPool* topo_pool = eb_model.get_topo_pool();
  if (layout_data == nullptr) {
    return false;
  }
  if (topo_pool == nullptr) {
    return false;
  }

  if (!buildTracks(eb_model) || !buildPixels(eb_model)) {
    return false;
  }
  buildSearchTrackNumMap(eb_model);

  Size net_num = layout_data->get_regular_net_count();
  std::vector<NetEnvironment>& net_environments = RCXDM.getDatabase().get_net_environment_list();
  net_environments.clear();
  net_environments.resize(net_num);

  std::map<Size, RoutingLayer>& routing_layers = layout_data->get_routing_layer_map();
  const Size min_lid = routing_layers.empty() ? 0 : routing_layers.begin()->first;
  const Size max_lid = routing_layers.empty() ? 0 : routing_layers.rbegin()->first;
  std::map<Size, EnvironmentPixel>& layer_to_pixel_prefer_dir = eb_model.get_layer_to_pixel_prefer_dir();
  std::map<Size, EnvironmentPixel>& layer_to_pixel_nonprefer_dir = eb_model.get_layer_to_pixel_nonprefer_dir();
  std::map<Size, EnvironmentTrack>& layer_to_track_prefer_dir = eb_model.get_layer_to_track_prefer_dir();
  std::map<Size, EnvironmentTrack>& layer_to_track_nonprefer_dir = eb_model.get_layer_to_track_nonprefer_dir();
  std::map<Size, Dbu>& layer_to_search_track_num = eb_model.get_layer_to_search_track_num();

  auto clip_cross_segments = [](const std::vector<CrossOverlapSub>& full, Dbu a0, Dbu a1) {
    std::vector<CrossOverlapSub> clipped_cross_overlap_sub_list;
    for (const CrossOverlapSub& cross_overlap_sub : full) {
      Dbu clipped_a0 = std::max(a0, cross_overlap_sub.get_start_coordinate());
      Dbu clipped_a1 = std::min(a1, cross_overlap_sub.get_end_coordinate());
      if (!(clipped_a0 < clipped_a1)) {
        continue;
      }
      if (!clipped_cross_overlap_sub_list.empty()
          && clipped_cross_overlap_sub_list.back().get_end_coordinate() == clipped_a0
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
  };

  auto collect_cross_side = [&](const LineSegment& full_seg, Size base_lid, bool search_up) {
    std::vector<PixelOverlapMerge::LayerPixelOverlaps> bufs;

    for (Size delta = 1; delta <= eb_model.get_cross_layer(); ++delta) {
      Size cand_lid = 0;

      if (search_up) {
        if (base_lid > max_lid || max_lid - base_lid < delta) {
          break;
        }
        cand_lid = base_lid + delta;
      } else {
        if (base_lid < min_lid || base_lid - min_lid < delta) {
          break;
        }
        cand_lid = base_lid - delta;
      }

      auto it_layer = routing_layers.find(cand_lid);
      if (it_layer == routing_layers.end()) {
        continue;
      }

      // Cross-over only queries the conductor set orthogonal to full_seg.
      std::map<Size, EnvironmentPixel>& pixel_map =
          (it_layer->second.get_is_prefer_horizontal() != full_seg.get_is_horizontal()) ? layer_to_pixel_prefer_dir
                                                                                           : layer_to_pixel_nonprefer_dir;

      auto it_pixel = pixel_map.find(cand_lid);
      if (it_pixel == pixel_map.end()) {
        continue;
      }

      std::vector<PixelOverlap> segs = it_pixel->second.overlap(full_seg);
      if (segs.empty()) {
        continue;
      }

      // Smaller layer deltas have higher priority in PixelOverlapMerge.
      PixelOverlapMerge::LayerPixelOverlaps in;
      in.layer = cand_lid;
      in.segs = std::move(segs);
      bufs.push_back(std::move(in));
    }

    return bufs;
  };

  const int net_threads = parallel::threadCount(net_num);
#pragma omp parallel for schedule(dynamic) num_threads(net_threads)
  for (Size nid = 0; nid < net_num; nid++) {
    TrackOverlapMerge track_merger;
    PixelOverlapMerge pixel_merger;
    NetEnvironment& environment = net_environments[nid];

    for (TopoEdge& edge : topo_pool->get_net_edge_list(nid)) {
      if (edge.get_is_via()) {
        environment.append_edge_interval_list({});  // placeholder to keep index aligned with TopoPool
        continue;
      }

      const Size lid = edge.get_layer_id();
      LineSegment& query_seg = edge.get_line_segment();

      std::vector<TrackOverlap> track_ov_up;
      std::vector<TrackOverlap> track_ov_dn;
      const bool layer_is_horz = routing_layers.at(lid).get_is_prefer_horizontal();
      std::map<Size, EnvironmentTrack>& track_map =
          edge.get_line_segment().get_is_horizontal() == layer_is_horz ? layer_to_track_prefer_dir : layer_to_track_nonprefer_dir;
      if (const auto track_it = track_map.find(lid); track_it != track_map.end()) {
        track_ov_up = track_it->second.overlap(query_seg, layer_to_search_track_num[lid]);
        track_ov_dn = track_it->second.overlap(query_seg, -layer_to_search_track_num[lid]);
      }

      std::vector<EdgeEnvironmentInterval> out;
      track_merger.compute(query_seg.get_lower(), query_seg.get_upper(), track_ov_dn, track_ov_up, out);

      std::vector<PixelOverlapMerge::LayerPixelOverlaps> dn_inputs = collect_cross_side(query_seg, lid, false);
      std::vector<PixelOverlapMerge::LayerPixelOverlaps> up_inputs = collect_cross_side(query_seg, lid, true);

      std::vector<CrossOverlapSub> cross_full;
      pixel_merger.compute(query_seg.get_lower(), query_seg.get_upper(), dn_inputs, up_inputs, cross_full);

      for (EdgeEnvironmentInterval& interval : out) {
        interval.set_cross_overlap_sub_list(
            clip_cross_segments(cross_full, interval.get_start_coordinate(), interval.get_end_coordinate()));
      }

      environment.append_edge_interval_list(std::move(out));
    }
  }

  return true;
}

bool EnvironmentBuilder::buildTracks(EBModel& eb_model)
{
  LayoutData* layout_data = eb_model.get_layout_data();
  TopoPool* topo_pool = eb_model.get_topo_pool();
  if (layout_data == nullptr) {
    return false;
  }
  if (topo_pool == nullptr) {
    return false;
  }

  std::map<Size, RoutingLayer>& routing_layers = layout_data->get_routing_layer_map();
  GtlRectI& rect = layout_data->get_die_shape();
  Dbu bucket_dlt = static_cast<Dbu>(eb_model.get_bucket_size_um() * layout_data->get_dbu_per_micron());
  std::map<Size, EnvironmentTrack>& layer_to_track_prefer_dir = eb_model.get_layer_to_track_prefer_dir();
  std::map<Size, EnvironmentTrack>& layer_to_track_nonprefer_dir = eb_model.get_layer_to_track_nonprefer_dir();

  layer_to_track_prefer_dir.clear();
  layer_to_track_nonprefer_dir.clear();

  for (auto& [lid, layer] : routing_layers) {
    TrackInfo& track_info = layer.get_track_info();

    EnvironmentTrack prefer_track;
    if (!initTrackForDirection(prefer_track, track_info, rect, bucket_dlt, layer.get_is_prefer_horizontal())) {
      return false;
    }
    layer_to_track_prefer_dir[lid] = std::move(prefer_track);

    EnvironmentTrack nonprefer_track;
    if (!initTrackForDirection(nonprefer_track, track_info, rect, bucket_dlt, !layer.get_is_prefer_horizontal())) {
      return false;
    }
    layer_to_track_nonprefer_dir[lid] = std::move(nonprefer_track);
  }

  auto add_track_edge = [&](TopoEdge& edge) {
    if (edge.get_is_via()) {
      return;
    }

    const Size lid = edge.get_layer_id();
    const bool layer_is_horz = routing_layers.at(lid).get_is_prefer_horizontal();
    std::map<Size, EnvironmentTrack>& track_map =
        edge.get_line_segment().get_is_horizontal() == layer_is_horz ? layer_to_track_prefer_dir : layer_to_track_nonprefer_dir;
    track_map.at(lid).addEdge(edge);
  };

  for (TopoEdge& edge : topo_pool->get_edge_pool()) {
    add_track_edge(edge);
  }
  for (TopoEdge& edge : topo_pool->get_special_edge_pool()) {
    add_track_edge(edge);
  }

  return true;
}

bool EnvironmentBuilder::initTrackForDirection(EnvironmentTrack& track,
                                                TrackInfo& track_info,
                                                GtlRectI& rect,
                                                Dbu bucket_dlt,
                                                bool is_horz)
{
  const Dbu die_x0 = geom::minX(rect);
  const Dbu die_y0 = geom::minY(rect);
  const Dbu die_x1 = geom::maxX(rect);
  const Dbu die_y1 = geom::maxY(rect);
  const Dbu die_dx = geom::deltaX(rect);
  const Dbu die_dy = geom::deltaY(rect);

  const Dbu track_ori = is_horz ? track_info.get_y_start() : track_info.get_x_start();
  const Dbu track_num = is_horz ? static_cast<Dbu>(track_info.get_y_count()) : static_cast<Dbu>(track_info.get_x_count());
  const Dbu track_dlt = is_horz ? track_info.get_y_step() : track_info.get_x_step();
  const Dbu axis_lo = is_horz ? die_y0 : die_x0;
  const Dbu axis_hi = is_horz ? die_y1 : die_x1;
  EnvironmentAxis track_axis = coverAxis(track_ori, track_num, track_dlt, axis_lo, axis_hi);

  track.set_track_origin(track_axis.get_origin());
  track.set_track_count(track_axis.get_count());
  track.set_track_step(track_axis.get_step());
  track.set_bucket_origin(is_horz ? die_x0 : die_y0);
  track.set_bucket_count(ceilDivPositive(is_horz ? die_dx : die_dy, bucket_dlt));
  track.set_bucket_step(bucket_dlt);
  return track.initTrack();
}

EnvironmentAxis EnvironmentBuilder::coverAxis(Dbu origin, Dbu count, Dbu step, Dbu lo, Dbu hi)
{
  if (step <= 0) {
    return EnvironmentAxis(origin, count, step);
  }

  I64 axis_origin = origin;
  I64 axis_count = count;
  const I64 axis_step = step;

  if (axis_origin > lo) {
    const I64 shift = (axis_origin - lo + axis_step - 1) / axis_step;
    axis_origin -= shift * axis_step;
    axis_count += shift;
  }

  const I64 covered_hi = axis_origin + axis_step * axis_count;
  if (covered_hi <= hi) {
    axis_count += (static_cast<I64>(hi) - covered_hi) / axis_step + 1;
  }

  return EnvironmentAxis(static_cast<Dbu>(axis_origin), static_cast<Dbu>(axis_count), step);
}

Dbu EnvironmentBuilder::ceilDivPositive(Dbu value, Dbu divisor)
{
  if (value <= 0 || divisor <= 0) {
    return 0;
  }
  return static_cast<Dbu>((static_cast<I64>(value) + divisor - 1) / divisor);
}

bool EnvironmentBuilder::buildPixels(EBModel& eb_model)
{
  LayoutData* layout_data = eb_model.get_layout_data();
  TopoPool* topo_pool = eb_model.get_topo_pool();
  if (layout_data == nullptr) {
    return false;
  }
  if (topo_pool == nullptr) {
    return false;
  }

  std::map<Size, RoutingLayer>& routing_layers = layout_data->get_routing_layer_map();
  GtlRectI& rect = layout_data->get_die_shape();
  Dbu die_x0 = geom::minX(rect);
  Dbu die_y0 = geom::minY(rect);
  Dbu die_x1 = geom::maxX(rect);
  Dbu die_y1 = geom::maxY(rect);
  std::map<Size, EnvironmentPixel>& layer_to_pixel_prefer_dir = eb_model.get_layer_to_pixel_prefer_dir();
  std::map<Size, EnvironmentPixel>& layer_to_pixel_nonprefer_dir = eb_model.get_layer_to_pixel_nonprefer_dir();

  layer_to_pixel_prefer_dir.clear();
  layer_to_pixel_nonprefer_dir.clear();

  for (auto& [lid, layer] : routing_layers) {
    TrackInfo& track_info = layer.get_track_info();
    EnvironmentPixel pixel;

    Dbu x0 = track_info.get_x_start();
    Dbu y0 = track_info.get_y_start();
    Dbu nx = static_cast<Dbu>(track_info.get_x_count());
    Dbu ny = static_cast<Dbu>(track_info.get_y_count());
    Dbu dx = track_info.get_x_step();
    Dbu dy = track_info.get_y_step();

    EnvironmentAxis x_axis = coverAxis(x0, nx, dx, die_x0, die_x1);
    EnvironmentAxis y_axis = coverAxis(y0, ny, dy, die_y0, die_y1);

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

  auto add_pixel_edge = [&](TopoEdge& edge) {
    if (edge.get_is_via()) {
      return;
    }

    Size lid = edge.get_layer_id();
    bool layer_is_horz = routing_layers.at(lid).get_is_prefer_horizontal();

    if (edge.get_line_segment().get_is_horizontal() == layer_is_horz) {
      layer_to_pixel_prefer_dir.at(lid).addEdge(edge);
    } else {
      layer_to_pixel_nonprefer_dir.at(lid).addEdge(edge);
    }
  };

  for (TopoEdge& edge : topo_pool->get_edge_pool()) {
    add_pixel_edge(edge);
  }
  for (TopoEdge& edge : topo_pool->get_special_edge_pool()) {
    add_pixel_edge(edge);
  }

  return true;
}

void EnvironmentBuilder::buildSearchTrackNumMap(EBModel& eb_model)
{
  LayoutData* layout_data = eb_model.get_layout_data();
  std::map<Size, RoutingLayer>& routing_layers = layout_data->get_routing_layer_map();
  std::map<Size, Dbu>& layer_to_search_track_num = eb_model.get_layer_to_search_track_num();

  layer_to_search_track_num.clear();

  for (auto& [lid, layer] : routing_layers) {
    layer_to_search_track_num[lid] = 10;
  }
}

}  // namespace ircx
