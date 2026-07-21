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
#pragma once

#include "CrossOverlapSub.hpp"
#include "EnvLayerPixelOverlaps.hpp"
#include "EnvPixel.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class EnvPixelOverlapMerge
{
 public:
  void compute(int32_t query_a0,
               int32_t query_a1,
               const std::vector<EnvLayerPixelOverlaps>& dn_inputs,
               const std::vector<EnvLayerPixelOverlaps>& up_inputs,
               std::vector<CrossOverlapSub>& out) const
  {
    if (query_a0 > query_a1) {
      std::swap(query_a0, query_a1);
    }

    out.clear();
    if (!(query_a0 < query_a1)) {
      return;
    }

    std::vector<EnvLayerPixelOverlaps> dn_norm;
    std::vector<EnvLayerPixelOverlaps> up_norm;
    dn_norm.reserve(dn_inputs.size());
    up_norm.reserve(up_inputs.size());

    for (const EnvLayerPixelOverlaps& input : dn_inputs) {
      if (input.get_layer_id() == 0) {
        continue;
      }
      EnvLayerPixelOverlaps normalized_input;
      normalized_input.set_layer_id(input.get_layer_id());
      normalizeOne(query_a0, query_a1, input.get_pixel_overlap_list(), normalized_input.get_pixel_overlap_list());
      if (!normalized_input.get_pixel_overlap_list().empty()) {
        dn_norm.push_back(std::move(normalized_input));
      }
    }

    for (const EnvLayerPixelOverlaps& input : up_inputs) {
      if (input.get_layer_id() == 0) {
        continue;
      }
      EnvLayerPixelOverlaps normalized_input;
      normalized_input.set_layer_id(input.get_layer_id());
      normalizeOne(query_a0, query_a1, input.get_pixel_overlap_list(), normalized_input.get_pixel_overlap_list());
      if (!normalized_input.get_pixel_overlap_list().empty()) {
        up_norm.push_back(std::move(normalized_input));
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
  static size_t countBreakpoints(const std::vector<EnvLayerPixelOverlaps>& inputs)
  {
    size_t n = 0;
    for (const EnvLayerPixelOverlaps& input : inputs) {
      n += input.get_pixel_overlap_list().size() * 2;
    }
    return n;
  }

  static void addBreakpoints(const std::vector<EnvLayerPixelOverlaps>& inputs, std::vector<int32_t>& bp)
  {
    for (const EnvLayerPixelOverlaps& input : inputs) {
      for (const EnvPixelOverlap& pixel_overlap : input.get_pixel_overlap_list()) {
        bp.push_back(pixel_overlap.get_start_coordinate());
        bp.push_back(pixel_overlap.get_end_coordinate());
      }
    }
  }

  static void normalizeOne(int32_t query_a0,
                           int32_t query_a1,
                           const std::vector<EnvPixelOverlap>& in,
                           std::vector<EnvPixelOverlap>& out)
  {
    out.clear();
    out.reserve(in.size());

    for (const EnvPixelOverlap& pixel_overlap : in) {
      const int32_t a0 = std::max(query_a0, pixel_overlap.get_start_coordinate());
      const int32_t a1 = std::min(query_a1, pixel_overlap.get_end_coordinate());
      if (a0 < a1) {
        out.emplace_back(a0, a1);
      }
    }

    std::sort(out.begin(), out.end(), isPixelOverlapLess);

    std::vector<EnvPixelOverlap> merged;
    merged.reserve(out.size());

    for (const EnvPixelOverlap& pixel_overlap : out) {
      if (merged.empty() || merged.back().get_end_coordinate() < pixel_overlap.get_start_coordinate()) {
        merged.push_back(pixel_overlap);
      } else {
        merged.back().set_end_coordinate(std::max(merged.back().get_end_coordinate(), pixel_overlap.get_end_coordinate()));
      }
    }

    out.swap(merged);
  }

  static bool isPixelOverlapLess(const EnvPixelOverlap& lhs, const EnvPixelOverlap& rhs)
  {
    if (lhs.get_start_coordinate() != rhs.get_start_coordinate()) {
      return lhs.get_start_coordinate() < rhs.get_start_coordinate();
    }
    return lhs.get_end_coordinate() < rhs.get_end_coordinate();
  }

  static void advanceCursor(const std::vector<EnvPixelOverlap>& segs, size_t& idx, int32_t x)
  {
    while (idx < segs.size() && segs[idx].get_end_coordinate() <= x) {
      ++idx;
    }
  }

  static bool covers(const std::vector<EnvPixelOverlap>& segs, size_t idx, int32_t a0, int32_t a1)
  {
    return idx < segs.size() && segs[idx].get_start_coordinate() < a1 && segs[idx].get_end_coordinate() > a0;
  }

  static size_t firstCoveringLayer(const std::vector<EnvLayerPixelOverlaps>& inputs,
                                   std::vector<size_t>& cursors,
                                   int32_t a0,
                                   int32_t a1)
  {
    for (size_t i = 0; i < inputs.size(); ++i) {
      advanceCursor(inputs[i].get_pixel_overlap_list(), cursors[i], a0);
      if (covers(inputs[i].get_pixel_overlap_list(), cursors[i], a0, a1)) {
        return inputs[i].get_layer_id();
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

    if (!out.empty() && out.back().get_end_coordinate() == a0 && out.back().get_below_layer_id() == blw_layer
        && out.back().get_above_layer_id() == abv_layer) {
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
