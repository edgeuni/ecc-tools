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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "Database.hpp"
#include "RCXHeader.hpp"
#include "SPEFCouplingRef.hpp"
#include "SPEFReportLayer.hpp"

namespace ircx {

class SWModel
{
 public:
  SWModel() = default;
  ~SWModel() = default;
  // getter
  Database* get_database() { return _database; }
  std::string& get_temp_directory_path() { return _temp_directory_path; }
  std::vector<std::vector<SPEFCouplingRef>>& get_net_coupling_ref_list() { return _net_coupling_ref_list; }
  std::vector<SPEFReportLayer>& get_report_layer_list() { return _report_layer_list; }
  std::unordered_map<Size, Size>& get_design_layer_id_to_report_layer_id_map()
  {
    return _design_layer_id_to_report_layer_id_map;
  }
  // setter
  void set_database(Database* database) { _database = database; }
  void set_temp_directory_path(const std::string& temp_directory_path) { _temp_directory_path = temp_directory_path; }
  void set_net_coupling_ref_list(const std::vector<std::vector<SPEFCouplingRef>>& net_coupling_ref_list)
  {
    _net_coupling_ref_list = net_coupling_ref_list;
  }
  void set_report_layer_list(const std::vector<SPEFReportLayer>& report_layer_list) { _report_layer_list = report_layer_list; }
  void set_design_layer_id_to_report_layer_id_map(const std::unordered_map<Size, Size>& design_layer_id_to_report_layer_id_map)
  {
    _design_layer_id_to_report_layer_id_map = design_layer_id_to_report_layer_id_map;
  }
  // function

 private:
  Database* _database = nullptr;
  std::string _temp_directory_path;
  std::vector<std::vector<SPEFCouplingRef>> _net_coupling_ref_list;
  std::vector<SPEFReportLayer> _report_layer_list;
  std::unordered_map<Size, Size> _design_layer_id_to_report_layer_id_map;
};

}  // namespace ircx
