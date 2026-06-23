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
#include "ConstraintManager.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void ConstraintManager::initInst()
{
  if (_cm_instance == nullptr) {
    _cm_instance = new ConstraintManager();
  }
}

ConstraintManager& ConstraintManager::getInst()
{
  if (_cm_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_cm_instance;
}

void ConstraintManager::destroyInst()
{
  if (_cm_instance != nullptr) {
    delete _cm_instance;
    _cm_instance = nullptr;
  }
}

// function

bool ConstraintManager::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  buildConstraint(database);

  STALOG.info(Loc::current(), "Build iSTA constraint: clocks=", database.get_timing_constraint().get_clock_map().size(),
              " port_constraints=", database.get_timing_constraint().get_port_constraint_map().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

ConstraintManager* ConstraintManager::_cm_instance = nullptr;

void ConstraintManager::buildConstraint(Database& database)
{
}

}  // namespace ista
