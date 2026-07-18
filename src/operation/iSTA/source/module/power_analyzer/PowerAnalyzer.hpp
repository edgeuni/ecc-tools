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
#include "PAModel.hpp"

namespace ista {

#define STAPA (ista::PowerAnalyzer::getInst())

class PowerAnalyzer
{
 public:
  static void initInst();
  static PowerAnalyzer& getInst();
  static void destroyInst();
  // function
  void analyze();

 private:
  // self
  static PowerAnalyzer* _pa_instance;

  PowerAnalyzer() = default;
  PowerAnalyzer(const PowerAnalyzer& other) = delete;
  PowerAnalyzer(PowerAnalyzer&& other) = delete;
  ~PowerAnalyzer() = default;
  PowerAnalyzer& operator=(const PowerAnalyzer& other) = delete;
  PowerAnalyzer& operator=(PowerAnalyzer&& other) = delete;
  // function
  PAModel initPAModel();
  void analyzePower(PAModel& pa_model);
};

}  // namespace ista
