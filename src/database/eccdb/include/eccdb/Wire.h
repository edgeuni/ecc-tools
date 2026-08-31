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

#include <cstddef>
#include <string>
#include <string_view>

#include "eccdb/Types.h"

namespace eccdb {

class Database;
class Net;
namespace detail {
class DatabaseState;
}

class Wire
{
 public:
  Wire() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept;
  [[nodiscard]] WireId getId() const noexcept { return _id; }
  [[nodiscard]] Net getNet() const;
  [[nodiscard]] DesignWireStatus getStatus() const;
  [[nodiscard]] std::string_view getShieldNet() const;
  [[nodiscard]] std::size_t getPathCount() const;
  [[nodiscard]] DesignWirePathView getPath(std::size_t index) const;

  void replace(DesignWireRoutingInput routing,
               DesignWireStatus status = DesignWireStatus::kRouted,
               std::string shield_net = {});
  [[nodiscard]] bool destroy();

 private:
  friend class Database;
  friend class Net;

  Wire(detail::DatabaseState& state, WireId id) noexcept : _state(&state), _id(id) {}
  [[nodiscard]] detail::DatabaseState& state() const;

  detail::DatabaseState* _state = nullptr;
  WireId _id;
};

}  // namespace eccdb
