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
  buildInstanceNameToIdxMap();
  buildRoutingLayerNameToIdxMap();
  buildIOPinNameToIdxMap();
  buildPGNetNameToIdxMap();
}

void DataManager::buildInstanceNameToIdxMap()
{
  std::map<std::string, int32_t>& instance_name_to_idx_map = _database.get_instance_name_to_idx_map();
  instance_name_to_idx_map.clear();
  for (int32_t instance_idx = 0; instance_idx < static_cast<int32_t>(_database.get_instance_list().size()); instance_idx++) {
    instance_name_to_idx_map[_database.get_instance_list()[instance_idx].get_name()] = instance_idx;
  }
}

void DataManager::buildRoutingLayerNameToIdxMap()
{
  std::map<std::string, int32_t>& routing_layer_name_to_idx_map = _database.get_routing_layer_name_to_idx_map();
  routing_layer_name_to_idx_map.clear();
  for (int32_t routing_layer_idx = 0; routing_layer_idx < static_cast<int32_t>(_database.get_routing_layer_list().size()); routing_layer_idx++) {
    routing_layer_name_to_idx_map[_database.get_routing_layer_list()[routing_layer_idx].get_name()] = routing_layer_idx;
  }
}

void DataManager::buildIOPinNameToIdxMap()
{
  std::map<std::string, int32_t>& io_pin_name_to_idx_map = _database.get_io_pin_name_to_idx_map();
  io_pin_name_to_idx_map.clear();
  for (int32_t io_pin_idx = 0; io_pin_idx < static_cast<int32_t>(_database.get_io_pin_list().size()); io_pin_idx++) {
    io_pin_name_to_idx_map[_database.get_io_pin_list()[io_pin_idx].get_name()] = io_pin_idx;
  }
}

void DataManager::buildPGNetNameToIdxMap()
{
  std::map<std::string, int32_t>& pg_net_name_to_idx_map = _database.get_pg_net_name_to_idx_map();
  pg_net_name_to_idx_map.clear();
  for (int32_t pg_net_idx = 0; pg_net_idx < static_cast<int32_t>(_database.get_pg_net_list().size()); pg_net_idx++) {
    pg_net_name_to_idx_map[_database.get_pg_net_list()[pg_net_idx].get_name()] = pg_net_idx;
  }
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
  Database& database = _database;
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(0), "FP_DATABASE_INPUT");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "design_name");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_design_name());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "micron_dbu");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_micron_dbu());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "manufacture_grid");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_manufacture_grid());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "cell_area");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_cell_area());
  Die& die = database.get_die();
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "die");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), "(", die.get_ll_x(), ",", die.get_ll_y(), ")-(", die.get_ur_x(), ",", die.get_ur_y(), ")");
  Core& core = database.get_core();
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "core");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), "(", core.get_ll_x(), ",", core.get_ll_y(), ")-(", core.get_ur_x(), ",", core.get_ur_y(), ")");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "site_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_site_map().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "instance_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_instance_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "net_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_net_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "routing_layer_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_routing_layer_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "io_pin_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_io_pin_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_net_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_pg_net_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_segment_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_pg_segment_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "placement_blockage_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_placement_blockage_rect_list().size());
}

#endif

}  // namespace ifp
