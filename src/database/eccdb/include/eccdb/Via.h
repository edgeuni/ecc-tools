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

#include <span>
#include <string_view>

#include "eccdb/Types.h"

namespace eccdb {

class Database;
namespace detail {
class DatabaseState;
}

class Via
{
 public:
  Via() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept;
  [[nodiscard]] ViaId getId() const noexcept { return _id; }
  [[nodiscard]] std::string_view getName() const;
  [[nodiscard]] bool isGenerated() const;
  [[nodiscard]] std::string_view getPatternName() const;
  [[nodiscard]] std::span<const DesignViaRectangle> getRectangles() const;
  [[nodiscard]] std::span<const DesignViaPolygon> getPolygons() const;
  [[nodiscard]] const DesignGeneratedVia* getGenerated() const;

  void replace(DesignVia value);
  [[nodiscard]] bool destroy();

 private:
  friend class Database;

  Via(detail::DatabaseState& state, ViaId id) noexcept : _state(&state), _id(id) {}
  [[nodiscard]] detail::DatabaseState& state() const;

  detail::DatabaseState* _state = nullptr;
  ViaId _id;
};

}  // namespace eccdb
