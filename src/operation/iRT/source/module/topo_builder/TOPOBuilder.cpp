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
#include "TOPOBuilder.hpp"
#include "TBTask.hpp"

#include "RTInterface.hpp"

namespace irt {

// public

void TOPOBuilder::initInst()
{
  if (_tb_instance == nullptr) {
    _tb_instance = new TOPOBuilder();
  }
}

TOPOBuilder& TOPOBuilder::getInst()
{
  if (_tb_instance == nullptr) {
    RTLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tb_instance;
}

void TOPOBuilder::destroyInst()
{
  if (_tb_instance != nullptr) {
    delete _tb_instance;
    _tb_instance = nullptr;
  }
}

// function

void TOPOBuilder::init()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  RTI.initFlute();

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

std::vector<Segment<PlanarCoord>> TOPOBuilder::getPlanarTopoList(TBTask& tb_task)
{
  return RTI.getPlanarTopoList(tb_task.get_planar_coord_list());
}

void TOPOBuilder::destroy()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  RTI.destroyFlute();

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TOPOBuilder* TOPOBuilder::_tb_instance = nullptr;

}  // namespace irt
