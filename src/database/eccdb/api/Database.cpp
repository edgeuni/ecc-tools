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
#include "eccdb/Database.h"

#include <stdexcept>
#include <utility>

#include "api/detail/DatabaseState.h"
#include "design/DesignDatabase.h"

namespace eccdb {

Database::Database(std::unique_ptr<detail::DatabaseState> state) noexcept : _state(std::move(state)) {}

Database::~Database() = default;
Database::Database(Database&&) noexcept = default;
Database& Database::operator=(Database&&) noexcept = default;

Database Database::open(const Config& config)
{
  return Database{detail::DatabaseState::open(config)};
}

bool Database::supportsLefDef() noexcept
{
#if ECCDB_HAS_LEF_DEF
  return true;
#else
  return false;
#endif
}

detail::DatabaseState& Database::state() const
{
  if (!_state) {
    throw std::logic_error("operation on a moved-from EccDB Database");
  }
  return *_state;
}

const std::vector<ImportDiagnostic>& Database::diagnostics() const noexcept
{
  static const std::vector<ImportDiagnostic> empty;
  return _state ? _state->diagnostics() : empty;
}

void Database::writeDef(const std::filesystem::path& file) const
{
  state().writeDef(file);
}

void Database::writeBinary(const BinaryFiles& files) const
{
  state().writeBinary(files);
}

Net Database::createNet(DesignNet value)
{
  auto& api = state();
  return Net{api, api.design().netlistStorage().createNet(std::move(value))};
}

Net Database::createSpecialNet(DesignNet value)
{
  auto& api = state();
  return Net{api, api.design().netlistStorage().createSpecialNet(std::move(value))};
}

Net Database::getNet(NetId id) const noexcept
{
  if (!_state) return {};
  auto& design = _state->design();
  return design.netlistStorage().contains(id) ? Net{*_state, id} : Net{};
}

Net Database::findNet(std::string_view name) const noexcept
{
  return _state ? getNet(_state->design().netlistStorage().findNet(name)) : Net{};
}

Net Database::findRegularNet(std::string_view name) const noexcept
{
  return _state ? getNet(_state->design().netlistStorage().findRegularNet(name)) : Net{};
}

Net Database::findSpecialNet(std::string_view name) const noexcept
{
  return _state ? getNet(_state->design().netlistStorage().findSpecialNet(name)) : Net{};
}

std::vector<Net> Database::getNets() const
{
  std::vector<Net> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.netlistStorage().nets()) {
    result.push_back(Net{api, id});
  }
  return result;
}

std::vector<Net> Database::getRegularNets() const
{
  std::vector<Net> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.netlistStorage().regularNets()) {
    result.push_back(Net{api, id});
  }
  return result;
}

std::vector<Net> Database::getSpecialNets() const
{
  std::vector<Net> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.netlistStorage().specialNets()) {
    result.push_back(Net{api, id});
  }
  return result;
}

Instance Database::createInstance(DesignInstance value)
{
  auto& api = state();
  return Instance{api, api.design().netlistStorage().createInstance(std::move(value))};
}

Instance Database::getInstance(InstanceId id) const noexcept
{
  if (!_state) return {};
  auto& design = _state->design();
  return design.netlistStorage().contains(id) ? Instance{*_state, id} : Instance{};
}

Instance Database::findInstance(std::string_view name) const noexcept
{
  return _state ? getInstance(_state->design().netlistStorage().findInstance(name)) : Instance{};
}

std::vector<Instance> Database::getInstances() const
{
  std::vector<Instance> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.netlistStorage().instances()) {
    result.push_back(Instance{api, id});
  }
  return result;
}

IoPin Database::createIoPin(DesignIoPin value)
{
  auto& api = state();
  return IoPin{api, api.design().netlistStorage().createIoPin(std::move(value))};
}

IoPin Database::getIoPin(IoPinId id) const noexcept
{
  if (!_state) return {};
  auto& design = _state->design();
  return design.netlistStorage().contains(id) ? IoPin{*_state, id} : IoPin{};
}

IoPin Database::findIoPin(std::string_view name) const noexcept
{
  return _state ? getIoPin(_state->design().netlistStorage().findIoPin(name)) : IoPin{};
}

InstancePin Database::getInstancePin(InstancePinId id) const noexcept
{
  if (!_state) return {};
  auto& design = _state->design();
  return design.netlistStorage().contains(id) ? InstancePin{*_state, id} : InstancePin{};
}

std::vector<IoPin> Database::getIoPins() const
{
  std::vector<IoPin> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.netlistStorage().ioPins()) {
    result.push_back(IoPin{api, id});
  }
  return result;
}

Wire Database::getWire(WireId id) const noexcept
{
  if (!_state) return {};
  auto& design = _state->design();
  return design.routingStorage().contains(id) ? Wire{*_state, id} : Wire{};
}

std::vector<Wire> Database::getWires() const
{
  std::vector<Wire> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.routingStorage().wires()) {
    result.push_back(Wire{api, id});
  }
  return result;
}

Via Database::createVia(DesignVia value)
{
  auto& api = state();
  return Via{api, api.design().routingStorage().createVia(std::move(value))};
}

Via Database::getVia(ViaId id) const noexcept
{
  if (!_state) return {};
  auto& design = _state->design();
  return design.routingStorage().contains(id) ? Via{*_state, id} : Via{};
}

Via Database::findVia(std::string_view name) const noexcept
{
  return _state ? getVia(_state->design().routingStorage().findVia(name)) : Via{};
}

std::vector<Via> Database::getVias() const
{
  std::vector<Via> result;
  auto& api = state();
  auto& design = api.design();
  for (const auto id : design.routingStorage().vias()) {
    result.push_back(Via{api, id});
  }
  return result;
}

}  // namespace eccdb
