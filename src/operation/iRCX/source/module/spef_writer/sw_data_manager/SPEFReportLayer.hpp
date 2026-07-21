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

#include "RCXHeader.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class SPEFReportLayer
{
 public:
  SPEFReportLayer() = default;
  ~SPEFReportLayer() = default;
  // getter
  size_t get_report_layer_id() const { return _report_layer_id; }
  size_t get_design_layer_id() const { return _design_layer_id; }
  std::string& get_design_layer_name() { return _design_layer_name; }
  std::string& get_process_layer_name() { return _process_layer_name; }
  // setter
  void set_report_layer_id(size_t report_layer_id) { _report_layer_id = report_layer_id; }
  void set_design_layer_id(size_t design_layer_id) { _design_layer_id = design_layer_id; }
  void set_design_layer_name(const std::string& design_layer_name) { _design_layer_name = design_layer_name; }
  void set_process_layer_name(const std::string& process_layer_name) { _process_layer_name = process_layer_name; }
  // function

 private:
  size_t _report_layer_id = SIZE_MAX;
  size_t _design_layer_id = SIZE_MAX;
  std::string _design_layer_name;
  std::string _process_layer_name;
};

}  // namespace ircx
