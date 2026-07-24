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
#include "MacroPlacer.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"

namespace ifp {

// public

void MacroPlacer::initInst()
{
  if (_mp_instance == nullptr) {
    _mp_instance = new MacroPlacer();
  }
}

MacroPlacer& MacroPlacer::getInst()
{
  if (_mp_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_mp_instance;
}

void MacroPlacer::destroyInst()
{
  if (_mp_instance != nullptr) {
    delete _mp_instance;
    _mp_instance = nullptr;
  }
}

// function

void MacroPlacer::place()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

MacroPlacer* MacroPlacer::_mp_instance = nullptr;

}  // namespace ifp
