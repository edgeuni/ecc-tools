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
 * @file IntervalEngine.hh
 * @brief iRCX module implementation detail.
 */
#pragma once

#include "EdgeEnvironmentInterval.hpp"
#include "EnvironmentPixel.hpp"
#include "EnvironmentTrack.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class TrackOverlapMerge
{
 public:
  void compute(int32_t query_a0,
               int32_t query_a1,
               const std::vector<TrackOverlap>& dn_in,
               const std::vector<TrackOverlap>& up_in,
               std::vector<EdgeEnvironmentInterval>& out) const
  {
    if (query_a0 > query_a1) {
      std::swap(query_a0, query_a1);
    }

    out.clear();
    if (!(query_a0 < query_a1)) {
      return;
    }

    std::vector<TrackOverlap> dn;
    std::vector<TrackOverlap> up;

    normalizeSide(query_a0, query_a1, dn_in, dn);
    normalizeSide(query_a0, query_a1, up_in, up);

    mergeTwoSides(dn, up, out);
  }

 private:
  static TrackOverlap makeNullOverlap(int32_t a0,
                                      int32_t a1)
  {
    TrackOverlap ov;
    ov.a0 = a0;
    ov.a1 = a1;
    ov.sp = kMaxDbu;
    ov.edge = nullptr;
    return ov;
  }

  static void emitTrackOverlap(std::vector<TrackOverlap>& out,
                               const TrackOverlap& ov)
  {
    if (!(ov.a0 < ov.a1)) {
      return;
    }

    if (!out.empty() &&
        out.back().a1 == ov.a0 &&
        out.back().edge == ov.edge &&
        out.back().sp == ov.sp) {
      out.back().a1 = ov.a1;
      return;
    }

    out.push_back(ov);
  }

  void normalizeSide(int32_t query_a0,
                     int32_t query_a1,
                     const std::vector<TrackOverlap>& in,
                     std::vector<TrackOverlap>& out) const
  {
    out.clear();

    std::vector<TrackOverlap> tmp;
    tmp.reserve(in.size());

    for (const auto& ov : in) {
      const int32_t a0 = std::max(query_a0, ov.a0);
      const int32_t a1 = std::min(query_a1, ov.a1);
      if (!(a0 < a1)) {
        continue;
      }

      TrackOverlap clipped = ov;
      clipped.a0 = a0;
      clipped.a1 = a1;
      tmp.push_back(clipped);
    }

    std::sort(tmp.begin(), tmp.end(), isTrackOverlapLess);

    int32_t cursor = query_a0;
    for (const auto& ov : tmp) {

      if (cursor < ov.a0) {
        emitTrackOverlap(out, makeNullOverlap(cursor, ov.a0));
      }

      emitTrackOverlap(out, ov);
      cursor = ov.a1;
    }

    if (cursor < query_a1) {
      emitTrackOverlap(out, makeNullOverlap(cursor, query_a1));
    }

    if (out.empty()) {
      out.push_back(makeNullOverlap(query_a0, query_a1));
    }
  }

  static bool isTrackOverlapLess(const TrackOverlap& lhs, const TrackOverlap& rhs)
  {
    if (lhs.a0 != rhs.a0) {
      return lhs.a0 < rhs.a0;
    }
    if (lhs.a1 != rhs.a1) {
      return lhs.a1 < rhs.a1;
    }
    if (lhs.edge != rhs.edge) {
      return lhs.edge < rhs.edge;
    }
    return lhs.sp < rhs.sp;
  }

  static void emitOutput(std::vector<EdgeEnvironmentInterval>& out,
                         int32_t a0,
                         int32_t a1,
                         const TrackOverlap& dn,
                         const TrackOverlap& up)
  {
    if (!(a0 < a1)) {
      return;
    }

    const int32_t l_sp = dn.sp;
    const int32_t h_sp = up.sp;
    TopoEdge* l_edge = dn.edge;
    TopoEdge* h_edge = up.edge;

    if (!out.empty() &&
        out.back().get_end_coordinate() == a0 &&
        out.back().get_lower_adjacent_edge() == l_edge &&
        out.back().get_upper_adjacent_edge() == h_edge &&
        out.back().get_lower_spacing() == l_sp &&
        out.back().get_upper_spacing() == h_sp) {
      out.back().set_end_coordinate(a1);
      return;
    }

    EdgeEnvironmentInterval iv;
    iv.set_start_coordinate(a0);
    iv.set_end_coordinate(a1);
    iv.set_lower_adjacent_edge(l_edge);
    iv.set_upper_adjacent_edge(h_edge);
    iv.set_lower_spacing(l_sp);
    iv.set_upper_spacing(h_sp);
    out.push_back(iv);
  }

  static void mergeTwoSides(const std::vector<TrackOverlap>& dn,
                            const std::vector<TrackOverlap>& up,
                            std::vector<EdgeEnvironmentInterval>& out)
  {
    out.clear();
    if (dn.empty() || up.empty()) {
      return;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < dn.size() && j < up.size()) {
      const int32_t s = std::max(dn[i].a0, up[j].a0);
      const int32_t t = std::min(dn[i].a1, up[j].a1);

      if (s < t) {
        emitOutput(out, s, t, dn[i], up[j]);
      }

      if (dn[i].a1 == t) {
        ++i;
      }
      if (up[j].a1 == t) {
        ++j;
      }
    }
  }
};

