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
#include "PowerReporter.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void PowerReporter::initInst()
{
  if (_pr_instance == nullptr) {
    _pr_instance = new PowerReporter();
  }
}

PowerReporter& PowerReporter::getInst()
{
  if (_pr_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pr_instance;
}

void PowerReporter::destroyInst()
{
  if (_pr_instance != nullptr) {
    delete _pr_instance;
    _pr_instance = nullptr;
  }
}

// function

void PowerReporter::report()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  PRModel pr_model = initPRModel();
  outputPowerReport(pr_model);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

PowerReporter* PowerReporter::_pr_instance = nullptr;

PRModel PowerReporter::initPRModel()
{
  PRModel pr_model;
  return pr_model;
}

void PowerReporter::outputPowerReport(PRModel& pr_model)
{
  (void) pr_model;
}

}  // namespace ista
