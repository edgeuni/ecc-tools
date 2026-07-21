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

#include "EdgeEnvInterval.hpp"
#include "EnvTrack.hpp"
#include "EnvTrackOverlap.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class EnvTrackOverlapMerge
{
 public:
  void compute(int32_t query_a0,
               int32_t query_a1,
               const std::vector<EnvTrackOverlap>& dn_in,
               const std::vector<EnvTrackOverlap>& up_in,
               std::vector<EdgeEnvInterval>& out) const
  {
    if (query_a0 > query_a1) {
      std::swap(query_a0, query_a1);
    }

    out.clear();
    if (!(query_a0 < query_a1)) {
      return;
    }

    std::vector<EnvTrackOverlap> dn;
    std::vector<EnvTrackOverlap> up;

    normalizeSide(query_a0, query_a1, dn_in, dn);
    normalizeSide(query_a0, query_a1, up_in, up);

    mergeTwoSides(dn, up, out);
  }

 private:
  static EnvTrackOverlap makeNullOverlap(int32_t a0, int32_t a1)
  {
    return EnvTrackOverlap(a0, a1, INT32_MAX, nullptr);
  }

  static void emitTrackOverlap(std::vector<EnvTrackOverlap>& out, const EnvTrackOverlap& ov)
  {
    if (!(ov.get_start_coordinate() < ov.get_end_coordinate())) {
      return;
    }

    if (!out.empty() && out.back().get_end_coordinate() == ov.get_start_coordinate()
        && out.back().get_edge() == ov.get_edge() && out.back().get_spacing() == ov.get_spacing()) {
      out.back().set_end_coordinate(ov.get_end_coordinate());
      return;
    }

    out.push_back(ov);
  }

  void normalizeSide(int32_t query_a0,
                     int32_t query_a1,
                     const std::vector<EnvTrackOverlap>& in,
                     std::vector<EnvTrackOverlap>& out) const
  {
    out.clear();

    std::vector<EnvTrackOverlap> tmp;
    tmp.reserve(in.size());

    for (const EnvTrackOverlap& overlap : in) {
      const int32_t a0 = std::max(query_a0, overlap.get_start_coordinate());
      const int32_t a1 = std::min(query_a1, overlap.get_end_coordinate());
      if (!(a0 < a1)) {
        continue;
      }

      EnvTrackOverlap clipped = overlap;
      clipped.set_start_coordinate(a0);
      clipped.set_end_coordinate(a1);
      tmp.push_back(clipped);
    }

    std::sort(tmp.begin(), tmp.end(), isTrackOverlapLess);

    int32_t cursor = query_a0;
    for (const EnvTrackOverlap& overlap : tmp) {
      if (cursor < overlap.get_start_coordinate()) {
        emitTrackOverlap(out, makeNullOverlap(cursor, overlap.get_start_coordinate()));
      }

      emitTrackOverlap(out, overlap);
      cursor = overlap.get_end_coordinate();
    }

    if (cursor < query_a1) {
      emitTrackOverlap(out, makeNullOverlap(cursor, query_a1));
    }

    if (out.empty()) {
      out.push_back(makeNullOverlap(query_a0, query_a1));
    }
  }

  static bool isTrackOverlapLess(const EnvTrackOverlap& lhs, const EnvTrackOverlap& rhs)
  {
    if (lhs.get_start_coordinate() != rhs.get_start_coordinate()) {
      return lhs.get_start_coordinate() < rhs.get_start_coordinate();
    }
    if (lhs.get_end_coordinate() != rhs.get_end_coordinate()) {
      return lhs.get_end_coordinate() < rhs.get_end_coordinate();
    }
    if (lhs.get_edge() != rhs.get_edge()) {
      if (lhs.get_edge() == nullptr || rhs.get_edge() == nullptr) {
        return lhs.get_edge() == nullptr;
      }
      if (lhs.get_edge()->get_is_special_net() != rhs.get_edge()->get_is_special_net()) {
        return lhs.get_edge()->get_is_special_net() < rhs.get_edge()->get_is_special_net();
      }
      if (lhs.get_edge()->get_net_id() != rhs.get_edge()->get_net_id()) {
        return lhs.get_edge()->get_net_id() < rhs.get_edge()->get_net_id();
      }
      return lhs.get_edge()->get_edge_id() < rhs.get_edge()->get_edge_id();
    }
    return lhs.get_spacing() < rhs.get_spacing();
  }

  static void emitOutput(std::vector<EdgeEnvInterval>& out,
                         int32_t a0,
                         int32_t a1,
                         const EnvTrackOverlap& dn,
                         const EnvTrackOverlap& up)
  {
    if (!(a0 < a1)) {
      return;
    }

    const int32_t l_sp = dn.get_spacing();
    const int32_t h_sp = up.get_spacing();
    TopoEdge* l_edge = dn.get_edge();
    TopoEdge* h_edge = up.get_edge();

    if (!out.empty() && out.back().get_end_coordinate() == a0 && out.back().get_lower_adjacent_edge() == l_edge
        && out.back().get_upper_adjacent_edge() == h_edge && out.back().get_lower_spacing() == l_sp
        && out.back().get_upper_spacing() == h_sp) {
      out.back().set_end_coordinate(a1);
      return;
    }

    EdgeEnvInterval iv;
    iv.set_start_coordinate(a0);
    iv.set_end_coordinate(a1);
    iv.set_lower_adjacent_edge(l_edge);
    iv.set_upper_adjacent_edge(h_edge);
    iv.set_lower_spacing(l_sp);
    iv.set_upper_spacing(h_sp);
    out.push_back(iv);
  }

  static void mergeTwoSides(const std::vector<EnvTrackOverlap>& dn,
                            const std::vector<EnvTrackOverlap>& up,
                            std::vector<EdgeEnvInterval>& out)
  {
    out.clear();
    if (dn.empty() || up.empty()) {
      return;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < dn.size() && j < up.size()) {
      const int32_t s = std::max(dn[i].get_start_coordinate(), up[j].get_start_coordinate());
      const int32_t t = std::min(dn[i].get_end_coordinate(), up[j].get_end_coordinate());

      if (s < t) {
        emitOutput(out, s, t, dn[i], up[j]);
      }

      if (dn[i].get_end_coordinate() == t) {
        ++i;
      }
      if (up[j].get_end_coordinate() == t) {
        ++j;
      }
    }
  }
};

}  // namespace ircx
