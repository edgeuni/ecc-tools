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
#include "eccdb/Instance.h"

#include <stdexcept>
#include <utility>

#include "api/detail/DatabaseState.h"
#include "design/DesignDatabase.h"
#include "eccdb/Pin.h"

namespace eccdb {

Instance::operator bool() const noexcept
{
  return _state != nullptr && _state->design().netlistStorage().contains(_id);
}

detail::DatabaseState& Instance::state() const
{
  if (!*this) {
    throw std::out_of_range("invalid Instance handle");
  }
  return *_state;
}

DesignInstance Instance::value() const
{
  return state().design().netlistStorage().instance(_id);
}

std::string_view Instance::getName() const
{
  return state().design().netlistStorage().instance(_id).name;
}

LibraryCellMasterId Instance::getMaster() const
{
  return state().design().netlistStorage().instance(_id).master;
}

Point Instance::getOrigin() const
{
  return state().design().netlistStorage().instance(_id).origin;
}

DesignOrientation Instance::getOrientation() const
{
  return state().design().netlistStorage().instance(_id).orientation;
}

DesignPlacementStatus Instance::getPlacementStatus() const
{
  return state().design().netlistStorage().instance(_id).placement_status;
}

DesignInstanceSource Instance::getSource() const
{
  return state().design().netlistStorage().instance(_id).source;
}

std::vector<InstancePin> Instance::getPins() const
{
  auto& api = state();
  auto& db = api.design();
  std::vector<InstancePin> result;
  for (const auto id : db.netlistStorage().instancePins(_id)) {
    result.push_back(InstancePin{api, id});
  }
  return result;
}

InstancePin Instance::findPin(std::string_view term_name) const
{
  auto& api = state();
  auto& db = api.design();
  const auto id = db.netlistStorage().findInstancePin(_id, term_name);
  return id ? InstancePin{api, id} : InstancePin{};
}

void Instance::rename(std::string name)
{
  auto instance = value();
  instance.name = std::move(name);
  replace(std::move(instance));
}

void Instance::setOrigin(Point origin)
{
  auto instance = value();
  instance.origin = origin;
  replace(std::move(instance));
}

void Instance::setOrientation(DesignOrientation orientation)
{
  auto instance = value();
  instance.orientation = orientation;
  replace(std::move(instance));
}

void Instance::setPlacementStatus(DesignPlacementStatus status)
{
  auto instance = value();
  instance.placement_status = status;
  replace(std::move(instance));
}

void Instance::setSource(DesignInstanceSource source)
{
  auto instance = value();
  instance.source = source;
  replace(std::move(instance));
}

void Instance::replace(DesignInstance value)
{
  state().design().netlistStorage().updateInstance(_id, std::move(value));
}

bool Instance::destroy()
{
  if (!*this || !_state->design().netlistStorage().destroyInstance(_id)) {
    return false;
  }
  _state = nullptr;
  _id = {};
  return true;
}

}  // namespace eccdb