class PixelOverlapMerge
{
 public:
  struct LayerPixelOverlaps {
    size_t layer = 0;                 // 0 means invalid / absent
    std::vector<PixelOverlap> segs; // higher priority comes first in input order
  };

  void compute(int32_t query_a0,
               int32_t query_a1,
               const std::vector<LayerPixelOverlaps>& dn_inputs,
               const std::vector<LayerPixelOverlaps>& up_inputs,
               std::vector<CrossOverlapSub>& out) const
  {
    if (query_a0 > query_a1) {
      std::swap(query_a0, query_a1);
    }

    out.clear();
    if (!(query_a0 < query_a1)) {
      return;
    }

    std::vector<NormalizedLayerPixelOverlaps> dn_norm;
    std::vector<NormalizedLayerPixelOverlaps> up_norm;
    dn_norm.reserve(dn_inputs.size());
    up_norm.reserve(up_inputs.size());

    for (const auto& in : dn_inputs) {
      if (in.layer == 0) {
        continue;
      }
      NormalizedLayerPixelOverlaps ni;
      ni.layer = in.layer;
      normalizeOne(query_a0, query_a1, in.segs, ni.segs);
      if (!ni.segs.empty()) {
        dn_norm.push_back(std::move(ni));
      }
    }

    for (const auto& in : up_inputs) {
      if (in.layer == 0) {
        continue;
      }
      NormalizedLayerPixelOverlaps ni;
      ni.layer = in.layer;
      normalizeOne(query_a0, query_a1, in.segs, ni.segs);
      if (!ni.segs.empty()) {
        up_norm.push_back(std::move(ni));
      }
    }

    std::vector<int32_t> bp;
    bp.reserve(2 + countBreakpoints(dn_norm) + countBreakpoints(up_norm));
    bp.push_back(query_a0);
    bp.push_back(query_a1);

    addBreakpoints(dn_norm, bp);
    addBreakpoints(up_norm, bp);

    std::sort(bp.begin(), bp.end());
    bp.erase(std::unique(bp.begin(), bp.end()), bp.end());

    if (bp.size() < 2) {
      return;
    }

    std::vector<size_t> dn_cursor(dn_norm.size(), 0);
    std::vector<size_t> up_cursor(up_norm.size(), 0);

    for (size_t k = 0; k + 1 < bp.size(); ++k) {
      const int32_t a0 = bp[k];
      const int32_t a1 = bp[k + 1];
      if (!(a0 < a1)) {
        continue;
      }

      const size_t blw_layer = firstCoveringLayer(dn_norm, dn_cursor, a0, a1);
      const size_t abv_layer = firstCoveringLayer(up_norm, up_cursor, a0, a1);

      emit(a0, a1, blw_layer, abv_layer, out);
    }
  }

