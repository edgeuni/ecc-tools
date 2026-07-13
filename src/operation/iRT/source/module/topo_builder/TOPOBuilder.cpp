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
#include "TOPOBuilder.hpp"

#include "Utility.hpp"
#include "flute3/flute.h"

namespace irt {

// public

void TOPOBuilder::initInst()
{
  if (_tb_instance == nullptr) {
    _tb_instance = new TOPOBuilder();
  }
}

TOPOBuilder& TOPOBuilder::getInst()
{
  if (_tb_instance == nullptr) {
    RTLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tb_instance;
}

void TOPOBuilder::destroyInst()
{
  if (_tb_instance != nullptr) {
    delete _tb_instance;
    _tb_instance = nullptr;
  }
}

// function

void TOPOBuilder::init()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  Flute::readLUT();

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

TBResult TOPOBuilder::buildPlanarTopo(const TBTask& tb_task)
{
  TBResult tb_result;
  TBSteinerRepairStat steiner_repair_stat;
  std::vector<Segment<PlanarCoord>> raw_topo_list = getFlutePlanarTopoList(tb_task.get_planar_coord_list());
  tb_result.set_planar_topo_list(legalizePlanarTopo(tb_task, std::move(raw_topo_list), steiner_repair_stat));
  tb_result.set_steiner_repair_stat(steiner_repair_stat);
  return tb_result;
}

std::vector<Segment<PlanarCoord>> TOPOBuilder::getPlanarTopoList(const TBTask& tb_task)
{
  TBResult tb_result = buildPlanarTopo(tb_task);
  return std::move(tb_result.get_planar_topo_list());
}

void TOPOBuilder::destroy()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  Flute::deleteLUT();

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TOPOBuilder* TOPOBuilder::_tb_instance = nullptr;

std::vector<Segment<PlanarCoord>> TOPOBuilder::getFlutePlanarTopoList(const std::vector<PlanarCoord>& planar_coord_list)
{
  std::vector<Segment<PlanarCoord>> planar_topo_list;
  if (planar_coord_list.size() <= 1) {
    return planar_topo_list;
  }

  int32_t point_num = static_cast<int32_t>(planar_coord_list.size());
  std::vector<Flute::DTYPE> x_list(point_num);
  std::vector<Flute::DTYPE> y_list(point_num);
  for (int32_t i = 0; i < point_num; i++) {
    x_list[i] = planar_coord_list[i].get_x();
    y_list[i] = planar_coord_list[i].get_y();
  }
  Flute::Tree flute_tree = Flute::flute(point_num, x_list.data(), y_list.data(), FLUTE_ACCURACY);
  for (int32_t i = 0; i < 2 * flute_tree.deg - 2; i++) {
    int32_t neighbor_idx = flute_tree.branch[i].n;
    PlanarCoord first_coord(flute_tree.branch[i].x, flute_tree.branch[i].y);
    PlanarCoord second_coord(flute_tree.branch[neighbor_idx].x, flute_tree.branch[neighbor_idx].y);
    if (first_coord != second_coord) {
      planar_topo_list.emplace_back(first_coord, second_coord);
    }
  }
  Flute::free_tree(flute_tree);
  return planar_topo_list;
}

std::vector<Segment<PlanarCoord>> TOPOBuilder::legalizePlanarTopo(const TBTask& tb_task,
                                                                 std::vector<Segment<PlanarCoord>> raw_topo_list,
                                                                 TBSteinerRepairStat& steiner_repair_stat)
{
  const GridMap<bool>* steiner_forbidden_map = tb_task.get_steiner_forbidden_map();
  if (steiner_forbidden_map == nullptr || steiner_forbidden_map->empty()) {
    return raw_topo_list;
  }

  std::set<PlanarCoord, CmpPlanarCoordByXASC> terminal_coord_set(tb_task.get_planar_coord_list().begin(),
                                                                tb_task.get_planar_coord_list().end());
  std::map<PlanarCoord, PlanarCoord, CmpPlanarCoordByXASC> steiner_legal_coord_map;
  auto legalizeSteinerCoord = [&](const PlanarCoord& coord) {
    if (terminal_coord_set.find(coord) != terminal_coord_set.end() || !isSteinerForbiddenCoord(steiner_forbidden_map, coord)) {
      return coord;
    }
    auto legal_iter = steiner_legal_coord_map.find(coord);
    if (legal_iter != steiner_legal_coord_map.end()) {
      return legal_iter->second;
    }

    steiner_repair_stat.raw_steiner_in_macro++;
    PlanarCoord legal_coord = getNearestLegalCoord(*steiner_forbidden_map, coord);
    steiner_legal_coord_map[coord] = legal_coord;
    if (isSteinerForbiddenCoord(steiner_forbidden_map, legal_coord)) {
      steiner_repair_stat.failed_steiner_legalize_num++;
    } else {
      steiner_repair_stat.fixed_steiner_in_macro++;
    }
    return legal_coord;
  };

  std::vector<Segment<PlanarCoord>> legal_topo_list;
  legal_topo_list.reserve(raw_topo_list.size());
  for (Segment<PlanarCoord>& raw_topo : raw_topo_list) {
    PlanarCoord first_coord = legalizeSteinerCoord(raw_topo.get_first());
    PlanarCoord second_coord = legalizeSteinerCoord(raw_topo.get_second());
    if (first_coord != second_coord) {
      legal_topo_list.emplace_back(first_coord, second_coord);
    }
  }
  return legal_topo_list;
}

PlanarCoord TOPOBuilder::getNearestLegalCoord(const GridMap<bool>& steiner_forbidden_map, const PlanarCoord& coord)
{
  if (steiner_forbidden_map.empty() || !steiner_forbidden_map.isInside(coord.get_x(), coord.get_y())
      || !isSteinerForbiddenCoord(&steiner_forbidden_map, coord)) {
    return coord;
  }

  PlanarCoord best_coord = coord;
  int32_t best_distance = INT_MAX;
  auto updateBestCoord = [&](int32_t x, int32_t y) {
    if (!steiner_forbidden_map.isInside(x, y) || steiner_forbidden_map[x][y]) {
      return;
    }
    PlanarCoord candidate_coord(x, y);
    int32_t distance = RTUTIL.getManhattanDistance(coord, candidate_coord);
    if (distance < best_distance || (distance == best_distance && CmpPlanarCoordByXASC()(candidate_coord, best_coord))) {
      best_distance = distance;
      best_coord = candidate_coord;
    }
  };

  int32_t max_radius = steiner_forbidden_map.get_x_size() + steiner_forbidden_map.get_y_size();
  for (int32_t radius = 1; radius <= max_radius; radius++) {
    for (int32_t dx = -radius; dx <= radius; dx++) {
      updateBestCoord(coord.get_x() + dx, coord.get_y() - radius);
      updateBestCoord(coord.get_x() + dx, coord.get_y() + radius);
    }
    for (int32_t dy = -radius + 1; dy <= radius - 1; dy++) {
      updateBestCoord(coord.get_x() - radius, coord.get_y() + dy);
      updateBestCoord(coord.get_x() + radius, coord.get_y() + dy);
    }
    if (best_distance != INT_MAX) {
      return best_coord;
    }
  }
  return coord;
}

bool TOPOBuilder::isSteinerForbiddenCoord(const GridMap<bool>* steiner_forbidden_map, const PlanarCoord& coord)
{
  if (steiner_forbidden_map == nullptr || steiner_forbidden_map->empty()
      || !steiner_forbidden_map->isInside(coord.get_x(), coord.get_y())) {
    return false;
  }
  return (*steiner_forbidden_map)[coord.get_x()][coord.get_y()];
}

}  // namespace irt
