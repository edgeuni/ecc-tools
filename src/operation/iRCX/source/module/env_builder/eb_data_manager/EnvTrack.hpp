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
/**
 * @file EnvTrack.hh
 * @brief iRCX module implementation detail.
 */
#pragma once

#include "EnvIntervalUtils.hpp"
#include "LineSegment.hpp"
#include "RCXHeader.hpp"
#include "TopoEdge.hpp"

namespace ircx {

// Parallel Overlap
struct EnvTrackOverlap {
  int32_t a0 = 0;
  int32_t a1 = 0;

  int32_t sp = kMaxDbu; // unsigned spacing = |current_fixed - coord|
  // edge == nullptr means this interval is not covered by any matched edge.
  TopoEdge* edge = nullptr;
};

struct EnvOverlapWidenContext {
  int32_t track_distance;  // |current_track_idx - base_track_idx|
  int32_t overlap_len;     // current raw overlap length before widening
  TopoEdge* edge;  // matched edge
};

class EnvTrack
{
 public:
  using EnvOverlapWidenFunc = std::function<int32_t(const EnvOverlapWidenContext&)>;

 private:
  struct EnvRemainingInterval {
    int32_t a0 = 0;
    int32_t a1 = 0;
  };

  struct EnvSearchContext {
    int32_t coord = 0;
    int32_t base_track_idx = 0;
    int32_t query_a0 = 0;
    int32_t query_a1 = 0;
    int step = 0;
    const EnvOverlapWidenFunc* widen_func = nullptr;
  };

  struct EnvTopoEdgeFixedLess
  {
    using is_transparent = void;

    bool operator()(TopoEdge* lhs,
                    TopoEdge* rhs) const
    {
      if (lhs == rhs) {
        return false;
      }
      if (lhs == nullptr || rhs == nullptr) {
        return lhs < rhs;
      }
      if (lhs->get_line_segment().get_coordinate() != rhs->get_line_segment().get_coordinate()) {
        return lhs->get_line_segment().get_coordinate() < rhs->get_line_segment().get_coordinate();
      }
      return lhs < rhs;
    }

    bool operator()(TopoEdge* lhs,
                    int32_t rhs_coord) const
    {
      return lhs->get_line_segment().get_coordinate() < rhs_coord;
    }

    bool operator()(int32_t lhs_coord,
                    TopoEdge* rhs) const
    {
      return lhs_coord < rhs->get_line_segment().get_coordinate();
    }
  };

  using EnvEdgeSet = std::set<TopoEdge*, EnvTopoEdgeFixedLess>;

 public:
  EnvTrack() = default;
  ~EnvTrack() = default;

  // getter
  int32_t get_track_origin() const { return track_ori_; }
  int32_t get_track_count() const { return track_num_; }
  int32_t get_track_step() const { return track_dlt_; }
  int32_t get_bucket_origin() const { return bucket_ori_; }
  int32_t get_bucket_count() const { return bucket_num_; }
  int32_t get_bucket_step() const { return bucket_dlt_; }

  // setter
  void set_track_origin(int32_t v) { track_ori_ = v; }
  void set_track_count(int32_t v) { track_num_ = v; }
  void set_track_step(int32_t v) { track_dlt_ = v; }
  void set_bucket_origin(int32_t v) { bucket_ori_ = v; }
  void set_bucket_count(int32_t v) { bucket_num_ = v; }
  void set_bucket_step(int32_t v) { bucket_dlt_ = v; }

  // coordinate mapping
  int32_t coordToTrack(int32_t coord) const { return (coord - track_ori_) / track_dlt_; }
  int32_t coordToBucket(int32_t coord) const { return (coord - bucket_ori_) / bucket_dlt_; }

  bool initTrack()
  {
    if (track_num_ <= 0 || track_dlt_ <= 0) {
      return false;
    }
    if (bucket_num_ <= 0 || bucket_dlt_ <= 0) {
      return false;
    }

    track_buckets_.assign(track_num_, std::vector<EnvEdgeSet>(bucket_num_));
    return true;
  }

