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
#include "PowerAnalyzer.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"

namespace ista {

// public

void PowerAnalyzer::initInst()
{
  if (_pa_instance == nullptr) {
    _pa_instance = new PowerAnalyzer();
  }
}

PowerAnalyzer& PowerAnalyzer::getInst()
{
  if (_pa_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pa_instance;
}

void PowerAnalyzer::destroyInst()
{
  if (_pa_instance != nullptr) {
    delete _pa_instance;
    _pa_instance = nullptr;
  }
}

// function

void PowerAnalyzer::analyze()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  PAModel pa_model = initPAModel();
  analyzePower(pa_model);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

PowerAnalyzer* PowerAnalyzer::_pa_instance = nullptr;

PAModel PowerAnalyzer::initPAModel()
{
  PAModel pa_model;
  return pa_model;
}

void PowerAnalyzer::analyzePower(PAModel& pa_model)
{
  (void) pa_model;
}

}  // namespace ista
