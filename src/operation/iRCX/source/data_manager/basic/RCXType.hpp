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

#include "RCXHeader.hpp"

namespace ircx {

using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;

using F32 = float;
using F64 = double;

inline constexpr int32_t kMaxDbu = std::numeric_limits<int32_t>::max();
inline constexpr size_t kMaxSize = std::numeric_limits<size_t>::max();
inline constexpr double kMaxMicron = std::numeric_limits<double>::max();
inline constexpr size_t kSpecialNetId = kMaxSize - 1;

using BgPointI = boost::geometry::model::point<int32_t, 2, boost::geometry::cs::cartesian>;
using BgBoxI = boost::geometry::model::box<BgPointI>;
using BgPolygonI = boost::geometry::model::polygon<BgPointI>;

using GtlPointI = boost::polygon::point_data<int32_t>;
using GtlRectI = boost::polygon::rectangle_data<int32_t>;
using GtlPolyI = boost::polygon::polygon_90_data<int32_t>;
using GtlPolySetI = boost::polygon::polygon_90_set_data<int32_t>;

using GtlPointF = boost::polygon::point_data<F64>;
using GtlRectF = boost::polygon::rectangle_data<F64>;
using GtlPolyF = boost::polygon::polygon_90_data<F64>;
using GtlPolySetF = boost::polygon::polygon_90_set_data<F64>;

using LayerShape = std::pair<size_t, GtlRectI>;

namespace unit {

inline double to_micron(int32_t value, int32_t dbu_per_micron)
{
  return static_cast<double>(value) / static_cast<double>(dbu_per_micron);
}

inline int32_t to_dbu(double value, int32_t dbu_per_micron)
{
  return static_cast<int32_t>(std::llround(value * static_cast<double>(dbu_per_micron)));
}

}  // namespace unit

}  // namespace ircx