  void addEdge(TopoEdge& edge)
  {
    int32_t a0 = edge.get_line_segment().get_lower();
    int32_t a1 = edge.get_line_segment().get_upper();
    ircx::env_interval::normalize(a0, a1);

    const int32_t track_idx = coordToTrack(edge.get_line_segment().get_coordinate());
    const int32_t bucket_idx0 = coordToBucket(a0);
    const int32_t bucket_idx1 = coordToBucket(a1 - 1);

    if (!trackValid(track_idx) || !bucketValid(bucket_idx0) || !bucketValid(bucket_idx1)) {
      return;
    }

    for (int32_t b = bucket_idx0; b <= bucket_idx1; ++b) {
      track_buckets_[track_idx][b].insert(&edge);
    }
  }

  // search_track_num > 0:
  //   search upward for search_track_num tracks, including the track containing coord;
  //   every returned non-null edge must satisfy edge->get_line_segment().get_coordinate() > coord
  //
  // search_track_num < 0:
  //   search downward for |search_track_num| tracks, including the track containing coord;
  //   every returned non-null edge must satisfy edge->get_line_segment().get_coordinate() < coord
  //
  // interval semantics: open interval (a0, a1)
  //
  // return semantics:
  //   - edge != nullptr: this interval is covered by the returned edge
  //   - edge == nullptr: this interval remains uncovered after searching all requested tracks
  //
  // widen_func semantics:
  //   - input:
  //       EnvOverlapWidenContext{
  //         track_distance = |current_track_idx - base_track_idx|,
  //         overlap_len    = current raw overlap length,
  //         edge           = matched edge
  //       }
  //   - output:
  //       single-side widening length
  //   - widening is symmetric:
  //       widened interval = (ov.a0 - ext, ov.a1 + ext)
  //   - widened interval is clipped into the original query interval [a0, a1]
  //
  // search strategy:
  //   1) on each track, collect candidate edges once from the initial remaining intervals;
  //   2) traverse those candidates in search direction order (nearest first by fixed);
  //   3) for each edge, compute overlap, widen it with widen_func, then clip to query interval;
  //   4) cut away the widened covered part from the remaining intervals;
  //   5) continue until no remaining interval is left or all candidates are consumed;
  //   6) recurse to the next track, until all requested tracks are processed or no remaining
  //      interval is left;
  //   7) any remaining uncovered intervals are returned with edge == nullptr.
  std::vector<EnvTrackOverlap> overlap(const LineSegment& line_seg,
                                        int32_t search_track_num,
                                        const EnvOverlapWidenFunc& widen_func = {}) const
  {
    std::vector<EnvTrackOverlap> result;
    if (search_track_num == 0) {
      return result;
    }

    int32_t a0 = line_seg.get_lower();
    int32_t a1 = line_seg.get_upper();
    ircx::env_interval::normalize(a0, a1);
    if (!intervalValid({a0, a1})) {
      return result;
    }

    const int32_t coord = line_seg.get_coordinate();
    const int32_t query_a0 = a0;
    const int32_t query_a1 = a1;

    const int32_t base_track_idx = coordToTrack(coord);
    if (!trackValid(base_track_idx)) {
      return result;
    }

    std::vector<EnvRemainingInterval> remaining;
    remaining.push_back({query_a0, query_a1});

    const int step = (search_track_num > 0) ? 1 : -1;
    const int32_t tracks_to_search = (search_track_num > 0) ? search_track_num : -search_track_num;

    EnvSearchContext ctx;
    ctx.coord = coord;
    ctx.base_track_idx = base_track_idx;
    ctx.query_a0 = query_a0;
    ctx.query_a1 = query_a1;
    ctx.step = step;
    ctx.widen_func = &widen_func;

    remaining = searchAcrossTracks(base_track_idx, tracks_to_search, remaining, result, ctx);

    for (const auto& iv : remaining) {
      EnvTrackOverlap ov;
      ov.a0 = iv.a0;
      ov.a1 = iv.a1;
      ov.sp = kMaxDbu;
      ov.edge = nullptr;
      result.push_back(ov);
    }

    return result;
  }

 private:
  static bool intervalValid(const EnvRemainingInterval& iv) { return iv.a0 < iv.a1; }

  static bool edgeIsInSearchDirection(TopoEdge* edge,
                                      const EnvSearchContext& ctx)
  {
    if (edge == nullptr) {
      return false;
    }
    return (ctx.step > 0) ? (edge->get_line_segment().get_coordinate() > ctx.coord)
                          : (edge->get_line_segment().get_coordinate() < ctx.coord);
  }

