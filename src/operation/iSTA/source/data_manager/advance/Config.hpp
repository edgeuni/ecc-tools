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
  std::string& get_temp_directory_path() { return _temp_directory_path; }
  std::string& get_log_file_path() { return _log_file_path; }
  std::string& get_dm_temp_directory_path() { return _dm_temp_directory_path; }
  std::string& get_gb_temp_directory_path() { return _gb_temp_directory_path; }
  std::string& get_gp_temp_directory_path() { return _gp_temp_directory_path; }
  std::string& get_ta_temp_directory_path() { return _ta_temp_directory_path; }
  // const getter
  const std::string& get_temp_directory_path() const { return _temp_directory_path; }
  const std::string& get_log_file_path() const { return _log_file_path; }
  const std::string& get_dm_temp_directory_path() const { return _dm_temp_directory_path; }
  const std::string& get_gb_temp_directory_path() const { return _gb_temp_directory_path; }
  const std::string& get_gp_temp_directory_path() const { return _gp_temp_directory_path; }
  const std::string& get_ta_temp_directory_path() const { return _ta_temp_directory_path; }
  // setter
  void set_option_num(const std::size_t option_num) { _option_num = option_num; }
  void set_temp_directory_path(const std::string& temp_directory_path) { _temp_directory_path = temp_directory_path; }
  void set_log_file_path(const std::string& log_file_path) { _log_file_path = log_file_path; }
  void set_dm_temp_directory_path(const std::string& dm_temp_directory_path) { _dm_temp_directory_path = dm_temp_directory_path; }
  void set_gb_temp_directory_path(const std::string& gb_temp_directory_path) { _gb_temp_directory_path = gb_temp_directory_path; }
  void set_gp_temp_directory_path(const std::string& gp_temp_directory_path) { _gp_temp_directory_path = gp_temp_directory_path; }
  void set_ta_temp_directory_path(const std::string& ta_temp_directory_path) { _ta_temp_directory_path = ta_temp_directory_path; }
  // function

 private:
  std::size_t _option_num = 0;
  std::string _temp_directory_path;
  std::string _log_file_path;
  std::string _dm_temp_directory_path;
  std::string _gb_temp_directory_path;
  std::string _gp_temp_directory_path;
  std::string _ta_temp_directory_path;
};

}  // namespace ista
