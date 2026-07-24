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
#include "PhyPlacer.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"

namespace ifp {

// public

void PhyPlacer::initInst()
{
  if (_pp_instance == nullptr) {
    _pp_instance = new PhyPlacer();
  }
}

PhyPlacer& PhyPlacer::getInst()
{
  if (_pp_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pp_instance;
}

void PhyPlacer::destroyInst()
{
  if (_pp_instance != nullptr) {
    delete _pp_instance;
    _pp_instance = nullptr;
  }
}

// function

void PhyPlacer::place()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

PhyPlacer* PhyPlacer::_pp_instance = nullptr;

}  // namespace ifp