  static EnvTrackOverlap applyWidenAndClip(const EnvTrackOverlap& ov,
                                        int32_t track_idx,
                                        const EnvSearchContext& ctx)
  {
    EnvTrackOverlap widened = ov;

    if (ctx.widen_func != nullptr && *ctx.widen_func && ov.edge != nullptr) {
      const EnvOverlapWidenContext widen_ctx = {
          .track_distance = std::abs(track_idx - ctx.base_track_idx),
          .overlap_len = ov.a1 - ov.a0,
          .edge = ov.edge,
      };

      int32_t ext = (*ctx.widen_func)(widen_ctx);
      if (ext < 0) {
        ext = 0;
      }

      widened.a0 -= ext;
      widened.a1 += ext;
    }

    // clip into original query interval [query_a0, query_a1]
    widened.a0 = std::clamp(widened.a0, ctx.query_a0, ctx.query_a1);
    widened.a1 = std::clamp(widened.a1, ctx.query_a0, ctx.query_a1);

    return widened;
  }

  EnvEdgeSet collectCandidateEdgesOnTrack(int32_t track_idx,
                                       const std::vector<EnvRemainingInterval>& remaining) const
  {
    EnvEdgeSet ordered;
    if (!trackValid(track_idx)) {
      return ordered;
    }

    for (const auto& iv : remaining) {
      int32_t bucket_idx0 = coordToBucket(iv.a0);
      int32_t bucket_idx1 = coordToBucket(iv.a1 - 1);
      if (bucket_idx0 > bucket_idx1) {
        std::swap(bucket_idx0, bucket_idx1);
      }

      if (!bucketValid(bucket_idx0) || !bucketValid(bucket_idx1)) {
        continue;
      }

      for (int32_t b = bucket_idx0; b <= bucket_idx1; ++b) {
        const auto& edge_set = track_buckets_[track_idx][b];
        ordered.insert(edge_set.begin(), edge_set.end());
      }
    }

    return ordered;
  }

  bool edgeHitsRemaining(TopoEdge* edge,
                         const std::vector<EnvRemainingInterval>& remaining) const
  {
    if (edge == nullptr) {
      return false;
    }

    int32_t edge_a0 = edge->get_line_segment().get_lower();
    int32_t edge_a1 = edge->get_line_segment().get_upper();
    ircx::env_interval::normalize(edge_a0, edge_a1);

    for (const auto& iv : remaining) {
      if (ircx::env_interval::overlaps(edge_a0, edge_a1, iv.a0, iv.a1)) {
        return true;
      }
    }
    return false;
  }

  std::vector<EnvTrackOverlap> computeEdgeOverlaps(int32_t track_idx,
                                                TopoEdge* edge,
                                                const std::vector<EnvRemainingInterval>& remaining,
                                                const EnvSearchContext& ctx) const
  {
    std::vector<EnvTrackOverlap> overlaps;
    if (edge == nullptr) {
      return overlaps;
    }

    int32_t edge_a0 = edge->get_line_segment().get_lower();
    int32_t edge_a1 = edge->get_line_segment().get_upper();
    ircx::env_interval::normalize(edge_a0, edge_a1);

    for (const auto& iv : remaining) {
      if (!ircx::env_interval::overlaps(edge_a0, edge_a1, iv.a0, iv.a1)) {
        continue;
      }

      const auto overlap = ircx::env_interval::intersection(edge_a0, edge_a1, iv.a0, iv.a1);
      EnvTrackOverlap ov;
      ov.a0 = overlap.a0;
      ov.a1 = overlap.a1;
      ov.sp = std::abs(edge->get_line_segment().get_coordinate() - ctx.coord);
      ov.edge = edge;

      if (ov.a0 < ov.a1) {
        EnvTrackOverlap widened = applyWidenAndClip(ov, track_idx, ctx);

        // widened interval should not cross the current remaining fragment
        widened.a0 = std::max(widened.a0, iv.a0);
        widened.a1 = std::min(widened.a1, iv.a1);

        if (widened.a0 < widened.a1) {
          overlaps.push_back(widened);
        }
      }
    }

    return overlaps;
  }

