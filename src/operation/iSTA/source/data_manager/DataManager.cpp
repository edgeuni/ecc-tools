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
#include "DataManager.hpp"

#include "Logger.hpp"
#include "Monitor.hpp"
#include "STAInterface.hpp"
#include "idm.h"

namespace ista {

// public

void DataManager::initInst()
{
  if (_data_manager_instance == nullptr) {
    _data_manager_instance = new DataManager();
  }
}

DataManager& DataManager::getInst()
{
  if (_data_manager_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_data_manager_instance;
}

void DataManager::destroyInst()
{
  if (_data_manager_instance != nullptr) {
    delete _data_manager_instance;
    _data_manager_instance = nullptr;
  }
}

// function

void DataManager::input(std::map<std::string, std::any>& config_map)
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  STALOG.info(Loc::current(), "Input iSTA config: option_num=", config_map.size());
  if (!STAI.inputIDB(dmInst->get_idb_design())) {
    STALOG.warn(Loc::current(), "Input IDB to iSTA data manager failed.");
  }

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void DataManager::output()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  const Summary& summary = _timing_model.summary;
  STALOG.info(Loc::current(), "Output iSTA summary: design=", _timing_model.design_name, " instances=", summary.instance_num,
              " ports=", summary.port_num, " pins=", summary.pin_num, " nets=", summary.net_num, " arcs=", summary.arc_num,
              " worst_slack=", summary.worst_slack, " worst_endpoint=", summary.worst_endpoint);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void DataManager::reset()
{
  _timing_model.reset();
}

// private

DataManager* DataManager::_data_manager_instance = nullptr;

}  // namespace ista
