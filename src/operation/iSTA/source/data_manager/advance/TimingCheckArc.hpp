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

class TimingCheckArc
{
 public:
  TimingCheckArc() = default;
  ~TimingCheckArc() = default;
  // getter
  std::string& get_clock_port() { return _clock_port; }
  std::string& get_data_port() { return _data_port; }
  double get_setup_time() const { return _setup_time; }
  // setter
  void set_clock_port(const std::string& clock_port) { _clock_port = clock_port; }
  void set_data_port(const std::string& data_port) { _data_port = data_port; }
  void set_setup_time(const double setup_time) { _setup_time = setup_time; }
  // function

 private:
  std::string _clock_port;
  std::string _data_port;
  double _setup_time = 0.0;
};

}  // namespace ista