 private:
  struct NormalizedLayerPixelOverlaps {
    size_t layer = 0;
    std::vector<PixelOverlap> segs;
  };

  static size_t countBreakpoints(const std::vector<NormalizedLayerPixelOverlaps>& inputs)
  {
    size_t n = 0;
    for (const auto& in : inputs) {
      n += static_cast<size_t>(in.segs.size() * 2);
    }
    return n;
  }

  static void addBreakpoints(const std::vector<NormalizedLayerPixelOverlaps>& inputs,
                             std::vector<int32_t>& bp)
  {
    for (const auto& in : inputs) {
      for (const auto& seg : in.segs) {
        bp.push_back(seg.a0);
        bp.push_back(seg.a1);
      }
    }
  }

  static void normalizeOne(int32_t query_a0,
                           int32_t query_a1,
                           const std::vector<PixelOverlap>& in,
                           std::vector<PixelOverlap>& out)
  {
    out.clear();
    out.reserve(in.size());

    for (const auto& ov : in) {
      const int32_t a0 = std::max(query_a0, ov.a0);
      const int32_t a1 = std::min(query_a1, ov.a1);
      if (a0 < a1) {
        out.push_back(PixelOverlap{a0, a1});
      }
    }

    std::sort(out.begin(), out.end(), isPixelOverlapLess);

    std::vector<PixelOverlap> merged;
    merged.reserve(out.size());

    for (const auto& ov : out) {
      if (merged.empty() || merged.back().a1 < ov.a0) {
        merged.push_back(ov);
      } else {
        merged.back().a1 = std::max(merged.back().a1, ov.a1);
      }
    }

    out.swap(merged);
  }

  static bool isPixelOverlapLess(const PixelOverlap& lhs, const PixelOverlap& rhs)
  {
    if (lhs.a0 != rhs.a0) {
      return lhs.a0 < rhs.a0;
    }
    return lhs.a1 < rhs.a1;
  }

  static void advanceCursor(const std::vector<PixelOverlap>& segs,
                            size_t& idx,
                            int32_t x)
  {
    while (idx < segs.size() && segs[idx].a1 <= x) {
      ++idx;
    }
  }

  static bool covers(const std::vector<PixelOverlap>& segs,
                     size_t idx,
                     int32_t a0,
                     int32_t a1)
  {
    return idx < segs.size() && segs[idx].a0 < a1 && segs[idx].a1 > a0;
  }

  static size_t firstCoveringLayer(const std::vector<NormalizedLayerPixelOverlaps>& inputs,
                                 std::vector<size_t>& cursors,
                                 int32_t a0,
                                 int32_t a1)
  {
    for (size_t i = 0; i < inputs.size(); ++i) {
      advanceCursor(inputs[i].segs, cursors[i], a0);
      if (covers(inputs[i].segs, cursors[i], a0, a1)) {
        return inputs[i].layer;
      }
    }
    return 0;
  }

  static void emit(int32_t a0,
                   int32_t a1,
                   size_t blw_layer,
                   size_t abv_layer,
                   std::vector<CrossOverlapSub>& out)
  {
    if (!(a0 < a1)) {
      return;
    }

    // The uncovered span is represented implicitly by gaps between emitted
    // cross-over segments and handled by downstream consumers as substrate/none.
    if (blw_layer == 0 && abv_layer == 0) {
      return;
    }

    if (!out.empty() &&
        out.back().get_end_coordinate() == a0 &&
        out.back().get_below_layer_id() == blw_layer &&
        out.back().get_above_layer_id() == abv_layer) {
      out.back().set_end_coordinate(a1);
      return;
    }

    CrossOverlapSub sub;
    sub.set_start_coordinate(a0);
    sub.set_end_coordinate(a1);
    sub.set_below_layer_id(blw_layer);
    sub.set_above_layer_id(abv_layer);
    out.push_back(sub);
  }
};

}  // namespace ircx
