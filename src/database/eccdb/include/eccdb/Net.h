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

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "eccdb/Types.h"

namespace eccdb {

class Database;
class InstancePin;
class IoPin;
class Wire;
namespace detail {
class DatabaseState;
}

class Net
{
 public:
  Net() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept;
  [[nodiscard]] NetId getId() const noexcept { return _id; }
  [[nodiscard]] std::string_view getName() const;
  [[nodiscard]] bool isSpecial() const;
  [[nodiscard]] DesignSignalUse getUse() const;
  [[nodiscard]] DesignNetSource getSource() const;
  [[nodiscard]] bool hasWeight() const;
  [[nodiscard]] int32_t getWeight() const;
  [[nodiscard]] TechNonDefaultRuleId getTechNonDefaultRule() const;
  [[nodiscard]] DesignNonDefaultRuleId getDesignNonDefaultRule() const;
  [[nodiscard]] const DesignNetOptions* getOptions() const;
  [[nodiscard]] std::vector<InstancePin> getInstancePins() const;
  [[nodiscard]] std::vector<IoPin> getIoPins() const;
  [[nodiscard]] std::vector<Wire> getWires() const;

  void rename(std::string name);
  void setUse(DesignSignalUse use);
  void setSource(DesignNetSource source);
  void setWeight(int32_t weight);
  void clearWeight();
  void setTechNonDefaultRule(TechNonDefaultRuleId rule);
  void setDesignNonDefaultRule(DesignNonDefaultRuleId rule);
  void clearNonDefaultRule();
  void setOptions(DesignNetOptions options);
  void replace(DesignNet value);
  [[nodiscard]] Wire createWire(DesignWireRoutingInput routing,
                                DesignWireStatus status = DesignWireStatus::kRouted,
                                std::string shield_net = {});
  [[nodiscard]] bool destroy();

 private:
  friend class Database;
  friend class InstancePin;
  friend class IoPin;
  friend class Wire;

  Net(detail::DatabaseState& state, NetId id) noexcept : _state(&state), _id(id) {}
  [[nodiscard]] detail::DatabaseState& state() const;
  [[nodiscard]] DesignNet value() const;

  detail::DatabaseState* _state = nullptr;
  NetId _id;
};

}  // namespace eccdb
