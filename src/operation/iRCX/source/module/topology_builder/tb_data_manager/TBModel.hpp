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

namespace ircx {

class TBModel
{
 public:
  TBModel() = default;
  ~TBModel() = default;
  // getter
  Database* get_database() { return _database; }
  std::string& get_temp_directory_path() { return _temp_directory_path; }
  // setter
  void set_database(Database* database) { _database = database; }
  void set_temp_directory_path(const std::string& temp_directory_path) { _temp_directory_path = temp_directory_path; }
  // function

 private:
  Database* _database = nullptr;
  std::string _temp_directory_path;
};

}  // namespace ircx
