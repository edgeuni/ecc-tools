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
#include "eccdb/Wire.h"

#include <stdexcept>
#include <utility>

#include "api/detail/DatabaseState.h"
#include "design/DesignDatabase.h"
#include "eccdb/Net.h"

namespace eccdb {

Wire::operator bool() const noexcept
{
  return _state != nullptr && _state->design().routingStorage().contains(_id);
}

detail::DatabaseState& Wire::state() const
{
  if (!*this) {
    throw std::out_of_range("invalid Wire handle");
  }
  return *_state;
}

Net Wire::getNet() const
{
  auto& api = state();
  auto& db = api.design();
  return Net{api, db.routingStorage().wire(_id).net};
}

DesignWireStatus Wire::getStatus() const
{
  return state().design().routingStorage().wire(_id).status;
}

std::string_view Wire::getShieldNet() const
{
  return state().design().routingStorage().wire(_id).shield_net;
}

std::size_t Wire::getPathCount() const
{
  return state().design().routingStorage().pathCount(_id);
}

DesignWirePathView Wire::getPath(std::size_t index) const
{
  return state().design().routingStorage().path(_id, index);
}

void Wire::replace(DesignWireRoutingInput routing, DesignWireStatus status, std::string shield_net)
{
  auto& db = state().design();
  const auto net = db.routingStorage().wire(_id).net;
  db.routingStorage().updateWire(
      _id, DesignWire{.net = net, .status = status, .shield_net = std::move(shield_net)}, std::move(routing));
}

bool Wire::destroy()
{
  if (!*this || !_state->design().routingStorage().destroyWire(_id)) {
    return false;
  }
  _state = nullptr;
  _id = {};
  return true;
}

}  // namespace eccdb
