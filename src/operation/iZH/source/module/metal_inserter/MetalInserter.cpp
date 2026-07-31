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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "MetalInserter.hpp"

namespace izh {

// public

void MetalInserter::initInst()
{
  if (_mi_instance == nullptr) {
    _mi_instance = new MetalInserter();
  }
}

MetalInserter& MetalInserter::getInst()
{
  if (_mi_instance == nullptr) {
    ZHLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_mi_instance;
}

void MetalInserter::destroyInst()
{
  if (_mi_instance != nullptr) {
    delete _mi_instance;
    _mi_instance = nullptr;
  }
}

// function

void MetalInserter::insert(std::map<std::string, std::any> config_map)
{
  Monitor monitor;
  ZHLOG.info(Loc::current(), "Starting...");

  MIModel mi_model = initMIModel(config_map);

  ZHLOG.info(Loc::current(), "ZH insertMetal");
  ZHLOG.info(Loc::current(), "Inserted ", mi_model.get_inserted_metal_num(), " metal fills");

  ZHLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

MIModel MetalInserter::initMIModel(std::map<std::string, std::any>& config_map)
{
  MIModel mi_model;
  if (!config_map.empty()) {
    ZHLOG.warn(Loc::current(), "The insertMetal config has not been consumed yet!");
  }
  return mi_model;
}

MetalInserter* MetalInserter::_mi_instance = nullptr;

}  // namespace izh
