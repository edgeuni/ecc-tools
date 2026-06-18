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
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include <string>
#include <vector>

#include "TimingModel.hpp"

namespace ista {

#define STAGP (ista::GraphPropagator::getInst())

class GraphPropagator
{
 public:
  static void initInst();
  static GraphPropagator& getInst();
  static void destroyInst();
  // function
  bool build();

 private:
  // self
  static GraphPropagator* _gp_instance;

  GraphPropagator() = default;
  GraphPropagator(const GraphPropagator& other) = delete;
  GraphPropagator(GraphPropagator&& other) = delete;
  ~GraphPropagator() = default;
  GraphPropagator& operator=(const GraphPropagator& other) = delete;
  GraphPropagator& operator=(GraphPropagator&& other) = delete;
  // function
  std::vector<std::string> propagateArrival(TimingModel& timing_model);
  bool isFinite(double value) const;
  void propagateRequired(TimingModel& timing_model, const std::vector<std::string>& timing_order);
  double resolveRequiredTime(const TimingModel& timing_model) const;
};

}  // namespace ista
