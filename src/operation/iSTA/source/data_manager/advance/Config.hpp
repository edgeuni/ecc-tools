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

class Config
{
 public:
  Config() = default;
  ~Config() = default;
  // getter
  std::size_t get_option_num() const { return _option_num; }
  std::string& get_report_directory() { return _report_directory; }
  // const getter
  const std::string& get_report_directory() const { return _report_directory; }
  // setter
  void set_option_num(const std::size_t option_num) { _option_num = option_num; }
  void set_report_directory(const std::string& report_directory) { _report_directory = report_directory; }
  // function

 private:
  std::size_t _option_num = 0;
  std::string _report_directory;
};

}  // namespace ista
