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

#include "eccdb/Types.h"

namespace eccdb {

class Database;
class Instance;
class Net;
namespace detail {
class DatabaseState;
}

class InstancePin
{
 public:
  InstancePin() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept;
  [[nodiscard]] InstancePinId getId() const noexcept { return _id; }
  [[nodiscard]] Instance getInstance() const;
  [[nodiscard]] LibraryMasterTermId getMasterTerm() const;
  [[nodiscard]] Net getNet() const noexcept;
  [[nodiscard]] Net getSpecialNet() const noexcept;

  void connect(Net net);
  void disconnect();
  void disconnect(Net net);

 private:
  friend class Database;
  friend class Instance;
  friend class Net;

  InstancePin(detail::DatabaseState& state, InstancePinId id) noexcept : _state(&state), _id(id) {}
  [[nodiscard]] detail::DatabaseState& state() const;

  detail::DatabaseState* _state = nullptr;
  InstancePinId _id;
};

class IoPin
{
 public:
  IoPin() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept;
  [[nodiscard]] IoPinId getId() const noexcept { return _id; }
  [[nodiscard]] std::string_view getName() const;
  [[nodiscard]] DesignIoPinDirection getDirection() const;
  [[nodiscard]] DesignSignalUse getUse() const;
  [[nodiscard]] Net getNet() const noexcept;
  [[nodiscard]] Net getSpecialNet() const noexcept;

  void rename(std::string name);
  void setDirection(DesignIoPinDirection direction);
  void setUse(DesignSignalUse use);
  void replace(DesignIoPin value);
  void connect(Net net);
  void disconnect();
  void disconnect(Net net);
  [[nodiscard]] bool destroy();

 private:
  friend class Database;
  friend class Net;

  IoPin(detail::DatabaseState& state, IoPinId id) noexcept : _state(&state), _id(id) {}
  [[nodiscard]] detail::DatabaseState& state() const;
  [[nodiscard]] DesignIoPin value() const;

  detail::DatabaseState* _state = nullptr;
  IoPinId _id;
};

}  // namespace eccdb
