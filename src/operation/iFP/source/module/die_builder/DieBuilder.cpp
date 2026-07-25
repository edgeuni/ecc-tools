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

#include "DataManager.hpp"
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

  buildFloorplan();
  buildTrackList();

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

#if 1  // build

void DieBuilder::buildFloorplan()
{
  Config& config = FPDM.getConfig();
  if (config.die_area.size() == 4 && config.core_area.size() == 4) {
    buildDie(config.die_area[0], config.die_area[1], config.die_area[2], config.die_area[3]);
    buildCore(config.core_area[0], config.core_area[1], config.core_area[2], config.core_area[3], config.core_site, config.io_site,
              config.corner_site);
  } else if (config.core_util > 0.0 && !config.core_site.empty()) {
    buildAutoFloorplan();
  }
}

void DieBuilder::buildTrackList()
{
  for (std::vector<std::string>& value_list : FPDM.getConfig().track_list) {
    if (value_list.size() == 5) {
      buildTrack(value_list[0], std::stoi(value_list[1]), std::stoi(value_list[2]), std::stoi(value_list[3]), std::stoi(value_list[4]));
    }
  }
}

void DieBuilder::buildDie(double die_lx, double die_ly, double die_ux, double die_uy)
{
  Die& die = FPDM.getDatabase().get_die();
  die.set_rect(transUnitDB(die_lx), transUnitDB(die_ly), transUnitDB(die_ux), transUnitDB(die_uy));
  FPDM.getDatabase().set_die_updated(true);
}

void DieBuilder::buildCore(double core_lx, double core_ly, double core_ux, double core_uy, std::string core_site_name,
                           std::string io_site_name, std::string corner_site_name)
{
  Database& database = FPDM.getDatabase();
  std::string io_site = io_site_name.empty() ? core_site_name : io_site_name;
  std::string corner_site = corner_site_name.empty() ? io_site : corner_site_name;
  Site& core_site = database.get_site_map()[core_site_name];

  int32_t site_width = core_site.get_width();
  int32_t site_height = core_site.get_height();
  int32_t core_lx_int = alignUp(transUnitDB(core_lx), site_width);
  int32_t core_ly_int = alignUp(transUnitDB(core_ly), site_height);
  int32_t core_ux_int = alignDown(transUnitDB(core_ux), site_width);
  int32_t core_uy_int = alignDown(transUnitDB(core_uy), site_height);

  Core& core = database.get_core();
  core.set_rect(core_lx_int, core_ly_int, core_ux_int, core_uy_int);
  core.set_core_site_name(core_site_name);
  core.set_io_site_name(io_site);
  core.set_corner_site_name(corner_site);
  database.set_core_updated(true);
  buildRowList();
}

void DieBuilder::buildRowList()
{
  Database& database = FPDM.getDatabase();
  Core& core = database.get_core();
  Site& core_site = database.get_site_map()[core.get_core_site_name()];

  int32_t site_height = core_site.get_height();
  int32_t row_num = std::abs(core.get_height()) / site_height;
  for (int32_t row_idx = 0; row_idx < row_num; row_idx++) {
    int32_t y_coord = core.get_ll_y() + row_idx * site_height;
    Row row;
    row.set_name("ROW_" + std::to_string(row_idx));
    row.set_site_name(core.get_core_site_name());
    row.set_y(y_coord);
    row.set_orient_name(row_idx % 2 == 0 ? "FS" : "N");
    row.set_rect(core.get_ll_x(), y_coord, core.get_ur_x(), y_coord + site_height);
    database.get_new_row_list().push_back(row);
  }
}

void DieBuilder::buildAutoFloorplan()
{
  Config& config = FPDM.getConfig();
  double cell_area = config.cell_area > 0.0 ? config.cell_area : FPDM.getDatabase().get_cell_area();
  double core_area = cell_area / config.core_util;
  double core_height = std::sqrt(core_area / config.xy_ratio);
  double core_width = core_area / core_height;

  buildDie(0.0, 0.0, core_width + 2.0 * config.x_margin, core_height + 2.0 * config.y_margin);
  buildCore(config.x_margin, config.y_margin, config.x_margin + core_width, config.y_margin + core_height, config.core_site, config.io_site,
            config.corner_site);
}

void DieBuilder::buildTrack(std::string layer_name, int32_t x_offset, int32_t x_pitch, int32_t y_offset, int32_t y_pitch)
{
  Track track;
  track.set_layer_name(layer_name);
  track.set_x_offset(x_offset);
  track.set_x_pitch(x_pitch);
  track.set_y_offset(y_offset);
  track.set_y_pitch(y_pitch);
  FPDM.getDatabase().get_new_track_list().push_back(track);
  FPDM.getDatabase().set_track_updated(true);
}

#endif

#if 1  // utility

int32_t DieBuilder::transUnitDB(double value)
{
  return std::round(FPDM.getDatabase().get_micron_dbu() * value);
}

int32_t DieBuilder::alignDown(int32_t value, int32_t step)
{
  int32_t remainder = value % step;
  return remainder == 0 ? value : (value >= 0 ? value - remainder : value - remainder - step);
}

int32_t DieBuilder::alignUp(int32_t value, int32_t step)
{
  int32_t remainder = value % step;
  return remainder == 0 ? value : (value >= 0 ? value + step - remainder : value - remainder);
}

#endif

// private

DieBuilder* DieBuilder::_db_instance = nullptr;

}  // namespace ifp
