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

#include "STAHeader.hpp"

namespace ista {

class TimingPoint
{
 public:
  TimingPoint() = default;
  ~TimingPoint() = default;
  // getter
  double get_arrival() const { return _arrival; }
  double get_required() const { return _required; }
  double get_slack() const { return _slack; }
  std::string& get_predecessor() { return _predecessor; }
  // const getter
  const std::string& get_predecessor() const { return _predecessor; }
  // setter
  void set_arrival(const double arrival) { _arrival = arrival; }
  void set_required(const double required) { _required = required; }
  void set_slack(const double slack) { _slack = slack; }
  void set_predecessor(const std::string& predecessor) { _predecessor = predecessor; }
  // function

 private:
  double _arrival = -std::numeric_limits<double>::infinity();
  double _required = std::numeric_limits<double>::infinity();
  double _slack = 0.0;
  std::string _predecessor;
};

}  // namespace ista
