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
#include "eccdb/Net.h"

#include <stdexcept>
#include <utility>

#include "api/detail/DatabaseState.h"
#include "design/DesignDatabase.h"
#include "eccdb/Pin.h"
#include "eccdb/Wire.h"

namespace eccdb {

Net::operator bool() const noexcept
{
  return _state != nullptr && _state->design().netlistStorage().contains(_id);
}

detail::DatabaseState& Net::state() const
{
  if (!*this) {
    throw std::out_of_range("invalid Net handle");
  }
  return *_state;
}

DesignNet Net::value() const
{
  return state().design().netlistStorage().net(_id);
}

std::string_view Net::getName() const
{
  return state().design().netlistStorage().net(_id).name;
}

bool Net::isSpecial() const
{
  return state().design().netlistStorage().isSpecialNet(_id);
}

DesignSignalUse Net::getUse() const
{
  return state().design().netlistStorage().net(_id).use;
}

DesignNetSource Net::getSource() const
{
  return state().design().netlistStorage().net(_id).source;
}

bool Net::hasWeight() const
{
  return (state().design().netlistStorage().net(_id).flags & DesignNetFlag::kHasWeight) != 0u;
}

int32_t Net::getWeight() const
{
  return state().design().netlistStorage().net(_id).weight;
}

TechNonDefaultRuleId Net::getTechNonDefaultRule() const
{
  return state().design().netlistStorage().net(_id).non_default_rule;
}

DesignNonDefaultRuleId Net::getDesignNonDefaultRule() const
{
  return state().design().netlistStorage().net(_id).design_non_default_rule;
}

const DesignNetOptions* Net::getOptions() const
{
  return state().design().netlistStorage().netOptions(_id);
}

std::vector<InstancePin> Net::getInstancePins() const
{
  auto& api = state();
  auto& db = api.design();
  std::vector<InstancePin> result;
  for (const auto id : db.netlistStorage().instancePins(_id)) {
    result.push_back(InstancePin{api, id});
  }
  return result;
}

std::vector<IoPin> Net::getIoPins() const
{
  auto& api = state();
  auto& db = api.design();
  std::vector<IoPin> result;
  for (const auto id : db.netlistStorage().ioPins(_id)) {
    result.push_back(IoPin{api, id});
  }
  return result;
}

std::vector<Wire> Net::getWires() const
{
  auto& api = state();
  auto& db = api.design();
  std::vector<Wire> result;
  for (const auto id : db.routingStorage().wireIds(_id)) {
    result.push_back(Wire{api, id});
  }
  return result;
}

void Net::rename(std::string name)
{
  auto net = value();
  net.name = std::move(name);
  replace(std::move(net));
}

void Net::setUse(DesignSignalUse use)
{
  auto net = value();
  net.use = use;
  replace(std::move(net));
}

void Net::setSource(DesignNetSource source)
{
  auto net = value();
  net.source = source;
  replace(std::move(net));
}

void Net::setWeight(int32_t weight)
{
  auto net = value();
  net.flags |= DesignNetFlag::kHasWeight;
  net.weight = weight;
  replace(std::move(net));
}

void Net::clearWeight()
{
  auto net = value();
  net.flags &= ~DesignNetFlag::kHasWeight;
  net.weight = 0;
  replace(std::move(net));
}

void Net::setTechNonDefaultRule(TechNonDefaultRuleId rule)
{
  auto net = value();
  net.flags |= DesignNetFlag::kHasNonDefaultRule;
  net.non_default_rule = rule;
  net.design_non_default_rule = {};
  replace(std::move(net));
}

void Net::setDesignNonDefaultRule(DesignNonDefaultRuleId rule)
{
  auto net = value();
  net.flags |= DesignNetFlag::kHasNonDefaultRule;
  net.non_default_rule = {};
  net.design_non_default_rule = rule;
  replace(std::move(net));
}

void Net::clearNonDefaultRule()
{
  auto net = value();
  net.flags &= ~DesignNetFlag::kHasNonDefaultRule;
  net.non_default_rule = {};
  net.design_non_default_rule = {};
  replace(std::move(net));
}

void Net::setOptions(DesignNetOptions options)
{
  state().design().netlistStorage().setNetOptions(_id, std::move(options));
}

void Net::replace(DesignNet value)
{
  state().design().netlistStorage().updateNet(_id, std::move(value));
}

Wire Net::createWire(DesignWireRoutingInput routing, DesignWireStatus status, std::string shield_net)
{
  auto& api = state();
  auto& db = api.design();
  const auto id = db.routingStorage().createWire(
      DesignWire{.net = _id, .status = status, .shield_net = std::move(shield_net)}, std::move(routing));
  return Wire{api, id};
}

bool Net::destroy()
{
  if (!*this || !_state->design().netlistStorage().destroyNet(_id)) {
    return false;
  }
  _state = nullptr;
  _id = {};
  return true;
}

}  // namespace eccdb
