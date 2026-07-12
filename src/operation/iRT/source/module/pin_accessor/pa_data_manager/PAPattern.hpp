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

#include "AccessPoint.hpp"
#include "EXTLayerRect.hpp"
#include "LayerCoord.hpp"
#include "PAPin.hpp"
#include "RTHeader.hpp"
#include "Segment.hpp"

namespace irt {

struct PAPatternKey
{
  std::string cell_master;
  int32_t orient = -1;
  std::string track_offset;
  std::string pin_set;

  bool operator<(const PAPatternKey& other) const
  {
    if (cell_master != other.cell_master) {
      return cell_master < other.cell_master;
    }
    if (orient != other.orient) {
      return orient < other.orient;
    }
    if (track_offset != other.track_offset) {
      return track_offset < other.track_offset;
    }
    return pin_set < other.pin_set;
  }
};

struct PAPatternInst
{
  PlanarCoord inst_origin;
  std::vector<std::pair<int32_t, PAPin*>> net_pin_pair_list;
};

struct PAPatternResult
{
  bool routed = false;
  std::map<std::string, std::vector<Segment<LayerCoord>>> pin_segment_map;
  std::map<std::string, std::vector<EXTLayerRect>> pin_patch_map;
  std::map<std::string, AccessPoint> pin_access_point_map;
};

}  // namespace irt
