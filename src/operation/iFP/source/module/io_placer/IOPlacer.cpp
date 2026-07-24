// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of the Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "IOPlacer.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"

namespace ifp {

// public

void IOPlacer::initInst()
{
  if (_iop_instance == nullptr) {
    _iop_instance = new IOPlacer();
  }
}

IOPlacer& IOPlacer::getInst()
{
  if (_iop_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_iop_instance;
}

void IOPlacer::destroyInst()
{
  if (_iop_instance != nullptr) {
    delete _iop_instance;
    _iop_instance = nullptr;
  }
}

// function

void IOPlacer::place()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

IOPlacer* IOPlacer::_iop_instance = nullptr;

}  // namespace ifp
