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
#include "FPInterface.hpp"

#include "DataManager.hpp"
#include "DieBuilder.hpp"
#include "IOPlacer.hpp"
#include "Logger.hpp"
#include "MacroPlacer.hpp"
#include "Monitor.hpp"
#include "PDNGenerator.hpp"
#include "PhyPlacer.hpp"
#include "Utility.hpp"

namespace ifp {

FPInterface* FPInterface::_fp_interface_instance = nullptr;

// public

FPInterface& FPInterface::getInst()
{
  if (_fp_interface_instance == nullptr) {
    _fp_interface_instance = new FPInterface();
  }
  return *_fp_interface_instance;
}

void FPInterface::destroyInst()
{
  if (_fp_interface_instance != nullptr) {
    delete _fp_interface_instance;
    _fp_interface_instance = nullptr;
  }
}

#if 1  // 外部调用FP的API

#if 1  // iFP

void FPInterface::initFP(std::map<std::string, std::any> config_map)
{
  Logger::initInst();
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  DataManager::initInst();
  FPDM.input(config_map);

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void FPInterface::runFP()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  DieBuilder::initInst();
  FPDB.build();
  DieBuilder::destroyInst();

  IOPlacer::initInst();
  FPIOP.place();
  IOPlacer::destroyInst();

  MacroPlacer::initInst();
  FPMP.place();
  MacroPlacer::destroyInst();

  PDNGenerator::initInst();
  FPPG.generate();
  PDNGenerator::destroyInst();

  PhyPlacer::initInst();
  FPP.place();
  PhyPlacer::destroyInst();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void FPInterface::destroyFP()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPDM.output();
  DataManager::destroyInst();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  FPLOG.printLogFilePath();
  Logger::destroyInst();
}

#endif

#endif

#if 1  // FP调用外部的API

#if 1  // TopData

#if 1  // input

void FPInterface::input(std::map<std::string, std::any>& config_map)
{
  wrapConfig(config_map);
  wrapDatabase();
}

void FPInterface::wrapConfig(std::map<std::string, std::any>& config_map)
{
  FPDM.getConfig().temp_directory_path = FPUTIL.getConfigValue<std::string>(config_map, "-temp_directory_path", "./fp_temp_directory");
  FPDM.getConfig().thread_number = FPUTIL.getConfigValue<int32_t>(config_map, "-thread_number", 128);
  omp_set_num_threads(std::max(FPDM.getConfig().thread_number, 1));
}

void FPInterface::wrapDatabase()
{
}

#endif

#if 1  // output

void FPInterface::output()
{
}

#endif

#endif

#endif

}  // namespace ifp
