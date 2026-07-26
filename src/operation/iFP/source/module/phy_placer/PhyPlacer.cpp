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

#if 1  // place phy cell

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
  for (int32_t row_idx = 0; row_idx < static_cast<int32_t>(row_list.size()); row_idx++) {
    buildPPRegionInRow(pp_model, row_list[row_idx], row_idx);
    int32_t y_coord = row_list[row_idx].get_y();
    pp_model.set_top_y_coord(std::max(pp_model.get_top_y_coord(), y_coord));
    pp_model.set_bottom_y_coord(std::min(pp_model.get_bottom_y_coord(), y_coord));
  }
  return static_cast<int32_t>(pp_model.get_pp_region_list().size());
}

void PhyPlacer::buildPPRegionInRow(PPModel& pp_model, Row& row, int32_t row_idx)
{
  int32_t row_start_x = row.get_ll_x();
  int32_t row_start_y = row.get_ll_y();
  int32_t row_end_x = row.get_ur_x();
  int32_t row_end_y = row.get_ur_y();

  std::vector<PlanarRect*> blockage_rect_list;
  for (PlanarRect& blockage_rect : FPDM.getDatabase().get_placement_blockage_rect_list()) {
    if (row_end_y < blockage_rect.get_ll_y() || row_start_y > blockage_rect.get_ur_y() || row_start_x > blockage_rect.get_ur_x()
        || row_end_x < blockage_rect.get_ll_x()) {
      continue;
    }
    blockage_rect_list.push_back(&blockage_rect);
  }

  if (blockage_rect_list.empty()) {
    PPRegion pp_region;
    pp_region.set_row_idx(row_idx);
    pp_region.set_start_coord(row_start_x);
    pp_region.set_end_coord(row_end_x);
    pp_region.set_y_coord(row.get_y());
    pp_region.set_orient(row.get_orient());
    pp_model.get_pp_region_list().push_back(pp_region);
    return;
  }

  std::sort(blockage_rect_list.begin(), blockage_rect_list.end(),
            [](PlanarRect* rect_a, PlanarRect* rect_b) { return rect_a->get_ll_x() < rect_b->get_ll_x(); });
  for (int32_t rect_idx = 0; rect_idx < static_cast<int32_t>(blockage_rect_list.size()); rect_idx++) {
    if (rect_idx == 0 && row_start_x < blockage_rect_list[rect_idx]->get_ll_x()) {
      PPRegion pp_region;
      pp_region.set_row_idx(row_idx);
      pp_region.set_start_coord(row_start_x);
      pp_region.set_end_coord(blockage_rect_list[rect_idx]->get_ll_x());
      pp_region.set_y_coord(row.get_y());
      pp_region.set_orient(row.get_orient());
      pp_model.get_pp_region_list().push_back(pp_region);
    }
    if (rect_idx == static_cast<int32_t>(blockage_rect_list.size()) - 1 && row_end_x > blockage_rect_list[rect_idx]->get_ur_x()) {
      PPRegion pp_region;
      pp_region.set_row_idx(row_idx);
      pp_region.set_start_coord(blockage_rect_list[rect_idx]->get_ur_x());
      pp_region.set_end_coord(row_end_x);
      pp_region.set_y_coord(row.get_y());
      pp_region.set_orient(row.get_orient());
      pp_model.get_pp_region_list().push_back(pp_region);
    }
    if (rect_idx > 0 && rect_idx < static_cast<int32_t>(blockage_rect_list.size()) - 1) {
      PPRegion pp_region;
      pp_region.set_row_idx(row_idx);
      pp_region.set_start_coord(blockage_rect_list[rect_idx - 1]->get_ur_x());
      pp_region.set_end_coord(blockage_rect_list[rect_idx]->get_ll_x());
      pp_region.set_y_coord(row.get_y());
      pp_region.set_orient(row.get_orient());
      pp_model.get_pp_region_list().push_back(pp_region);
    }
  }
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
      addPhyCell("ENDCAP_" + std::to_string(endcap_idx++), endcap_name, pp_region.get_end_coord() - endcap_width,
                 pp_region.get_y_coord(), pp_region.get_orient());
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
      if (x_coord + 2 * inst_space >= region_end && region_end - x_coord > tapcell_width * 2
          && pp_region.get_row_idx() % 2 == 0) {
        addPhyCell("PHY_" + std::to_string(tapcell_idx++), tapcell_name, region_end - tapcell_width, pp_region.get_y_coord(),
                   pp_region.get_orient());
      }
      x_coord += inst_space * 2;
    }
  }
  return endcap_idx + tapcell_idx;
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

int32_t PhyPlacer::getCellMasterWidthByOrient(CellMaster& cell_master, PlacementOrientation orient)
{
  if (orient == PlacementOrientation::kN || orient == PlacementOrientation::kS || orient == PlacementOrientation::kFN
      || orient == PlacementOrientation::kFS) {
    return cell_master.get_width();
  }
  return cell_master.get_height();
}

#endif

// private

PhyPlacer* PhyPlacer::_pp_instance = nullptr;

}  // namespace ifp
