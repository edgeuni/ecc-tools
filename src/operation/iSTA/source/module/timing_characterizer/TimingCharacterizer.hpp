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

#include "Database.hpp"

namespace ista {

#define STATC (ista::TimingCharacterizer::getInst())

class TimingCharacterizer
{
 public:
  static void initInst();
  static TimingCharacterizer& getInst();
  static void destroyInst();
  // function
  void characterize();

 private:
  // self
  static TimingCharacterizer* _tc_instance;

  TimingCharacterizer() = default;
  TimingCharacterizer(const TimingCharacterizer& other) = delete;
  TimingCharacterizer(TimingCharacterizer&& other) = delete;
  ~TimingCharacterizer() = default;
  TimingCharacterizer& operator=(const TimingCharacterizer& other) = delete;
  TimingCharacterizer& operator=(TimingCharacterizer&& other) = delete;
  // function
  void outputLibFileList(Database& database);
  void outputLibFile(std::string& design_name, std::string analysis_type);
  std::string getLibFilePath(std::string& design_name, std::string analysis_type);
};

}  // namespace ista
