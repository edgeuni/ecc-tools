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

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "eccdb/Config.h"
#include "eccdb/Instance.h"
#include "eccdb/Net.h"
#include "eccdb/Pin.h"
#include "eccdb/Types.h"
#include "eccdb/Via.h"
#include "eccdb/Wire.h"

namespace eccdb {

namespace detail {
class DatabaseState;
class DatabaseTestAccess;
}  // namespace detail

struct ImportDiagnostic
{
  std::string source;
  std::string statement;
  std::size_t occurrence_count = 0;
};

// Owning public API entry point. Its implementation owns the technology, library and
// physical-design databases; public object handles borrow that shared state.
class Database
{
 public:
  [[nodiscard]] static Database open(const Config& config);
  [[nodiscard]] static bool supportsLefDef() noexcept;

  ~Database();
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  Database(Database&&) noexcept;
  Database& operator=(Database&&) noexcept;

  [[nodiscard]] const std::vector<ImportDiagnostic>& diagnostics() const noexcept;
  void writeDef(const std::filesystem::path& file) const;
  void writeBinary(const BinaryFiles& files) const;

  [[nodiscard]] Net createNet(DesignNet value);
  [[nodiscard]] Net createSpecialNet(DesignNet value);
  [[nodiscard]] Net getNet(NetId id) const noexcept;
  [[nodiscard]] Net findNet(std::string_view name) const noexcept;
  [[nodiscard]] Net findRegularNet(std::string_view name) const noexcept;
  [[nodiscard]] Net findSpecialNet(std::string_view name) const noexcept;
  [[nodiscard]] std::vector<Net> getNets() const;
  [[nodiscard]] std::vector<Net> getRegularNets() const;
  [[nodiscard]] std::vector<Net> getSpecialNets() const;

  [[nodiscard]] Instance createInstance(DesignInstance value);
  [[nodiscard]] Instance getInstance(InstanceId id) const noexcept;
  [[nodiscard]] Instance findInstance(std::string_view name) const noexcept;
  [[nodiscard]] std::vector<Instance> getInstances() const;

  [[nodiscard]] IoPin createIoPin(DesignIoPin value);
  [[nodiscard]] IoPin getIoPin(IoPinId id) const noexcept;
  [[nodiscard]] IoPin findIoPin(std::string_view name) const noexcept;
  [[nodiscard]] InstancePin getInstancePin(InstancePinId id) const noexcept;
  [[nodiscard]] std::vector<IoPin> getIoPins() const;

  [[nodiscard]] Wire getWire(WireId id) const noexcept;
  [[nodiscard]] std::vector<Wire> getWires() const;

  [[nodiscard]] Via createVia(DesignVia value);
  [[nodiscard]] Via getVia(ViaId id) const noexcept;
  [[nodiscard]] Via findVia(std::string_view name) const noexcept;
  [[nodiscard]] std::vector<Via> getVias() const;

 private:
  friend class detail::DatabaseTestAccess;

  explicit Database(std::unique_ptr<detail::DatabaseState> state) noexcept;
  [[nodiscard]] detail::DatabaseState& state() const;

  std::unique_ptr<detail::DatabaseState> _state;
};

}  // namespace eccdb
