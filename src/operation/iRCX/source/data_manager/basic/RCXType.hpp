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

using U8 = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

using I8 = std::int8_t;
using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;

using F32 = float;
using F64 = double;

using Size = std::size_t;

using Dbu = I32;
using Micron = F64;

inline constexpr Dbu kMaxDbu = std::numeric_limits<Dbu>::max();
inline constexpr Size kMaxSize = std::numeric_limits<Size>::max();
inline constexpr Micron kMaxMicron = std::numeric_limits<Micron>::max();
inline constexpr Size kSpecialNetId = kMaxSize - 1;

using BgPointI = boost::geometry::model::point<Dbu, 2, boost::geometry::cs::cartesian>;
using BgBoxI = boost::geometry::model::box<BgPointI>;
using BgPolygonI = boost::geometry::model::polygon<BgPointI>;

using GtlPointI = boost::polygon::point_data<Dbu>;
using GtlRectI = boost::polygon::rectangle_data<Dbu>;
using GtlPolyI = boost::polygon::polygon_90_data<Dbu>;
using GtlPolySetI = boost::polygon::polygon_90_set_data<Dbu>;

using GtlPointF = boost::polygon::point_data<F64>;
using GtlRectF = boost::polygon::rectangle_data<F64>;
using GtlPolyF = boost::polygon::polygon_90_data<F64>;
using GtlPolySetF = boost::polygon::polygon_90_set_data<F64>;

using LayerShape = std::pair<Size, GtlRectI>;

namespace unit {

inline Micron to_micron(Dbu value, Dbu dbu_per_micron)
{
  return static_cast<Micron>(value) / static_cast<Micron>(dbu_per_micron);
}

inline Dbu to_dbu(Micron value, Dbu dbu_per_micron)
{
  return static_cast<Dbu>(std::llround(value * static_cast<Micron>(dbu_per_micron)));
}

}  // namespace unit

}  // namespace ircx
