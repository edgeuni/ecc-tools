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
// WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "Database.hpp"
#include "PRModel.hpp"

namespace ista {

#define STAPR (ista::PowerReporter::getInst())

class PowerReporter
{
 public:
  static void initInst();
  static PowerReporter& getInst();
  static void destroyInst();
  // function
  void report();

 private:
  // self
  static PowerReporter* _pr_instance;

  PowerReporter() = default;
  PowerReporter(const PowerReporter& other) = delete;
  PowerReporter(PowerReporter&& other) = delete;
  ~PowerReporter() = default;
  PowerReporter& operator=(const PowerReporter& other) = delete;
  PowerReporter& operator=(PowerReporter&& other) = delete;
  // function
  PRModel initPRModel();
  void outputPowerReport(PRModel& pr_model);
};

}  // namespace ista
