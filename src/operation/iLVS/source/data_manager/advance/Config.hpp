// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#pragma once

#include "LVSHeader.hpp"

namespace ilvs {

class Config
{
 public:
  Config() = default;
  ~Config() = default;
  /////////////////////////////////////////////
  // **********       LVS        ********** //
  std::string temp_directory_path;  // required
  int32_t thread_number;            // optional
  /////////////////////////////////////////////
  // **********       LVS        ********** //
  std::string log_file_path;  // building
  // **********   DataManager    ********** //
  std::string dm_temp_directory_path;  // building
  // ******** NetlistExtractor   ********** //
  std::string ne_temp_directory_path;  // building
  // **********   LVSChecker     ********** //
  std::string lc_temp_directory_path;  // building
  // **********   LVSReporter    ********** //
  std::string lr_temp_directory_path;  // building
  /////////////////////////////////////////////
};

}  // namespace ilvs
