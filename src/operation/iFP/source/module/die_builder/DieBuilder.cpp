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
#include "DieBuilder.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"

namespace ifp {

// public

void DieBuilder::initInst()
{
  if (_db_instance == nullptr) {
    _db_instance = new DieBuilder();
  }
}

DieBuilder& DieBuilder::getInst()
{
  if (_db_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_db_instance;
}

void DieBuilder::destroyInst()
{
  if (_db_instance != nullptr) {
    delete _db_instance;
    _db_instance = nullptr;
  }
}

// function

void DieBuilder::build()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

DieBuilder* DieBuilder::_db_instance = nullptr;

}  // namespace ifp
