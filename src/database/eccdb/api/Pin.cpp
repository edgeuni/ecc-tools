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
#include "eccdb/Pin.h"

#include <stdexcept>
#include <utility>

#include "api/detail/DatabaseState.h"
#include "design/DesignDatabase.h"
#include "eccdb/Instance.h"
#include "eccdb/Net.h"

namespace eccdb {

InstancePin::operator bool() const noexcept
{
  return _state != nullptr && _state->design().netlistStorage().contains(_id);
}

detail::DatabaseState& InstancePin::state() const
{
  if (!*this) {
    throw std::out_of_range("invalid InstancePin handle");
  }
  return *_state;
}

Instance InstancePin::getInstance() const
{
  auto& api = state();
  auto& db = api.design();
  return Instance{api, db.netlistStorage().instancePin(_id).instance};
}

LibraryMasterTermId InstancePin::getMasterTerm() const
{
  return state().design().netlistStorage().instancePin(_id).master_term;
}

Net InstancePin::getNet() const noexcept
{
  if (!*this) {
    return {};
  }
  const auto id = _state->design().netlistStorage().instancePin(_id).net;
  return id ? Net{*_state, id} : Net{};
}

Net InstancePin::getSpecialNet() const noexcept
{
  if (!*this) {
    return {};
  }
  const auto id = _state->design().netlistStorage().instancePin(_id).special_net;
  return id ? Net{*_state, id} : Net{};
}

void InstancePin::connect(Net net)
{
  auto& api = state();
  auto& db = api.design();
  if (!net || net._state != &api) {
    throw std::invalid_argument("pin and net belong to different designs");
  }
  db.netlistStorage().connect(_id, net._id);
}

void InstancePin::disconnect()
{
  state().design().netlistStorage().disconnect(_id);
}

void InstancePin::disconnect(Net net)
{
  auto& api = state();
  auto& db = api.design();
  if (!net || net._state != &api) {
    throw std::invalid_argument("pin and net belong to different designs");
  }
  db.netlistStorage().disconnect(_id, net._id);
}

IoPin::operator bool() const noexcept
{
  return _state != nullptr && _state->design().netlistStorage().contains(_id);
}

detail::DatabaseState& IoPin::state() const
{
  if (!*this) {
    throw std::out_of_range("invalid IoPin handle");
  }
  return *_state;
}

DesignIoPin IoPin::value() const
{
  return state().design().netlistStorage().ioPin(_id);
}

std::string_view IoPin::getName() const
{
  return state().design().netlistStorage().ioPin(_id).name;
}

DesignIoPinDirection IoPin::getDirection() const
{
  return state().design().netlistStorage().ioPin(_id).direction;
}

DesignSignalUse IoPin::getUse() const
{
  return state().design().netlistStorage().ioPin(_id).use;
}

Net IoPin::getNet() const noexcept
{
  if (!*this) {
    return {};
  }
  const auto id = _state->design().netlistStorage().ioPin(_id).net;
  return id ? Net{*_state, id} : Net{};
}

Net IoPin::getSpecialNet() const noexcept
{
  if (!*this) {
    return {};
  }
  const auto id = _state->design().netlistStorage().ioPin(_id).special_net;
  return id ? Net{*_state, id} : Net{};
}

void IoPin::rename(std::string name)
{
  auto pin = value();
  pin.name = std::move(name);
  replace(std::move(pin));
}

void IoPin::setDirection(DesignIoPinDirection direction)
{
  auto pin = value();
  pin.direction = direction;
  replace(std::move(pin));
}

void IoPin::setUse(DesignSignalUse use)
{
  auto pin = value();
  pin.use = use;
  replace(std::move(pin));
}

void IoPin::replace(DesignIoPin value)
{
  state().design().netlistStorage().updateIoPin(_id, std::move(value));
}

void IoPin::connect(Net net)
{
  auto& api = state();
  auto& db = api.design();
  if (!net || net._state != &api) {
    throw std::invalid_argument("pin and net belong to different designs");
  }
  db.netlistStorage().connect(_id, net._id);
}

void IoPin::disconnect()
{
  state().design().netlistStorage().disconnect(_id);
}

void IoPin::disconnect(Net net)
{
  auto& api = state();
  auto& db = api.design();
  if (!net || net._state != &api) {
    throw std::invalid_argument("pin and net belong to different designs");
  }
  db.netlistStorage().disconnect(_id, net._id);
}

bool IoPin::destroy()
{
  if (!*this || !_state->design().netlistStorage().destroyIoPin(_id)) {
    return false;
  }
  _state = nullptr;
  _id = {};
  return true;
}

}  // namespace eccdb
