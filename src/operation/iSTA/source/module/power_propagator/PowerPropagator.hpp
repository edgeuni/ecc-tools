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
#include "PPModel.hpp"

namespace ista {

#define STAPP (ista::PowerPropagator::getInst())

class PowerPropagator
{
 public:
  static void initInst();
  static PowerPropagator& getInst();
  static void destroyInst();
  // function
  void propagate();

 private:
  // self
  static PowerPropagator* _pp_instance;

  PowerPropagator() = default;
  PowerPropagator(const PowerPropagator& other) = delete;
  PowerPropagator(PowerPropagator&& other) = delete;
  ~PowerPropagator() = default;
  PowerPropagator& operator=(const PowerPropagator& other) = delete;
  PowerPropagator& operator=(PowerPropagator&& other) = delete;
  // function
  PPModel initPPModel();
  void propagateActivity(PPModel& pp_model);
};

}  // namespace ista
