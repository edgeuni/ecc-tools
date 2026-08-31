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

#include <string>
#include <string_view>
#include <vector>

#include "eccdb/Types.h"

namespace eccdb {

class Database;
class InstancePin;
namespace detail {
class DatabaseState;
}

class Instance
{
 public:
  Instance() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept;
  [[nodiscard]] InstanceId getId() const noexcept { return _id; }
  [[nodiscard]] std::string_view getName() const;
  [[nodiscard]] LibraryCellMasterId getMaster() const;
  [[nodiscard]] Point getOrigin() const;
  [[nodiscard]] DesignOrientation getOrientation() const;
  [[nodiscard]] DesignPlacementStatus getPlacementStatus() const;
  [[nodiscard]] DesignInstanceSource getSource() const;
  [[nodiscard]] std::vector<InstancePin> getPins() const;
  [[nodiscard]] InstancePin findPin(std::string_view term_name) const;

  void rename(std::string name);
  void setOrigin(Point origin);
  void setOrientation(DesignOrientation orientation);
  void setPlacementStatus(DesignPlacementStatus status);
  void setSource(DesignInstanceSource source);
  void replace(DesignInstance value);
  [[nodiscard]] bool destroy();

 private:
  friend class Database;
  friend class InstancePin;

  Instance(detail::DatabaseState& state, InstanceId id) noexcept : _state(&state), _id(id) {}
  [[nodiscard]] detail::DatabaseState& state() const;
  [[nodiscard]] DesignInstance value() const;

  detail::DatabaseState* _state = nullptr;
  InstanceId _id;
};

}  // namespace eccdb
