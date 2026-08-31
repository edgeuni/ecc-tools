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
#include "eccdb/Via.h"

#include <stdexcept>
#include <utility>

#include "api/detail/DatabaseState.h"
#include "design/DesignDatabase.h"

namespace eccdb {

Via::operator bool() const noexcept
{
  return _state != nullptr && _state->design().routingStorage().contains(_id);
}

detail::DatabaseState& Via::state() const
{
  if (!*this) {
    throw std::out_of_range("invalid Via handle");
  }
  return *_state;
}

std::string_view Via::getName() const
{
  return state().design().routingStorage().via(_id).name;
}

bool Via::isGenerated() const
{
  return (state().design().routingStorage().via(_id).flags & DesignViaFlag::kGenerated) != 0u;
}

std::string_view Via::getPatternName() const
{
  return state().design().routingStorage().via(_id).pattern_name;
}

std::span<const DesignViaRectangle> Via::getRectangles() const
{
  return state().design().routingStorage().via(_id).rectangles;
}

std::span<const DesignViaPolygon> Via::getPolygons() const
{
  return state().design().routingStorage().via(_id).polygons;
}

const DesignGeneratedVia* Via::getGenerated() const
{
  const auto& via = state().design().routingStorage().via(_id);
  return (via.flags & DesignViaFlag::kGenerated) != 0u ? &via.generated : nullptr;
}

void Via::replace(DesignVia value)
{
  state().design().routingStorage().updateVia(_id, std::move(value));
}

bool Via::destroy()
{
  if (!*this || !_state->design().routingStorage().destroyVia(_id)) {
    return false;
  }
  _state = nullptr;
  _id = {};
  return true;
}

}  // namespace eccdb
