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

#include "EnvOverlapWidenContext.hpp"
#include "EnvSearchContext.hpp"
#include "EnvTopoEdgeFixedLess.hpp"
#include "EnvTrackOverlap.hpp"
#include "LineSegment.hpp"
#include "RCXHeader.hpp"
#include "TopoEdge.hpp"
#include "Utility.hpp"

namespace ircx {

class EnvTrack
{
 public:
  using EnvOverlapWidenFunc = ircx::EnvOverlapWidenFunc;
  using EnvEdgeSet = std::set<TopoEdge*, EnvTopoEdgeFixedLess>;
  using EnvRemainingInterval = IntervalRange<int32_t>;
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
    RCXUTIL.normalizeInterval(a0, a1);

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
  //       widened interval = (overlap_start - ext, overlap_end + ext)
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
  std::vector<EnvTrackOverlap> overlap(const LineSegment& line_seg, int32_t search_track_num,
                                       const EnvOverlapWidenFunc& widen_func = {}) const
  {
    std::vector<EnvTrackOverlap> result;
    if (search_track_num == 0) {
      return result;
    }

    int32_t a0 = line_seg.get_lower();
    int32_t a1 = line_seg.get_upper();
    RCXUTIL.normalizeInterval(a0, a1);
    if (!RCXUTIL.isIntervalValid(a0, a1)) {
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
    remaining.emplace_back(query_a0, query_a1);

    const int32_t step = (search_track_num > 0) ? 1 : -1;
    const int32_t tracks_to_search = (search_track_num > 0) ? search_track_num : -search_track_num;

    EnvSearchContext ctx(coord, base_track_idx, query_a0, query_a1, step, widen_func);

    remaining = searchAcrossTracks(base_track_idx, tracks_to_search, remaining, result, ctx);

    for (const EnvRemainingInterval& interval : remaining) {
      EnvTrackOverlap ov;
      ov.set_start_coordinate(interval.get_start());
      ov.set_end_coordinate(interval.get_end());
      ov.set_spacing(INT32_MAX);
      ov.set_edge(nullptr);
      result.push_back(ov);
    }

    return result;
  }

 private:
  static bool edgeIsInSearchDirection(TopoEdge* edge, const EnvSearchContext& ctx)
  {
    if (edge == nullptr) {
      return false;
    }
    return (ctx.get_step() > 0) ? (edge->get_line_segment().get_coordinate() > ctx.get_coordinate())
                                : (edge->get_line_segment().get_coordinate() < ctx.get_coordinate());
  }

  static EnvTrackOverlap applyWidenAndClip(const EnvTrackOverlap& ov, int32_t track_idx, const EnvSearchContext& ctx)
  {
    EnvTrackOverlap widened = ov;

    if (ctx.get_widen_func() && ov.get_edge() != nullptr) {
      const EnvOverlapWidenContext widen_context(std::abs(track_idx - ctx.get_base_track_index()),
                                                 ov.get_end_coordinate() - ov.get_start_coordinate(), *ov.get_edge());

      int32_t ext = ctx.get_widen_func()(widen_context);
      if (ext < 0) {
        ext = 0;
      }

      widened.set_start_coordinate(widened.get_start_coordinate() - ext);
      widened.set_end_coordinate(widened.get_end_coordinate() + ext);
    }

    // clip into original query interval [query_a0, query_a1]
    widened.set_start_coordinate(
        std::clamp(widened.get_start_coordinate(), ctx.get_query_start_coordinate(), ctx.get_query_end_coordinate()));
    widened.set_end_coordinate(std::clamp(widened.get_end_coordinate(), ctx.get_query_start_coordinate(), ctx.get_query_end_coordinate()));

    return widened;
  }

  EnvEdgeSet collectCandidateEdgesOnTrack(int32_t track_idx, const std::vector<EnvRemainingInterval>& remaining) const
  {
    EnvEdgeSet ordered;
    if (!trackValid(track_idx)) {
      return ordered;
    }

    for (const EnvRemainingInterval& interval : remaining) {
      int32_t bucket_idx0 = coordToBucket(interval.get_start());
      int32_t bucket_idx1 = coordToBucket(interval.get_end() - 1);
      if (bucket_idx0 > bucket_idx1) {
        std::swap(bucket_idx0, bucket_idx1);
      }

      if (!bucketValid(bucket_idx0) || !bucketValid(bucket_idx1)) {
        continue;
      }

      for (int32_t b = bucket_idx0; b <= bucket_idx1; ++b) {
        const EnvEdgeSet& edge_set = track_buckets_[track_idx][b];
        ordered.insert(edge_set.begin(), edge_set.end());
      }
    }

    return ordered;
  }

  bool edgeHitsRemaining(TopoEdge* edge, const std::vector<EnvRemainingInterval>& remaining) const
  {
    if (edge == nullptr) {
      return false;
    }

    int32_t edge_a0 = edge->get_line_segment().get_lower();
    int32_t edge_a1 = edge->get_line_segment().get_upper();
    RCXUTIL.normalizeInterval(edge_a0, edge_a1);

    for (const EnvRemainingInterval& interval : remaining) {
      if (RCXUTIL.isIntervalOverlap(edge_a0, edge_a1, interval.get_start(), interval.get_end())) {
        return true;
      }
    }
    return false;
  }

  std::vector<EnvTrackOverlap> computeEdgeOverlaps(int32_t track_idx, TopoEdge* edge, const std::vector<EnvRemainingInterval>& remaining,
                                                   const EnvSearchContext& ctx) const
  {
    std::vector<EnvTrackOverlap> overlaps;
    if (edge == nullptr) {
      return overlaps;
    }

    int32_t edge_a0 = edge->get_line_segment().get_lower();
    int32_t edge_a1 = edge->get_line_segment().get_upper();
    RCXUTIL.normalizeInterval(edge_a0, edge_a1);

    for (const EnvRemainingInterval& interval : remaining) {
      if (!RCXUTIL.isIntervalOverlap(edge_a0, edge_a1, interval.get_start(), interval.get_end())) {
        continue;
      }

      const IntervalRange<int32_t> overlap = RCXUTIL.getIntervalIntersection(edge_a0, edge_a1, interval.get_start(), interval.get_end());
      EnvTrackOverlap ov;
      ov.set_start_coordinate(overlap.get_start());
      ov.set_end_coordinate(overlap.get_end());
      ov.set_spacing(std::abs(edge->get_line_segment().get_coordinate() - ctx.get_coordinate()));
      ov.set_edge(edge);

      if (ov.get_start_coordinate() < ov.get_end_coordinate()) {
        EnvTrackOverlap widened = applyWidenAndClip(ov, track_idx, ctx);

        // widened interval should not cross the current remaining fragment
        widened.set_start_coordinate(std::max(widened.get_start_coordinate(), interval.get_start()));
        widened.set_end_coordinate(std::min(widened.get_end_coordinate(), interval.get_end()));

        if (widened.get_start_coordinate() < widened.get_end_coordinate()) {
          overlaps.push_back(widened);
        }
      }
    }

    return overlaps;
  }

  // Iteratively consume one track using a single ordered candidate set.
  // The key invariant is that `remaining` only shrinks, so an edge that does not
  // belong to the initial candidate set can never become relevant later.
  void searchWithinTrack(int32_t track_idx, std::vector<EnvRemainingInterval> remaining, std::vector<EnvTrackOverlap>& result,
                         const EnvSearchContext& ctx) const
  {
    if (!trackValid(track_idx) || remaining.empty()) {
      return;
    }

    const EnvEdgeSet ordered = collectCandidateEdgesOnTrack(track_idx, remaining);
    if (ordered.empty()) {
      return;
    }

    if (ctx.get_step() > 0) {
      for (EnvEdgeSet::const_iterator iter = ordered.upper_bound(ctx.get_coordinate()); iter != ordered.end() && !remaining.empty();
           ++iter) {
        consumeEdge(track_idx, *iter, remaining, result, ctx);
      }
    } else {
      EnvEdgeSet::const_iterator iter = ordered.lower_bound(ctx.get_coordinate());
      for (EnvEdgeSet::const_reverse_iterator reverse_iter = std::make_reverse_iterator(iter);
           reverse_iter != ordered.rend() && !remaining.empty(); ++reverse_iter) {
        consumeEdge(track_idx, *reverse_iter, remaining, result, ctx);
      }
    }
  }

  void consumeEdge(int32_t track_idx, TopoEdge* edge, std::vector<EnvRemainingInterval>& remaining, std::vector<EnvTrackOverlap>& result,
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
      next_remaining = RCXUTIL.subtractInterval(next_remaining, overlap.get_start_coordinate(), overlap.get_end_coordinate());
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
  std::vector<EnvRemainingInterval> searchAcrossTracks(int32_t track_idx, int32_t tracks_left, std::vector<EnvRemainingInterval> remaining,
                                                       std::vector<EnvTrackOverlap>& result, const EnvSearchContext& ctx) const
  {
    if (tracks_left <= 0 || remaining.empty()) {
      return remaining;
    }

    if (!trackValid(track_idx)) {
      return remaining;
    }

    std::vector<EnvTrackOverlap> local_overlaps;
    searchWithinTrack(track_idx, remaining, local_overlaps, ctx);

    for (const EnvTrackOverlap& overlap : local_overlaps) {
      // Defensive check: the directional constraint should already be guaranteed by
      if (!edgeIsInSearchDirection(overlap.get_edge(), ctx)) {
        continue;
      }

      result.push_back(overlap);
      remaining = RCXUTIL.subtractInterval(remaining, overlap.get_start_coordinate(), overlap.get_end_coordinate());
      if (remaining.empty()) {
        return remaining;
      }
    }

    return searchAcrossTracks(track_idx + ctx.get_step(), tracks_left - 1, remaining, result, ctx);
  }

 private:
  // each track -> multiple buckets
  std::vector<std::vector<EnvEdgeSet>> track_buckets_;

  int32_t track_ori_ = INT32_MAX;
  int32_t track_num_ = 0;
  int32_t track_dlt_ = 0;

  int32_t bucket_ori_ = INT32_MAX;
  int32_t bucket_num_ = 0;
  int32_t bucket_dlt_ = 0;

  bool trackValid(int32_t t) const { return 0 <= t && t < track_num_; }
  bool bucketValid(int32_t b) const { return 0 <= b && b < bucket_num_; }
};

}  // namespace ircx