  // Iteratively consume one track using a single ordered candidate set.
  // The key invariant is that `remaining` only shrinks, so an edge that does not
  // belong to the initial candidate set can never become relevant later.
  void searchWithinTrack(int32_t track_idx,
                         std::vector<EnvRemainingInterval> remaining,
                         std::vector<EnvTrackOverlap>& result,
                         const EnvSearchContext& ctx) const
  {
    if (!trackValid(track_idx) || remaining.empty()) {
      return;
    }

    const EnvEdgeSet ordered = collectCandidateEdgesOnTrack(track_idx, remaining);
    if (ordered.empty()) {
      return;
    }

    if (ctx.step > 0) {
      for (auto it = ordered.upper_bound(ctx.coord);
           it != ordered.end() && !remaining.empty();
           ++it) {
        consumeEdge(track_idx, *it, remaining, result, ctx);
      }
    } else {
      auto it = ordered.lower_bound(ctx.coord);
      for (auto rit = std::make_reverse_iterator(it);
           rit != ordered.rend() && !remaining.empty();
           ++rit) {
        consumeEdge(track_idx, *rit, remaining, result, ctx);
      }
    }
  }

  void consumeEdge(int32_t track_idx,
                   TopoEdge* edge,
                   std::vector<EnvRemainingInterval>& remaining,
                   std::vector<EnvTrackOverlap>& result,
                   const EnvSearchContext& ctx) const
  {
    if (!edgeHitsRemaining(edge, remaining)) {
      return;
    }

    std::vector<EnvTrackOverlap> overlap_list = computeEdgeOverlaps(track_idx, edge, remaining, ctx);
    if (overlap_list.empty()) {
      return;
    }

    result.insert(result.end(), overlap_list.begin(), overlap_list.end());

    std::vector<EnvRemainingInterval> next_remaining = remaining;
    for (const EnvTrackOverlap& overlap : overlap_list) {
      next_remaining = ircx::env_interval::subtract(next_remaining, overlap.a0, overlap.a1);
      if (next_remaining.empty()) {
        break;
      }
    }
    remaining = std::move(next_remaining);
  }

  // Recursively process tracks in the requested direction.
  // For each track:
  //   1) consume as much remaining interval as possible on this track;
  //   2) commit only the widened+clipped overlap pieces to the final result;
  //   3) subtract those committed overlap pieces from remaining;
  //   4) recurse to the next track;
  //   5) return whatever interval is still uncovered after all requested tracks are processed.
  std::vector<EnvRemainingInterval> searchAcrossTracks(int32_t track_idx,
                                              int32_t tracks_left,
                                              std::vector<EnvRemainingInterval> remaining,
                                              std::vector<EnvTrackOverlap>& result,
                                              const EnvSearchContext& ctx) const
  {
    if (tracks_left <= 0 || remaining.empty()) {
      return remaining;
    }

    if (!trackValid(track_idx)) {
      return remaining;
    }

    std::vector<EnvTrackOverlap> local_overlaps;
    searchWithinTrack(track_idx, remaining, local_overlaps, ctx);

    for (const auto& ov : local_overlaps) {
      // Defensive check: the directional constraint should already be guaranteed by
      if (!edgeIsInSearchDirection(ov.edge, ctx)) {
        continue;
      }

      result.push_back(ov);
      remaining = ircx::env_interval::subtract(remaining, ov.a0, ov.a1);
      if (remaining.empty()) {
        return remaining;
      }
    }

    return searchAcrossTracks(track_idx + ctx.step, tracks_left - 1, remaining, result, ctx);
  }

 private:
  // each track -> multiple buckets
  std::vector<std::vector<EnvEdgeSet>> track_buckets_;

  int32_t track_ori_ = 0;
  int32_t track_num_ = 0;
  int32_t track_dlt_ = 0;

  int32_t bucket_ori_ = 0;
  int32_t bucket_num_ = 0;
  int32_t bucket_dlt_ = 10000;

  bool trackValid(int32_t t) const { return 0 <= t && t < track_num_; }
  bool bucketValid(int32_t b) const { return 0 <= b && b < bucket_num_; }
};

}  // namespace ircx
