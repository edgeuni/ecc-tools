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
#include "PhyPlacer.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ifp {

// public

void PhyPlacer::initInst()
{
  if (_pp_instance == nullptr) {
    _pp_instance = new PhyPlacer();
  }
}

PhyPlacer& PhyPlacer::getInst()
{
  if (_pp_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pp_instance;
}

void PhyPlacer::destroyInst()
{
  if (_pp_instance != nullptr) {
    delete _pp_instance;
    _pp_instance = nullptr;
  }
}

// function

void PhyPlacer::place()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  PPModel pp_model;
  placePhyCell(pp_model);

  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PhyPlacer::placePhyCell(PPModel& pp_model)
{
  Config& config = FPDM.getConfig();
  if (config.tap_distance_micron <= 0.0 || config.tapcell_name.empty() || config.endcap_name.empty()) {
    return;
  }

  Database& database = FPDM.getDatabase();
  int32_t inst_space = FPUTIL.transMicronToDBU(config.tap_distance_micron, database.get_micron_dbu());
  adjustTapDistance(inst_space);

  if (database.get_cell_master_map().find(config.tapcell_name) == database.get_cell_master_map().end()
      || database.get_cell_master_map().find(config.endcap_name) == database.get_cell_master_map().end()) {
    return;
  }
  if (buildPPRegionList(pp_model) == 0) {
    return;
  }
  insertPhyCell(pp_model, inst_space, config.tapcell_name, config.endcap_name);
}

void PhyPlacer::adjustTapDistance(int32_t& inst_space)
{
  Database& database = FPDM.getDatabase();
  if (database.get_core().get_core_site_name().empty()) {
    return;
  }
  Site& core_site = database.get_site_map()[database.get_core().get_core_site_name()];
  if (inst_space % core_site.get_width() == 0) {
    return;
  }
  int32_t site_num = inst_space / core_site.get_width();
  inst_space = site_num * core_site.get_width();
}

int32_t PhyPlacer::buildPPRegionList(PPModel& pp_model)
{
  std::vector<Row>& row_list = FPDM.getDatabase().get_row_list();
  int32_t row_idx = -1;
  int32_t previous_y_coord = INT32_MIN;
  for (Row& row : row_list) {
    if (row.get_y() != previous_y_coord) {
      row_idx++;
      previous_y_coord = row.get_y();
    }
    buildPPRegionInRow(pp_model, row, row_idx);
    int32_t y_coord = row.get_y();
    pp_model.set_top_y_coord(std::max(pp_model.get_top_y_coord(), y_coord));
    pp_model.set_bottom_y_coord(std::min(pp_model.get_bottom_y_coord(), y_coord));
  }
  return static_cast<int32_t>(pp_model.get_pp_region_list().size());
}

void PhyPlacer::buildPPRegionInRow(PPModel& pp_model, Row& row, int32_t row_idx)
{
  std::vector<std::pair<int32_t, int32_t>> macro_bottom_interval_list = getMacroBottomIntervalList(row);
  if (macro_bottom_interval_list.empty()) {
    addPPRegion(pp_model, row, row_idx, row.get_ll_x(), row.get_ur_x());
    return;
  }

  int32_t current_x = row.get_ll_x();
  for (std::pair<int32_t, int32_t>& macro_bottom_interval : macro_bottom_interval_list) {
    if (macro_bottom_interval.second <= current_x) {
      continue;
    }
    if (macro_bottom_interval.first > current_x) {
      addPPRegion(pp_model, row, row_idx, current_x, macro_bottom_interval.first);
    }
    current_x = std::max(current_x, macro_bottom_interval.second);
    if (current_x >= row.get_ur_x()) {
      return;
    }
  }
  if (current_x < row.get_ur_x()) {
    addPPRegion(pp_model, row, row_idx, current_x, row.get_ur_x());
  }
}

std::vector<std::pair<int32_t, int32_t>> PhyPlacer::getMacroBottomIntervalList(Row& row)
{
  Database& database = FPDM.getDatabase();
  Site& site = database.get_site_map()[row.get_site_name()];
  std::vector<std::pair<int32_t, int32_t>> macro_bottom_interval_list;
  for (Instance& instance : database.get_instance_list()) {
    if (!instance.get_macro() || !instance.get_placed()) {
      continue;
    }
    PlanarRect& placement_halo_rect = instance.get_placement_halo_rect();
    if (row.get_ur_y() > placement_halo_rect.get_ll_y() || placement_halo_rect.get_ll_y() >= row.get_ur_y() + row.get_height()) {
      continue;
    }
    int32_t start_x = std::max(row.get_ll_x(), FPUTIL.alignDown(placement_halo_rect.get_ll_x(), site.get_width()));
    int32_t end_x = std::min(row.get_ur_x(), FPUTIL.alignUp(placement_halo_rect.get_ur_x(), site.get_width()));
    if (start_x < end_x) {
      macro_bottom_interval_list.emplace_back(start_x, end_x);
    }
  }
  std::sort(macro_bottom_interval_list.begin(), macro_bottom_interval_list.end(),
            [](const std::pair<int32_t, int32_t>& first, const std::pair<int32_t, int32_t>& second) { return first.first < second.first; });
  return macro_bottom_interval_list;
}

void PhyPlacer::addPPRegion(PPModel& pp_model, Row& row, int32_t row_idx, int32_t start_coord, int32_t end_coord)
{
  if (start_coord >= end_coord) {
    return;
  }
  PPRegion pp_region;
  pp_region.set_row_idx(row_idx);
  pp_region.set_start_coord(start_coord);
  pp_region.set_end_coord(end_coord);
  pp_region.set_y_coord(row.get_y());
  pp_region.set_orient(row.get_orient());
  pp_model.get_pp_region_list().push_back(pp_region);
}

int32_t PhyPlacer::insertPhyCell(PPModel& pp_model, int32_t inst_space, std::string tapcell_name, std::string endcap_name)
{
  Database& database = FPDM.getDatabase();
  CellMaster& tapcell_master = database.get_cell_master_map()[tapcell_name];
  CellMaster& endcap_master = database.get_cell_master_map()[endcap_name];

  int32_t endcap_idx = 0;
  int32_t tapcell_idx = 0;
  for (PPRegion& pp_region : pp_model.get_pp_region_list()) {
    int32_t endcap_width = getCellMasterWidthByOrient(endcap_master, pp_region.get_orient());
    if (pp_region.get_y_coord() == pp_model.get_top_y_coord() || pp_region.get_y_coord() == pp_model.get_bottom_y_coord()) {
      for (int32_t x_coord = pp_region.get_start_coord(); x_coord < pp_region.get_end_coord(); x_coord += endcap_width) {
        addPhyCell("ENDCAP_" + std::to_string(endcap_idx++), endcap_name, x_coord, pp_region.get_y_coord(), pp_region.get_orient());
      }
      continue;
    }

    if (pp_region.get_end_coord() - pp_region.get_start_coord() >= endcap_width) {
      addPhyCell("ENDCAP_" + std::to_string(endcap_idx++), endcap_name, pp_region.get_start_coord(), pp_region.get_y_coord(),
                 pp_region.get_orient());
    }
    if (pp_region.get_end_coord() - pp_region.get_start_coord() >= 2 * endcap_width) {
      addPhyCell("ENDCAP_" + std::to_string(endcap_idx++), endcap_name, pp_region.get_end_coord() - endcap_width, pp_region.get_y_coord(),
                 pp_region.get_orient());
    }

    int32_t core_start_x = database.get_core().get_ll_x();
    int32_t region_start = pp_region.get_start_coord() + endcap_width;
    int32_t region_end = pp_region.get_end_coord() - endcap_width;
    int32_t tapcell_width = getCellMasterWidthByOrient(tapcell_master, pp_region.get_orient());
    int32_t x_coord = region_start;
    while (x_coord + tapcell_width <= region_end) {
      if (x_coord == region_start) {
        if (pp_region.get_row_idx() % 2 == 0) {
          addPhyCell("PHY_" + std::to_string(tapcell_idx++), tapcell_name, region_start, pp_region.get_y_coord(), pp_region.get_orient());
          x_coord = core_start_x + ((x_coord - core_start_x) / inst_space + 2) * inst_space;
        } else {
          x_coord = core_start_x + ((x_coord - core_start_x) / inst_space + 1) * inst_space;
        }
        continue;
      }

      addPhyCell("PHY_" + std::to_string(tapcell_idx++), tapcell_name, x_coord, pp_region.get_y_coord(), pp_region.get_orient());
      if (x_coord + 2 * inst_space >= region_end && region_end - x_coord > tapcell_width * 2 && pp_region.get_row_idx() % 2 == 0) {
        addPhyCell("PHY_" + std::to_string(tapcell_idx++), tapcell_name, region_end - tapcell_width, pp_region.get_y_coord(),
                   pp_region.get_orient());
      }
      x_coord += inst_space * 2;
    }
  }
  insertMacroBottomEndcap(endcap_idx, endcap_name);
  return endcap_idx + tapcell_idx;
}

void PhyPlacer::insertMacroBottomEndcap(int32_t& endcap_idx, std::string endcap_name)
{
  Database& database = FPDM.getDatabase();
  CellMaster& endcap_master = database.get_cell_master_map()[endcap_name];
  for (Row& row : database.get_row_list()) {
    int32_t current_x = row.get_ll_x();
    int32_t endcap_width = getCellMasterWidthByOrient(endcap_master, row.get_orient());
    std::vector<std::pair<int32_t, int32_t>> macro_bottom_interval_list = getMacroBottomIntervalList(row);
    for (std::pair<int32_t, int32_t>& macro_bottom_interval : macro_bottom_interval_list) {
      if (macro_bottom_interval.second <= current_x) {
        continue;
      }
      int32_t start_x = std::max(current_x, macro_bottom_interval.first);
      for (int32_t x_coord = start_x; x_coord + endcap_width <= macro_bottom_interval.second; x_coord += endcap_width) {
        addPhyCell("ENDCAP_" + std::to_string(endcap_idx++), endcap_name, x_coord, row.get_y(), row.get_orient());
      }
      current_x = std::max(current_x, macro_bottom_interval.second);
    }
  }
}

int32_t PhyPlacer::getCellMasterWidthByOrient(CellMaster& cell_master, PlacementOrientation orient)
{
  if (orient == PlacementOrientation::kN || orient == PlacementOrientation::kS || orient == PlacementOrientation::kFN
      || orient == PlacementOrientation::kFS) {
    return cell_master.get_width();
  }
  return cell_master.get_height();
}

void PhyPlacer::addPhyCell(std::string instance_name, std::string cell_master_name, int32_t x_coord, int32_t y_coord,
                           PlacementOrientation orient)
{
  Database& database = FPDM.getDatabase();
  CellMaster& cell_master = database.get_cell_master_map()[cell_master_name];
  Instance instance;
  instance.set_name(instance_name);
  instance.set_master_name(cell_master_name);
  instance.set_orient(orient);
  instance.set_coord(x_coord, y_coord);
  instance.set_width(cell_master.get_width());
  instance.set_height(cell_master.get_height());
  instance.set_fixed(true);
  instance.set_placed(true);
  instance.set_new_instance(true);
  int32_t instance_idx = static_cast<int32_t>(database.get_instance_list().size());
  database.get_instance_list().push_back(instance);
  database.get_instance_name_to_idx_map()[instance_name] = instance_idx;
}

// private

PhyPlacer* PhyPlacer::_pp_instance = nullptr;

}  // namespace ifp
