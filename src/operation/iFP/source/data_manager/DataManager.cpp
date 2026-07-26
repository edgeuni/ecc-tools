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
  _config.ip_temp_directory_path = _config.temp_directory_path + "io_placer/";
  _config.mp_temp_directory_path = _config.temp_directory_path + "macro_placer/";
  _config.pg_temp_directory_path = _config.temp_directory_path + "pdn_generator/";
  _config.pp_temp_directory_path = _config.temp_directory_path + "phy_placer/";

  FPUTIL.removeDir(_config.temp_directory_path);
  FPUTIL.createDir(_config.temp_directory_path);
  FPUTIL.createDirByFile(_config.log_file_path);
  FPUTIL.createDir(_config.dm_temp_directory_path);
  FPUTIL.createDir(_config.db_temp_directory_path);
  FPUTIL.createDir(_config.ip_temp_directory_path);
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
  buildPGIOPinList();
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

void DataManager::buildPGIOPinList()
{
  std::vector<IOPin>& io_pin_list = _database.get_io_pin_list();
  std::map<std::string, int32_t>& io_pin_name_to_idx_map = _database.get_io_pin_name_to_idx_map();
  for (PGIOPin& pg_io_pin : _config.pg_io_pin_list) {
    std::string& pin_name = pg_io_pin.get_pin_name();
    if (io_pin_name_to_idx_map.find(pin_name) != io_pin_name_to_idx_map.end()) {
      io_pin_list[io_pin_name_to_idx_map[pin_name]].set_special_net(true);
      continue;
    }

    IOPin io_pin;
    io_pin.set_name(pin_name);
    io_pin.set_special_net(true);
    io_pin_list.push_back(io_pin);
    io_pin_name_to_idx_map[pin_name] = static_cast<int32_t>(io_pin_list.size()) - 1;
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

  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_site_name");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_site_name);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_xy_ratio");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_xy_ratio);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_core_util");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_core_util);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_margin_left_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_margin_left_micron);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_margin_right_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_margin_right_micron);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_margin_top_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_margin_top_micron);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "layout_margin_bottom_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.layout_margin_bottom_micron);

  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "io_pin_layer_name_list");
  for (std::string& layer_name : _config.io_pin_layer_name_list) {
    FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), layer_name);
  }
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "io_pin_width_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.io_pin_width_micron);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "io_pin_depth_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.io_pin_depth_micron);

  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_io_pin_list");
  for (PGIOPin& pg_io_pin : _config.pg_io_pin_list) {
    FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), pg_io_pin.get_pin_name(), " ", pg_io_pin.get_net_name(), " ",
               pg_io_pin.get_net_type() == PGNetType::kPower ? "power" : "ground");
  }
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_global_connect_list");
  for (PGGlobalConnect& pg_global_connect : _config.pg_global_connect_list) {
    FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), pg_global_connect.get_net_name(), " ", pg_global_connect.get_instance_pin_name(), " ",
               pg_global_connect.get_net_type() == PGNetType::kPower ? "power" : "ground");
  }
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_grid_list");
  for (PGGrid& pg_grid : _config.pg_grid_list) {
    FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), pg_grid.get_power_net_name(), " ", pg_grid.get_ground_net_name(), " ",
               pg_grid.get_layer_name(), " ", pg_grid.get_width_micron(), " ", pg_grid.get_offset_micron());
  }
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_stripe_list");
  for (PGStripe& pg_stripe : _config.pg_stripe_list) {
    FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), pg_stripe.get_power_net_name(), " ", pg_stripe.get_ground_net_name(), " ",
               pg_stripe.get_layer_name(), " ", pg_stripe.get_width_micron(), " ", pg_stripe.get_pitch_micron(), " ",
               pg_stripe.get_offset_micron());
  }
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "pg_layer_pair_list");
  for (PGLayerPair& pg_layer_pair : _config.pg_layer_pair_list) {
    FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), pg_layer_pair.get_first_layer_name(), " ", pg_layer_pair.get_second_layer_name());
  }

  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "tapcell_name");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.tapcell_name);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "tap_distance_micron");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.tap_distance_micron);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "endcap_name");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.endcap_name);

  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(0), "FP_CONFIG_BUILD");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "log_file_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.log_file_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "dm_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.dm_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "db_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.db_temp_directory_path);
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "ip_temp_directory_path");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), _config.ip_temp_directory_path);
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
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "instance_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_instance_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "net_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_net_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "routing_layer_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_routing_layer_list().size());
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(1), "io_pin_num");
  FPLOG.info(Loc::current(), FPUTIL.getSpaceByTabNum(2), database.get_io_pin_list().size());
}

#endif

}  // namespace ifp
