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

#include "DataManager.hpp"
#include "EBModel.hpp"
#include "EnvAxis.hpp"
#include "EnvLayerPixelOverlaps.hpp"
#include "EnvPixelOverlapMerge.hpp"
#include "EnvTrack.hpp"
#include "LineSegment.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "RCXHeader.hpp"
#include "TopoEdge.hpp"
#include "TrackInfo.hpp"

namespace ircx {

#define RCXEB (ircx::EnvBuilder::getInst())

class EnvBuilder
{
 public:
  static void initInst();
  static EnvBuilder& getInst();
  static void destroyInst();
  // function
  void build();

 private:
  // self
  static EnvBuilder* _eb_instance;

  EnvBuilder() = default;
  EnvBuilder(const EnvBuilder& other) = delete;
  EnvBuilder(EnvBuilder&& other) = delete;
  ~EnvBuilder() = default;
  EnvBuilder& operator=(const EnvBuilder& other) = delete;
  EnvBuilder& operator=(EnvBuilder&& other) = delete;
  // function
  void buildEBModel(EBModel& eb_model);
  bool buildNetEnvs(EBModel& eb_model);
  std::vector<CrossOverlapSub> clipCrossSegments(const std::vector<CrossOverlapSub>& cross_overlap_sub_list, int32_t a0, int32_t a1);
  std::vector<EnvLayerPixelOverlaps> collectCrossSide(EBModel& eb_model, const LineSegment& line_segment, size_t base_lid, bool search_up);
  bool buildTracks(EBModel& eb_model);
  void addTrackEdge(EBModel& eb_model, TopoEdge& edge);
  bool initTrackForDirection(EnvTrack& track, TrackInfo& track_info, GTLRectInt& rect, int32_t bucket_dlt, bool is_horz);
  EnvAxis coverAxis(int32_t origin, int32_t count, int32_t step, int32_t lo, int32_t hi);
  bool buildPixels(EBModel& eb_model);
  void addPixelEdge(EBModel& eb_model, TopoEdge& edge);
  void buildSearchTrackNumMap(EBModel& eb_model);
};

}  // namespace ircx
