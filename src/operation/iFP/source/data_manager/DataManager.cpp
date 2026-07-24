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
#include "DataManager.hpp"

#include "FPInterface.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ifp {

// public

void DataManager::initInst()
{
  if (_dm_instance == nullptr) {
    _dm_instance = new DataManager();
  }
}

DataManager& DataManager::getInst()
{
  if (_dm_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_dm_instance;
}

void DataManager::destroyInst()
{
  if (_dm_instance != nullptr) {
    delete _dm_instance;
    _dm_instance = nullptr;
  }
}

void DataManager::input(std::map<std::string, std::any>& config_map)
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPI.input(config_map);
  buildConfig();
  buildDatabase();
  printConfig();
  printDatabase();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void DataManager::output()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  FPI.output();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

DataManager* DataManager::_dm_instance = nullptr;

#if 1  // build

void DataManager::buildConfig()
{
  _config.temp_directory_path = std::filesystem::absolute(_config.temp_directory_path);
  _config.temp_directory_path += "/";
  _config.log_file_path = _config.temp_directory_path + "fp.log";
  _config.dm_temp_directory_path = _config.temp_directory_path + "data_manager/";
  _config.db_temp_directory_path = _config.temp_directory_path + "die_builder/";
  _config.iop_temp_directory_path = _config.temp_directory_path + "io_placer/";
  _config.mp_temp_directory_path = _config.temp_directory_path + "macro_placer/";
  _config.pg_temp_directory_path = _config.temp_directory_path + "pdn_generator/";
  _config.pp_temp_directory_path = _config.temp_directory_path + "phy_placer/";

  FPUTIL.removeDir(_config.temp_directory_path);
  FPUTIL.createDir(_config.temp_directory_path);
  FPUTIL.createDirByFile(_config.log_file_path);
  FPUTIL.createDir(_config.dm_temp_directory_path);
  FPUTIL.createDir(_config.db_temp_directory_path);
  FPUTIL.createDir(_config.iop_temp_directory_path);
  FPUTIL.createDir(_config.mp_temp_directory_path);
  FPUTIL.createDir(_config.pg_temp_directory_path);
  FPUTIL.createDir(_config.pp_temp_directory_path);
  FPLOG.openLogFileStream(_config.log_file_path);
}

void DataManager::buildDatabase()
{
}

#endif

#if 1  // exhibit

void DataManager::printConfig()
{
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(0), "FP_CONFIG_INPUT");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "thread_number");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.thread_number);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "dm_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.dm_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "db_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.db_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "iop_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.iop_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "mp_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.mp_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.pg_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pp_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.pp_temp_directory_path);
}

void DataManager::printDatabase()
{
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(0), "FP_DATABASE_INPUT");
}

#endif

}  // namespace ifp
