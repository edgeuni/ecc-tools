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

#define STACM (ista::ConstraintManager::getInst())

class ConstraintManager
{
 public:
  static void initInst();
  static ConstraintManager& getInst();
  static void destroyInst();
  // function
  bool build();

 private:
  // self
  static ConstraintManager* _cm_instance;

  ConstraintManager() = default;
  ConstraintManager(const ConstraintManager& other) = delete;
  ConstraintManager(ConstraintManager&& other) = delete;
  ~ConstraintManager() = default;
  ConstraintManager& operator=(const ConstraintManager& other) = delete;
  ConstraintManager& operator=(ConstraintManager&& other) = delete;
  // function
  void buildConstraint(Database& database);
};

}  // namespace ista
