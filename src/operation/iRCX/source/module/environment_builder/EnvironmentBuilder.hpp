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
#include "EnvironmentAxis.hpp"
#include "EnvironmentTrack.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "RCXHeader.hpp"
#include "TrackInfo.hpp"

namespace ircx {

#define RCXEB (ircx::EnvironmentBuilder::getInst())

class EnvironmentBuilder
{
 public:
  static void initInst();
  static EnvironmentBuilder& getInst();
  static void destroyInst();
  // function
  void build();

 private:
  // self
  static EnvironmentBuilder* _eb_instance;

  EnvironmentBuilder() = default;
  EnvironmentBuilder(const EnvironmentBuilder& other) = delete;
  EnvironmentBuilder(EnvironmentBuilder&& other) = delete;
  ~EnvironmentBuilder() = default;
  EnvironmentBuilder& operator=(const EnvironmentBuilder& other) = delete;
  EnvironmentBuilder& operator=(EnvironmentBuilder&& other) = delete;
  // function
  EBModel initEBModel();
  void buildEBModel(EBModel& eb_model);
  bool buildNetEnvironments(EBModel& eb_model);
  bool buildTracks(EBModel& eb_model);
  bool initTrackForDirection(EnvironmentTrack& track, TrackInfo& track_info, GtlRectI& rect, Dbu bucket_dlt, bool is_horz);
  EnvironmentAxis coverAxis(Dbu origin, Dbu count, Dbu step, Dbu lo, Dbu hi);
  Dbu ceilDivPositive(Dbu value, Dbu divisor);
  bool buildPixels(EBModel& eb_model);
  void buildSearchTrackNumMap(EBModel& eb_model);
};

}  // namespace ircx
