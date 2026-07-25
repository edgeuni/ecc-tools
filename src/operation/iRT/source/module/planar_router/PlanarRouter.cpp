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
#include "PlanarRouter.hpp"

#include <cmath>

#include "GDSPlotter.hpp"
#include "RTInterface.hpp"
#include "TBTask.hpp"
#include "TOPOBuilder.hpp"
#include "PRCandidate.hpp"
#include "Utility.hpp"

namespace irt {

// public

void PlanarRouter::initInst()
{
  if (_pr_instance == nullptr) {
    _pr_instance = new PlanarRouter();
  }
}

PlanarRouter& PlanarRouter::getInst()
{
  if (_pr_instance == nullptr) {
    RTLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_pr_instance;
}

void PlanarRouter::destroyInst()
{
  if (_pr_instance != nullptr) {
    delete _pr_instance;
    _pr_instance = nullptr;
  }
}

// function

void PlanarRouter::generate()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  PRModel pr_model = initPRModel();
  setPRComParam(pr_model);
  initPRTaskList(pr_model);

  buildPlanarRoutingEdgeMap();

  runRouteFlow(pr_model);

  // debugPlotPRModel(pr_model, "after");
  updateSummary(pr_model);
  printSummary(pr_model);
  outputGuide(pr_model);
  outputNetCSV(pr_model);
  // outputUsageCSV(pr_model);
  // outputCongestionCostCSV(pr_model);
  outputJson(pr_model);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

bool PlanarRouter::repair()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  PRModel pr_model = initPRModel();
  setPRComParam(pr_model);
  updateLayerCongestion(pr_model);
  buildPlanarRoutingEdgeMap();
  std::vector<PRNet*> overflow_pr_net_list = buildPRResult(pr_model);
  if (overflow_pr_net_list.empty()) {
    RTLOG.info(Loc::current(), "No layer overflow net");
    return false;
  }
  routePRNetList(pr_model, overflow_pr_net_list, "layer overflow A*", PRRouteMode::kAStar);

  Die& die = RTDM.getDatabase().get_die();
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      RTDM.updateNetGlobalResultToGCellMap(ChangeType::kDel, net_idx, segment);
    }
  }
  for (PRNet& pr_net : pr_model.get_pr_net_list()) {
    uploadNetResult(pr_net);
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

PlanarRouter* PlanarRouter::_pr_instance = nullptr;

PRModel PlanarRouter::initPRModel()
{
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();

  PRModel pr_model;
  pr_model.set_pr_net_list(convertToPRNetList(net_list));
  return pr_model;
}

std::vector<PRNet> PlanarRouter::convertToPRNetList(std::vector<Net>& net_list)
{
  std::vector<PRNet> pr_net_list;
  pr_net_list.reserve(net_list.size());
  for (size_t i = 0; i < net_list.size(); i++) {
    pr_net_list.emplace_back(convertToPRNet(net_list[i]));
  }
  return pr_net_list;
}

PRNet PlanarRouter::convertToPRNet(Net& net)
{
  PRNet pr_net;
  pr_net.set_origin_net(&net);
  pr_net.set_net_idx(net.get_net_idx());
  pr_net.set_connect_type(net.get_connect_type());
  for (Pin& pin : net.get_pin_list()) {
    pr_net.get_pr_pin_list().push_back(PRPin(pin));
  }
  pr_net.set_bounding_box(net.get_bounding_box());
  return pr_net;
}

void PlanarRouter::setPRComParam(PRModel& pr_model)
{
  int32_t topo_spilt_length = 30;
  int32_t expand_step_num = 30;
  int32_t expand_step_length = 1;
  int32_t astar_search_margin = 50;
  double prefer_wire_unit = 1;
  double non_prefer_wire_unit = 2.5 * prefer_wire_unit;
  double corner_weight = non_prefer_wire_unit;
  double overflow_unit = 8 * non_prefer_wire_unit;
  /**
   * topo_spilt_length, expand_step_num, expand_step_length, astar_search_margin, overflow_unit
   */

  PRComParam pr_com_param(topo_spilt_length, expand_step_num, expand_step_length, astar_search_margin, overflow_unit, corner_weight);
  RTLOG.info(Loc::current(), "topo_spilt_length: ", pr_com_param.get_topo_spilt_length());
  RTLOG.info(Loc::current(), "expand_step_num: ", pr_com_param.get_expand_step_num());
  RTLOG.info(Loc::current(), "expand_step_length: ", pr_com_param.get_expand_step_length());
  RTLOG.info(Loc::current(), "astar_search_margin: ", pr_com_param.get_astar_search_margin());
  RTLOG.info(Loc::current(), "overflow_unit: ", pr_com_param.get_overflow_unit());
  RTLOG.info(Loc::current(), "corner_weight: ", pr_com_param.get_corner_weight());
  RTLOG.info(Loc::current(), "cost_mode: routing_edge");
  pr_model.set_pr_com_param(pr_com_param);
}

void PlanarRouter::initPRTaskList(PRModel& pr_model)
{
  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();
  std::vector<PRNet*>& pr_task_list = pr_model.get_pr_task_list();
  pr_task_list.reserve(pr_net_list.size());
  for (PRNet& pr_net : pr_net_list) {
    pr_task_list.push_back(&pr_net);
  }
  std::sort(pr_task_list.begin(), pr_task_list.end(), CmpPRNet());
}

void PlanarRouter::buildPlanarRoutingEdgeMap()
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  GridMap<RoutingEdge>& planar_routing_h_edge_map = RTDM.getDatabase().get_planar_routing_h_edge_map();
  GridMap<RoutingEdge>& planar_routing_v_edge_map = RTDM.getDatabase().get_planar_routing_v_edge_map();
  std::vector<GridMap<RoutingEdge>>& routing_h_edge_map = RTDM.getDatabase().get_routing_h_edge_map();
  std::vector<GridMap<RoutingEdge>>& routing_v_edge_map = RTDM.getDatabase().get_routing_v_edge_map();

  planar_routing_h_edge_map.init(std::max(0, gcell_map.get_x_size() - 1), gcell_map.get_y_size());
  planar_routing_v_edge_map.init(gcell_map.get_x_size(), std::max(0, gcell_map.get_y_size() - 1));
  for (GridMap<RoutingEdge>* planar_routing_edge_map : {&planar_routing_h_edge_map, &planar_routing_v_edge_map}) {
    for (int32_t layer_idx = 0; layer_idx < static_cast<int32_t>(routing_h_edge_map.size()); layer_idx++) {
      GridMap<RoutingEdge>& routing_edge_map = planar_routing_edge_map == &planar_routing_h_edge_map ? routing_h_edge_map[layer_idx]
                                                                                                        : routing_v_edge_map[layer_idx];
      for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
        for (int32_t y = 0; y < routing_edge_map.get_y_size(); y++) {
          RoutingEdge& planar_routing_edge = (*planar_routing_edge_map)[x][y];
          RoutingEdge& routing_edge = routing_edge_map[x][y];
          planar_routing_edge.set_supply(planar_routing_edge.get_supply() + routing_edge.get_supply());
          planar_routing_edge.set_congestion_cost(planar_routing_edge.get_congestion_cost() + routing_edge.get_congestion_cost());
          planar_routing_edge.get_ignore_net_set().insert(routing_edge.get_ignore_net_set().begin(), routing_edge.get_ignore_net_set().end());
        }
      }
    }
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::updateLayerCongestion(PRModel& pr_model)
{
  constexpr int32_t congestion_radius = 2;
  double overflow_unit = pr_model.get_pr_com_param().get_overflow_unit();
  for (std::vector<GridMap<RoutingEdge>>* routing_edge_map_list :
       {&RTDM.getDatabase().get_routing_h_edge_map(), &RTDM.getDatabase().get_routing_v_edge_map()}) {
    for (GridMap<RoutingEdge>& routing_edge_map : *routing_edge_map_list) {
      for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
        for (int32_t y = 0; y < routing_edge_map.get_y_size(); y++) {
          routing_edge_map[x][y].set_congestion_cost(0);
        }
      }
      for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
        for (int32_t y = 0; y < routing_edge_map.get_y_size(); y++) {
          int32_t overflow = routing_edge_map[x][y].get_overflow();
          if (overflow == 0) {
            continue;
          }
          double cost = overflow_unit * std::pow(overflow + 1, 4);
          for (int32_t neighbor_x = std::max(0, x - congestion_radius);
               neighbor_x <= std::min(routing_edge_map.get_x_size() - 1, x + congestion_radius); neighbor_x++) {
            for (int32_t neighbor_y = std::max(0, y - congestion_radius);
                 neighbor_y <= std::min(routing_edge_map.get_y_size() - 1, y + congestion_radius); neighbor_y++) {
              int32_t distance = std::abs(neighbor_x - x) + std::abs(neighbor_y - y);
              if (distance <= congestion_radius) {
                RoutingEdge& neighbor_edge = routing_edge_map[neighbor_x][neighbor_y];
                neighbor_edge.set_congestion_cost(neighbor_edge.get_congestion_cost() + cost / (distance + 1));
              }
            }
          }
        }
      }
    }
  }
}

std::vector<PRNet*> PlanarRouter::buildPRResult(PRModel& pr_model)
{
  Die& die = RTDM.getDatabase().get_die();
  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();
  std::vector<GridMap<RoutingEdge>>& routing_h_edge_map = RTDM.getDatabase().get_routing_h_edge_map();
  std::vector<GridMap<RoutingEdge>>& routing_v_edge_map = RTDM.getDatabase().get_routing_v_edge_map();
  std::vector<PRNet*> overflow_pr_net_list;

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    PRNet& pr_net = pr_net_list[net_idx];
    bool has_overflow = false;
    for (Segment<LayerCoord>* segment : segment_set) {
      LayerCoord& first_coord = segment->get_first();
      LayerCoord& second_coord = segment->get_second();
      if (first_coord.get_planar_coord() != second_coord.get_planar_coord()) {
        pr_net.get_routing_segment_list().emplace_back(first_coord.get_planar_coord(), second_coord.get_planar_coord());
      }
      if (first_coord.get_layer_idx() != second_coord.get_layer_idx()) {
        continue;
      }
      int32_t layer_idx = first_coord.get_layer_idx();
      if (RTUTIL.isHorizontal(first_coord, second_coord)) {
        int32_t first_x = std::min(first_coord.get_x(), second_coord.get_x());
        int32_t second_x = std::max(first_coord.get_x(), second_coord.get_x());
        for (int32_t x = first_x; x < second_x; x++) {
          RoutingEdge& routing_edge = routing_h_edge_map[layer_idx][x][first_coord.get_y()];
          has_overflow |= !routing_edge.get_ignore_net_set().count(net_idx) && routing_edge.get_overflow() > 0;
        }
      } else if (RTUTIL.isVertical(first_coord, second_coord)) {
        int32_t first_y = std::min(first_coord.get_y(), second_coord.get_y());
        int32_t second_y = std::max(first_coord.get_y(), second_coord.get_y());
        for (int32_t y = first_y; y < second_y; y++) {
          RoutingEdge& routing_edge = routing_v_edge_map[layer_idx][first_coord.get_x()][y];
          has_overflow |= !routing_edge.get_ignore_net_set().count(net_idx) && routing_edge.get_overflow() > 0;
        }
      }
    }
    if (has_overflow) {
      overflow_pr_net_list.push_back(&pr_net);
    }
  }
  for (PRNet& pr_net : pr_net_list) {
    if (pr_net.get_routing_segment_list().empty()) {
      continue;
    }
    pr_model.set_curr_pr_task(&pr_net);
    updateRoutingSegmentListToGraph(pr_model, pr_net.get_routing_segment_list(), ChangeType::kAdd, pr_net.get_routing_edge_set());
  }
  pr_model.set_curr_pr_task(nullptr);
  std::sort(overflow_pr_net_list.begin(), overflow_pr_net_list.end(), CmpPRNet());
  return overflow_pr_net_list;
}

// routing edge

RoutingEdge& PlanarRouter::getPlanarRoutingEdge(const PlanarCoord& first_coord, const PlanarCoord& second_coord)
{
  if (RTUTIL.getManhattanDistance(first_coord, second_coord) != 1) {
    RTLOG.error(Loc::current(), "The planar routing edge coord is error!");
  }
  if (RTUTIL.isHorizontal(first_coord, second_coord)) {
    int32_t x = std::min(first_coord.get_x(), second_coord.get_x());
    return RTDM.getDatabase().get_planar_routing_h_edge_map()[x][first_coord.get_y()];
  }
  if (RTUTIL.isVertical(first_coord, second_coord)) {
    int32_t y = std::min(first_coord.get_y(), second_coord.get_y());
    return RTDM.getDatabase().get_planar_routing_v_edge_map()[first_coord.get_x()][y];
  }
  RTLOG.error(Loc::current(), "The planar routing edge direction is error!");
  return RTDM.getDatabase().get_planar_routing_h_edge_map()[0][0];
}

PlanarRouter::PREdgeCost PlanarRouter::getRoutingEdgeCost(RoutingEdge& routing_edge, double overflow_unit)
{
  constexpr double saturation_start_ratio = 0.8;
  constexpr double hotspot_start_ratio = 0.9;

  PREdgeCost edge_cost;
  edge_cost.congestion_cost = routing_edge.get_congestion_cost();
  int32_t supply = routing_edge.get_supply();
  int32_t usage = routing_edge.get_usage();
  if (usage == 0) {
    return edge_cost;
  }
  if (supply <= 0) {
    edge_cost.max_usage_ratio = usage + 1.0;
    edge_cost.overflow = usage;
    edge_cost.is_overflow = true;
    edge_cost.overflow_cost = overflow_unit * std::pow(edge_cost.overflow + 1, 4);
    return edge_cost;
  }

  double usage_ratio = usage / 1.0 / supply;
  edge_cost.max_usage_ratio = usage_ratio;
  if (usage > supply) {
    edge_cost.overflow = usage - supply;
    edge_cost.is_overflow = true;
    edge_cost.overflow_cost = overflow_unit * std::pow(edge_cost.overflow + 1, 4);
    return edge_cost;
  }

  edge_cost.usage_cost = overflow_unit * std::pow(usage_ratio, 4);
  if (usage_ratio >= saturation_start_ratio) {
    double saturation_ratio = (usage_ratio - saturation_start_ratio) / (1.0 - saturation_start_ratio);
    edge_cost.is_saturated = true;
    edge_cost.saturation_cost = overflow_unit * std::pow(saturation_ratio, 2);
  }
  if (usage_ratio >= hotspot_start_ratio) {
    double hotspot_ratio = (usage_ratio - hotspot_start_ratio) / (1.0 - hotspot_start_ratio);
    edge_cost.is_hotspot = true;
    edge_cost.hotspot_cost = overflow_unit * 2.0 * std::pow(hotspot_ratio, 2);
  }
  return edge_cost;
}

void PlanarRouter::updateRoutingSegmentListToGraph(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list,
                                                    ChangeType change_type, std::set<RoutingEdge*>& routing_edge_set)
{
  int32_t delta = 0;
  if (change_type == ChangeType::kAdd) {
    delta = 1;
  } else if (change_type == ChangeType::kDel) {
    delta = -1;
  } else {
    RTLOG.error(Loc::current(), "The change type is error!");
  }

  int32_t curr_net_idx = pr_model.get_curr_pr_task()->get_net_idx();
  std::map<RoutingEdge*, std::set<Orientation>> routing_edge_orient_map;
  for (Segment<PlanarCoord>& routing_segment : routing_segment_list) {
    PlanarCoord first_coord = routing_segment.get_first();
    PlanarCoord second_coord = routing_segment.get_second();
    if (first_coord == second_coord) {
      continue;
    }
    if (!RTUTIL.isRightAngled(first_coord, second_coord)) {
      RTLOG.error(Loc::current(), "The routing segment is oblique!");
    }
    int32_t first_x = std::min(first_coord.get_x(), second_coord.get_x());
    int32_t second_x = std::max(first_coord.get_x(), second_coord.get_x());
    int32_t first_y = std::min(first_coord.get_y(), second_coord.get_y());
    int32_t second_y = std::max(first_coord.get_y(), second_coord.get_y());
    if (RTUTIL.isHorizontal(first_coord, second_coord)) {
      for (int32_t x = first_x; x <= second_x; x++) {
        if (x != first_x) {
          RoutingEdge& routing_edge = getPlanarRoutingEdge(PlanarCoord(x - 1, first_y), PlanarCoord(x, first_y));
          routing_edge_orient_map[&routing_edge].insert(Orientation::kWest);
        }
        if (x != second_x) {
          RoutingEdge& routing_edge = getPlanarRoutingEdge(PlanarCoord(x, first_y), PlanarCoord(x + 1, first_y));
          routing_edge_orient_map[&routing_edge].insert(Orientation::kEast);
        }
      }
    } else {
      for (int32_t y = first_y; y <= second_y; y++) {
        if (y != first_y) {
          RoutingEdge& routing_edge = getPlanarRoutingEdge(PlanarCoord(first_x, y - 1), PlanarCoord(first_x, y));
          routing_edge_orient_map[&routing_edge].insert(Orientation::kSouth);
        }
        if (y != second_y) {
          RoutingEdge& routing_edge = getPlanarRoutingEdge(PlanarCoord(first_x, y), PlanarCoord(first_x, y + 1));
          routing_edge_orient_map[&routing_edge].insert(Orientation::kNorth);
        }
      }
    }
  }
  for (auto& [routing_edge, orient_set] : routing_edge_orient_map) {
    if (change_type == ChangeType::kAdd) {
      if (!routing_edge_set.insert(routing_edge).second) {
        continue;
      }
    } else if (routing_edge_set.erase(routing_edge) == 0) {
      continue;
    }
    if (routing_edge->get_ignore_net_set().count(curr_net_idx)) {
      continue;
    }
    std::map<Orientation, int32_t>& orient_demand_map = routing_edge->get_orient_demand_map();
    for (Orientation orient : orient_set) {
      if (change_type == ChangeType::kDel && (!RTUTIL.exist(orient_demand_map, orient) || orient_demand_map[orient] <= 0)) {
        RTLOG.error(Loc::current(), "The planar routing edge demand is error!");
      }
      orient_demand_map[orient] += delta;
      if (orient_demand_map[orient] == 0) {
        orient_demand_map.erase(orient);
      }
    }
  }
}

void PlanarRouter::runRouteFlow(PRModel& pr_model)
{
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  constexpr int MAX_ITER =3;
  std::vector<PRNet*>& pr_task_list = pr_model.get_pr_task_list();

  routePRNetList(pr_model, pr_task_list, "initial LZ pattern", PRRouteMode::kLZPattern);
  updateCongestion(pr_model);
  routePRNetList(pr_model, pr_task_list, "congestion LZ pattern", PRRouteMode::kLZPattern);
  updateCongestion(pr_model);
  routePRNetList(pr_model, getOverflowPRNetList(pr_model), "repair All pattern", PRRouteMode::kAllPattern);
  updateCongestion(pr_model);

  for (int iter = 0; iter < MAX_ITER; iter++) {
    auto rerouteNets = getOverflowPRNetList(pr_model);
    if (rerouteNets.size() == 0) {
      break;
    }
    auto& param = pr_model.get_pr_com_param();
    param.set_astar_search_margin(param.get_astar_search_margin() * 2);
    param.set_overflow_unit(param.get_overflow_unit() * 2);

    routePRNetList(pr_model, rerouteNets, "overflow A*", PRRouteMode::kAStar);
    updateCongestion(pr_model);

  }

  // routePRNetList(pr_model, getHighUsagePRNetList(pr_model), "high usage A*", PRRouteMode::kAStar);
  // updateCongestion(pr_model);
  for (PRNet* pr_net : pr_task_list) {
    uploadNetResult(*pr_net);
  }

  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::routePRNetList(PRModel& pr_model, const std::vector<PRNet*>& pr_net_list, const char* route_mode,
                                  PRRouteMode pr_route_mode)
{
  RTLOG.info(Loc::current(), "Mode: ", route_mode, ", net_num: ", pr_net_list.size());
  size_t next_percent = 10;
  for (size_t i = 0; i < pr_net_list.size(); i++) {
    PRNet* pr_net = pr_net_list[i];
    routePRNet(pr_model, pr_net, pr_route_mode);
    size_t percent = (i + 1) * 100 / pr_net_list.size();
    if (percent >= next_percent || i + 1 == pr_net_list.size()) {
      RTLOG.info(Loc::current(), "Mode: ", route_mode, ", progress: ", percent, "% (", i + 1, "/", pr_net_list.size(), ")");
      next_percent += 10;
    }
  }
}

void PlanarRouter::routePRNet(PRModel& pr_model, PRNet* pr_net, PRRouteMode pr_route_mode)
{
  initSingleTask(pr_model, pr_net);
  std::vector<Segment<PlanarCoord>> old_routing_segment_list = pr_net->get_routing_segment_list();
  if (!old_routing_segment_list.empty()) {
    updateRoutingSegmentListToGraph(pr_model, old_routing_segment_list, ChangeType::kDel, pr_net->get_routing_edge_set());
  }
  if (!routeSingleTask(pr_model, pr_route_mode) && !old_routing_segment_list.empty()) {
    pr_net->set_routing_segment_list(old_routing_segment_list);
    updateRoutingSegmentListToGraph(pr_model, old_routing_segment_list, ChangeType::kAdd, pr_net->get_routing_edge_set());
  }
  resetSingleTask(pr_model);
}

void PlanarRouter::initSingleTask(PRModel& pr_model, PRNet* pr_net)
{
  pr_model.set_curr_pr_task(pr_net);
}

bool PlanarRouter::routeSingleTask(PRModel& pr_model, PRRouteMode pr_route_mode)
{
  std::vector<Segment<PlanarCoord>> routing_segment_list;
  PRNet* pr_net = pr_model.get_curr_pr_task();
  if (!routePlanarTopoList(pr_model, routing_segment_list, pr_route_mode)) {
    updateRoutingSegmentListToGraph(pr_model, routing_segment_list, ChangeType::kDel, pr_net->get_routing_edge_set());
    return false;
  }

  MTree<PlanarCoord> routing_tree = getCoordTree(pr_model, routing_segment_list);
  std::vector<Segment<PlanarCoord>> final_routing_segment_list;
  for (Segment<TNode<PlanarCoord>*>& routing_segment : RTUTIL.getSegListByTree(routing_tree)) {
    final_routing_segment_list.emplace_back(routing_segment.get_first()->value(), routing_segment.get_second()->value());
  }
  updateRoutingSegmentListToGraph(pr_model, routing_segment_list, ChangeType::kDel, pr_net->get_routing_edge_set());
  updateRoutingSegmentListToGraph(pr_model, final_routing_segment_list, ChangeType::kAdd, pr_net->get_routing_edge_set());
  pr_net->set_routing_segment_list(final_routing_segment_list);
  return true;
}

void PlanarRouter::resetSingleTask(PRModel& pr_model)
{
  pr_model.set_curr_pr_task(nullptr);
}

void PlanarRouter::updateCongestion(PRModel& pr_model)
{
  constexpr int32_t congestion_radius = 1;
  constexpr double congestion_decay = 0.5;
  double congestion_unit = pr_model.get_pr_com_param().get_overflow_unit();
  for (GridMap<RoutingEdge>* routing_edge_map : {&RTDM.getDatabase().get_planar_routing_h_edge_map(),
                                                  &RTDM.getDatabase().get_planar_routing_v_edge_map()}) {
    for (int32_t x = 0; x < routing_edge_map->get_x_size(); x++) {
      for (int32_t y = 0; y < routing_edge_map->get_y_size(); y++) {
        double total_usage_ratio = 0;
        int32_t edge_num = 0;
        for (int32_t neighbor_x = std::max(0, x - congestion_radius); neighbor_x <= std::min(routing_edge_map->get_x_size() - 1, x + congestion_radius);
             neighbor_x++) {
          for (int32_t neighbor_y = std::max(0, y - congestion_radius);
               neighbor_y <= std::min(routing_edge_map->get_y_size() - 1, y + congestion_radius); neighbor_y++) {
            RoutingEdge& neighbor_edge = (*routing_edge_map)[neighbor_x][neighbor_y];
            if (neighbor_edge.get_supply() == 0) {
              continue;
            }
            total_usage_ratio += neighbor_edge.get_usage() / 1.0 / neighbor_edge.get_supply();
            edge_num++;
          }
        }
        RoutingEdge& routing_edge = (*routing_edge_map)[x][y];
        double usage_ratio = routing_edge.get_supply() == 0 ? 0 : routing_edge.get_usage() / 1.0 / routing_edge.get_supply();
        usage_ratio = std::max(0.0, usage_ratio - 0.8);
        double average_usage_ratio = edge_num == 0 ? 0 : total_usage_ratio / edge_num;
        double new_congestion_cost = congestion_unit * std::pow(average_usage_ratio + usage_ratio, 2);
        routing_edge.set_congestion_cost(routing_edge.get_congestion_cost() * congestion_decay + new_congestion_cost);
      }
    }
  }
}

std::vector<PRNet*> PlanarRouter::getOverflowPRNetList(PRModel& pr_model)
{
  std::vector<PRNet*> pr_net_list;
  for (PRNet& pr_net : pr_model.get_pr_net_list()) {
    for (RoutingEdge* routing_edge : pr_net.get_routing_edge_set()) {
      if (routing_edge->get_ignore_net_set().count(pr_net.get_net_idx())) {
        continue;
      }
      if (routing_edge->get_overflow() > 0) {
        pr_net_list.push_back(&pr_net);
        break;
      }
    }
  }
  return pr_net_list;
}

std::vector<PRNet*> PlanarRouter::getHighUsagePRNetList(PRModel& pr_model)
{
  int32_t high_usage_net_num = pr_model.get_pr_net_list().size() / 20;
  std::vector<std::pair<double, PRNet*>> usage_pr_net_list;
  for (PRNet& pr_net : pr_model.get_pr_net_list()) {
    double max_usage_ratio = 0;
    for (RoutingEdge* routing_edge : pr_net.get_routing_edge_set()) {
      if (routing_edge->get_ignore_net_set().count(pr_net.get_net_idx()) || routing_edge->get_supply() == 0) {
        continue;
      }
      max_usage_ratio = std::max(max_usage_ratio, routing_edge->get_usage() / 1.0 / routing_edge->get_supply());
    }
    if (max_usage_ratio > 0.9) {
      usage_pr_net_list.emplace_back(max_usage_ratio, &pr_net);
    }
  }
  std::sort(usage_pr_net_list.begin(), usage_pr_net_list.end(), [](const auto& a, const auto& b) {
    if (a.first != b.first) {
      return a.first > b.first;
    }
    return a.second->get_net_idx() < b.second->get_net_idx();
  });

  std::vector<PRNet*> pr_net_list;
  for (int32_t i = 0; i < std::min(high_usage_net_num, static_cast<int32_t>(usage_pr_net_list.size())); i++) {
    pr_net_list.push_back(usage_pr_net_list[i].second);
  }
  return pr_net_list;
}

bool PlanarRouter::routePlanarTopoList(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list, PRRouteMode pr_route_mode)
{
  std::vector<Segment<PlanarCoord>> planar_topo_list = getPlanarTopoList(pr_model);

  for (size_t topo_idx = 0; topo_idx < planar_topo_list.size(); topo_idx++) {
    std::vector<PRCandidate> candidate_list;
    if (pr_route_mode != PRRouteMode::kAStar) {
      candidate_list = getPRCandidateListByTopo(pr_model, planar_topo_list[topo_idx], pr_route_mode);
      #pragma omp parallel for
      for (int32_t candidate_idx = 0; candidate_idx < static_cast<int32_t>(candidate_list.size()); candidate_idx++) {
        updatePRCandidate(pr_model, candidate_list[candidate_idx]);
      }
    } else {
      std::vector<Segment<PlanarCoord>> astar_segment_list = getRoutingSegmentListByAStar(pr_model, planar_topo_list[topo_idx]);
      if (astar_segment_list.empty()) {
        return false;
      }
      candidate_list.emplace_back(astar_segment_list);
      updatePRCandidate(pr_model, candidate_list.back());
    }

    PRCandidate* best_candidate = &candidate_list.front();
    for (PRCandidate& pr_candidate : candidate_list) {
      if (isBetterCandidate(pr_model, pr_candidate, *best_candidate)) {
        best_candidate = &pr_candidate;
      }
    }
    for (Segment<PlanarCoord>& routing_segment : best_candidate->get_routing_segment_list()) {
      routing_segment_list.push_back(routing_segment);
    }
    updateRoutingSegmentListToGraph(pr_model, best_candidate->get_routing_segment_list(), ChangeType::kAdd,
                                    pr_model.get_curr_pr_task()->get_routing_edge_set());
  }
  return true;
}

bool PlanarRouter::isBetterCandidate(PRModel& pr_model, PRCandidate& candidate, PRCandidate& current_best)
{
  double corner_weight = pr_model.get_pr_com_param().get_corner_weight();

  bool a_blocked = candidate.get_is_path_blocked();
  bool b_blocked = current_best.get_is_path_blocked();
  if (!a_blocked && b_blocked) {
    return true;
  } else if (a_blocked && !b_blocked) {
    return false;
  }
  bool a_overflow = candidate.get_is_overflow();
  bool b_overflow = current_best.get_is_overflow();
  if (!a_overflow && b_overflow) {
    return true;
  } else if (a_overflow && !b_overflow) {
    return false;
  }
  double score_a = candidate.get_total_wire_length() + candidate.get_total_cost() + corner_weight * candidate.get_total_corner_num();
  double score_b = current_best.get_total_wire_length() + current_best.get_total_cost() + corner_weight * current_best.get_total_corner_num();
  if (std::abs(score_a - score_b) < 1e-9) {
    if (candidate.get_saturation_edge_num() != current_best.get_saturation_edge_num()) {
      return candidate.get_saturation_edge_num() < current_best.get_saturation_edge_num();
    }
    if (candidate.get_hotspot_edge_num() != current_best.get_hotspot_edge_num()) {
      return candidate.get_hotspot_edge_num() < current_best.get_hotspot_edge_num();
    }
    if (std::abs(candidate.get_max_usage_ratio() - current_best.get_max_usage_ratio()) >= 1e-9) {
      return candidate.get_max_usage_ratio() < current_best.get_max_usage_ratio();
    }
    return candidate.get_total_wire_length() < current_best.get_total_wire_length();
  }
  return score_a < score_b;
}

std::vector<PRCandidate> PlanarRouter::getPRCandidateListByTopo(PRModel& pr_model, Segment<PlanarCoord>& planar_topo,
                                                                 PRRouteMode pr_route_mode)
{
  std::vector<PRCandidate> pr_candidate_list;

  std::vector<std::vector<std::vector<Segment<PlanarCoord>>>> pattern_list;
  if (!isLongObliqueTopo(pr_model, planar_topo)) {
    pattern_list.push_back(getRoutingSegmentListByStraight(planar_topo));
  }
  pattern_list.push_back(getRoutingSegmentListByLPattern(planar_topo));
  pattern_list.push_back(getRoutingSegmentListByZPattern(planar_topo));
  if (pr_route_mode == PRRouteMode::kAllPattern) {
    pattern_list.push_back(getRoutingSegmentListByInner3Bends(planar_topo));
    pattern_list.push_back(getRoutingSegmentListByUPattern(pr_model, planar_topo));
    pattern_list.push_back(getRoutingSegmentListByOuter3Bends(pr_model, planar_topo));
  }
  for (std::vector<std::vector<Segment<PlanarCoord>>>& routing_segment_list_list : pattern_list) {
    for (std::vector<Segment<PlanarCoord>>& routing_segment_list : routing_segment_list_list) {
      pr_candidate_list.emplace_back(routing_segment_list);
    }
  }
  return pr_candidate_list;
}

std::vector<Segment<PlanarCoord>> PlanarRouter::getPlanarTopoList(PRModel& pr_model)
{
  std::vector<PlanarCoord> planar_coord_list;
  {
    for (PRPin& pr_pin : pr_model.get_curr_pr_task()->get_pr_pin_list()) {
      planar_coord_list.push_back(pr_pin.get_access_point().get_grid_coord());
    }
    std::sort(planar_coord_list.begin(), planar_coord_list.end(), CmpPlanarCoordByXASC());
    planar_coord_list.erase(std::unique(planar_coord_list.begin(), planar_coord_list.end()), planar_coord_list.end());
  }
  TBTask tb_task;
  tb_task.set_planar_coord_list(planar_coord_list);
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  std::vector<Macro>& macro_list = RTDM.getDatabase().get_macro_list();
  std::vector<PlanarRect> planar_obs_list;
  planar_obs_list.reserve(macro_list.size());
  for (Macro& macro : macro_list) {
    PlanarRect body_grid_rect = RTUTIL.getClosedGCellGridRect(macro.get_body_rect(), gcell_axis);
    planar_obs_list.emplace_back(std::max(0, body_grid_rect.get_ll_x() - 1), std::max(0, body_grid_rect.get_ll_y() - 1),
                                 std::min(gcell_map.get_x_size() - 1, body_grid_rect.get_ur_x() + 1),
                                 std::min(gcell_map.get_y_size() - 1, body_grid_rect.get_ur_y() + 1));
  }
  tb_task.set_planar_obs_list(std::move(planar_obs_list));
  tb_task.set_planar_search_region(PlanarRect(0, 0, gcell_map.get_x_size() - 1, gcell_map.get_y_size() - 1));

  return RTTB.getPlanarTopoList(tb_task);
}

std::vector<Segment<PlanarCoord>> PlanarRouter::getRoutingSegmentListByAStar(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord start_coord = planar_topo.get_first();
  PlanarCoord end_coord = planar_topo.get_second();
  if (start_coord == end_coord) {
    return {};
  }
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  int32_t max_search_margin = std::max(gcell_map.get_x_size(), gcell_map.get_y_size());
  int32_t search_margin_step = std::max(1, pr_model.get_pr_com_param().get_astar_search_margin());
  int32_t search_margin = search_margin_step;
  while (true) {
    PlanarRect search_rect = getAStarSearchRect(planar_topo, search_margin);
    if (!prepareAStarWorkspace(search_rect, _astar_workspace)) {
      return {};
    }
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    if (searchRoutingSegmentByAStar(pr_model, start_coord, end_coord, _astar_workspace, routing_segment_list)) {
      return routing_segment_list;
    }
    if (search_rect.get_ll_x() == 0 && search_rect.get_ll_y() == 0 && search_rect.get_ur_x() == gcell_map.get_x_size() - 1
        && search_rect.get_ur_y() == gcell_map.get_y_size() - 1) {
      return {};
    }
    search_margin = std::min(max_search_margin, search_margin + search_margin_step);
  }
}

bool PlanarRouter::prepareAStarWorkspace(const PlanarRect& workspace_rect, PRAStarWorkspace& workspace)
{
  workspace.workspace_rect = workspace_rect;
  workspace.x_size = workspace_rect.get_ur_x() - workspace_rect.get_ll_x() + 1;
  workspace.y_size = workspace_rect.get_ur_y() - workspace_rect.get_ll_y() + 1;
  if (workspace.x_size <= 0 || workspace.y_size <= 0) {
    RTLOG.error(Loc::current(), "The A* workspace is empty!");
    return false;
  }
  int64_t cell_num = static_cast<int64_t>(workspace.x_size) * workspace.y_size;
  if (cell_num > INT_MAX) {
    RTLOG.error(Loc::current(), "The A* workspace is too large!");
    return false;
  }
  if (workspace.node_state_list.size() < static_cast<size_t>(cell_num)) {
    workspace.node_state_list.resize(static_cast<size_t>(cell_num));
  }
  return true;
}

int32_t PlanarRouter::getAStarNodeIndex(const PRAStarWorkspace& workspace, const PlanarCoord& coord)
{
  int32_t local_x = coord.get_x() - workspace.workspace_rect.get_ll_x();
  int32_t local_y = coord.get_y() - workspace.workspace_rect.get_ll_y();
  if (local_x < 0 || workspace.x_size <= local_x || local_y < 0 || workspace.y_size <= local_y) {
    RTLOG.error(Loc::current(), "The A* node is outside the workspace!");
  }
  return local_x * workspace.y_size + local_y;
}

PlanarCoord PlanarRouter::getAStarNodeCoord(const PRAStarWorkspace& workspace, int32_t node_idx)
{
  int64_t cell_num = static_cast<int64_t>(workspace.x_size) * workspace.y_size;
  if (node_idx < 0 || cell_num <= node_idx) {
    RTLOG.error(Loc::current(), "The A* node index is outside the workspace!");
  }
  return PlanarCoord(workspace.workspace_rect.get_ll_x() + node_idx / workspace.y_size,
                     workspace.workspace_rect.get_ll_y() + node_idx % workspace.y_size);
}

PlanarRouter::PRAStarNodeState& PlanarRouter::getAStarNodeState(PRAStarWorkspace& workspace, int32_t node_idx)
{
  PRAStarNodeState& node_state = workspace.node_state_list[node_idx];
  if (node_state.search_stamp != workspace.search_stamp) {
    node_state.search_stamp = workspace.search_stamp;
    node_state.closed = false;
    node_state.parent_idx = -1;
    node_state.known_cost = DBL_MAX;
  }
  return node_state;
}

bool PlanarRouter::searchRoutingSegmentByAStar(PRModel& pr_model, const PlanarCoord& start_coord, const PlanarCoord& end_coord,
                                               PRAStarWorkspace& workspace, std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  if (start_coord == end_coord || !RTUTIL.isInside(workspace.workspace_rect, start_coord)
      || !RTUTIL.isInside(workspace.workspace_rect, end_coord)) {
    return false;
  }
  workspace.search_stamp++;
  if (workspace.search_stamp == 0) {
    for (PRAStarNodeState& node_state : workspace.node_state_list) {
      node_state.search_stamp = 0;
    }
    workspace.search_stamp = 1;
  }
  workspace.open_heap.clear();

  auto cmpQueueNode = [&](const PRAStarQueueNode& a, const PRAStarQueueNode& b) {
    if (std::abs(a.getTotalCost() - b.getTotalCost()) < 1e-9) {
      if (std::abs(a.estimated_cost - b.estimated_cost) < 1e-9) {
        return a.node_idx > b.node_idx;
      }
      return a.estimated_cost > b.estimated_cost;
    }
    return a.getTotalCost() > b.getTotalCost();
  };
  int32_t start_idx = getAStarNodeIndex(workspace, start_coord);
  int32_t end_idx = getAStarNodeIndex(workspace, end_coord);
  const std::set<RoutingEdge*>& routing_edge_set = pr_model.get_curr_pr_task()->get_routing_edge_set();
  double heuristic_weight = routing_edge_set.empty() ? 1.0 : 0.0;
  PRAStarNodeState& start_state = getAStarNodeState(workspace, start_idx);
  start_state.known_cost = 0;
  workspace.open_heap.push_back({start_idx, 0, heuristic_weight * RTUTIL.getManhattanDistance(start_coord, end_coord)});
  std::push_heap(workspace.open_heap.begin(), workspace.open_heap.end(), cmpQueueNode);

  GridMap<RoutingEdge>& routing_h_edge_map = RTDM.getDatabase().get_planar_routing_h_edge_map();
  GridMap<RoutingEdge>& routing_v_edge_map = RTDM.getDatabase().get_planar_routing_v_edge_map();
  int32_t curr_net_idx = pr_model.get_curr_pr_task()->get_net_idx();
  double overflow_unit = pr_model.get_pr_com_param().get_overflow_unit();
  double corner_weight = pr_model.get_pr_com_param().get_corner_weight();
  int32_t workspace_ll_x = workspace.workspace_rect.get_ll_x();
  int32_t workspace_ll_y = workspace.workspace_rect.get_ll_y();
  constexpr int32_t step_x_list[] = {-1, 1, 0, 0};
  constexpr int32_t step_y_list[] = {0, 0, -1, 1};
  while (!workspace.open_heap.empty()) {
    std::pop_heap(workspace.open_heap.begin(), workspace.open_heap.end(), cmpQueueNode);
    PRAStarQueueNode queue_node = workspace.open_heap.back();
    workspace.open_heap.pop_back();

    PRAStarNodeState& curr_node_state = getAStarNodeState(workspace, queue_node.node_idx);
    if (curr_node_state.closed || !RTUTIL.equalDoubleByError(queue_node.known_cost, curr_node_state.known_cost, RT_ERROR)) {
      continue;
    }
    curr_node_state.closed = true;
    if (queue_node.node_idx == end_idx) {
      break;
    }

    int32_t curr_local_x = queue_node.node_idx / workspace.y_size;
    int32_t curr_local_y = queue_node.node_idx % workspace.y_size;
    int32_t curr_x = workspace_ll_x + curr_local_x;
    int32_t curr_y = workspace_ll_y + curr_local_y;
    bool has_parent = curr_node_state.parent_idx != -1;
    bool parent_is_horizontal = has_parent && std::abs(curr_node_state.parent_idx - queue_node.node_idx) == workspace.y_size;
    for (size_t step_idx = 0; step_idx < 4; step_idx++) {
      int32_t step_x = step_x_list[step_idx];
      int32_t step_y = step_y_list[step_idx];
      int32_t neighbor_local_x = curr_local_x + step_x;
      int32_t neighbor_local_y = curr_local_y + step_y;
      if (neighbor_local_x < 0 || workspace.x_size <= neighbor_local_x || neighbor_local_y < 0
          || workspace.y_size <= neighbor_local_y) {
        continue;
      }

      int32_t neighbor_idx = queue_node.node_idx + step_x * workspace.y_size + step_y;
      PRAStarNodeState& neighbor_node_state = getAStarNodeState(workspace, neighbor_idx);
      if (neighbor_node_state.closed) {
        continue;
      }

      int32_t neighbor_x = curr_x + step_x;
      int32_t neighbor_y = curr_y + step_y;
      bool is_horizontal = step_x != 0;
      RoutingEdge& routing_edge = is_horizontal ? routing_h_edge_map[std::min(curr_x, neighbor_x)][curr_y]
                                                : routing_v_edge_map[curr_x][std::min(curr_y, neighbor_y)];
      bool is_owned = routing_edge_set.count(&routing_edge);
      bool is_ignored = routing_edge.get_ignore_net_set().count(curr_net_idx);
      if (!is_owned && routing_edge.get_supply() == 0 && !is_ignored) {
        continue;
      }
      double step_cost = is_owned ? 0 : 1.0;
      if (!is_owned && !is_ignored) {
        RoutingEdge candidate_edge = routing_edge;
        std::map<Orientation, int32_t>& orient_demand_map = candidate_edge.get_orient_demand_map();
        orient_demand_map[is_horizontal ? Orientation::kEast : Orientation::kSouth]++;
        orient_demand_map[is_horizontal ? Orientation::kWest : Orientation::kNorth]++;
        step_cost += getRoutingEdgeCost(candidate_edge, overflow_unit).getTotalCost();
      }
      if (has_parent && parent_is_horizontal != is_horizontal) {
        step_cost += corner_weight;
      }
      double next_known_cost = curr_node_state.known_cost + step_cost;
      if (next_known_cost < neighbor_node_state.known_cost) {
        neighbor_node_state.parent_idx = queue_node.node_idx;
        neighbor_node_state.known_cost = next_known_cost;
        double estimated_cost = heuristic_weight * (std::abs(neighbor_x - end_coord.get_x()) + std::abs(neighbor_y - end_coord.get_y()));
        workspace.open_heap.push_back({neighbor_idx, next_known_cost, estimated_cost});
        std::push_heap(workspace.open_heap.begin(), workspace.open_heap.end(), cmpQueueNode);
      }
    }
  }

  PRAStarNodeState& end_state = getAStarNodeState(workspace, end_idx);
  if (!end_state.closed) {
    return false;
  }

  std::vector<PlanarCoord> coord_list;
  int32_t curr_idx = end_idx;
  while (true) {
    coord_list.push_back(getAStarNodeCoord(workspace, curr_idx));
    if (curr_idx == start_idx) {
      break;
    }
    curr_idx = getAStarNodeState(workspace, curr_idx).parent_idx;
    if (curr_idx == -1) {
      return false;
    }
  }
  std::reverse(coord_list.begin(), coord_list.end());
  routing_segment_list = getRoutingSegmentListByCoordList(coord_list);
  return !routing_segment_list.empty();
}

PlanarRect PlanarRouter::getAStarSearchRect(Segment<PlanarCoord>& planar_topo, int32_t search_margin)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  std::vector<Macro>& macro_list = RTDM.getDatabase().get_macro_list();
  PlanarCoord first_coord = planar_topo.get_first();
  PlanarCoord second_coord = planar_topo.get_second();

  PlanarRect topo_rect(std::min(first_coord.get_x(), second_coord.get_x()), std::min(first_coord.get_y(), second_coord.get_y()),
                       std::max(first_coord.get_x(), second_coord.get_x()), std::max(first_coord.get_y(), second_coord.get_y()));
  PlanarRect search_rect = topo_rect;
  for (Macro& macro : macro_list) {
    PlanarRect body_grid_rect = RTUTIL.getClosedGCellGridRect(macro.get_body_rect(), gcell_axis);
    if (!RTUTIL.isClosedOverlap(topo_rect, body_grid_rect) && !RTUTIL.isInside(body_grid_rect, first_coord)
        && !RTUTIL.isInside(body_grid_rect, second_coord)) {
      continue;
    }
    search_rect.set_ll_x(std::min(search_rect.get_ll_x(), body_grid_rect.get_ll_x()));
    search_rect.set_ll_y(std::min(search_rect.get_ll_y(), body_grid_rect.get_ll_y()));
    search_rect.set_ur_x(std::max(search_rect.get_ur_x(), body_grid_rect.get_ur_x()));
    search_rect.set_ur_y(std::max(search_rect.get_ur_y(), body_grid_rect.get_ur_y()));
  }

  search_rect.set_ll_x(std::max(0, search_rect.get_ll_x() - search_margin));
  search_rect.set_ll_y(std::max(0, search_rect.get_ll_y() - search_margin));
  search_rect.set_ur_x(std::min(gcell_map.get_x_size() - 1, search_rect.get_ur_x() + search_margin));
  search_rect.set_ur_y(std::min(gcell_map.get_y_size() - 1, search_rect.get_ur_y() + search_margin));
  return search_rect;
}

std::vector<Segment<PlanarCoord>> PlanarRouter::getRoutingSegmentListByCoordList(std::vector<PlanarCoord>& coord_list)
{
  std::vector<Segment<PlanarCoord>> routing_segment_list;
  if (coord_list.size() <= 1) {
    return routing_segment_list;
  }

  PlanarCoord segment_first_coord = coord_list.front();
  PlanarCoord prev_coord = coord_list.front();
  Direction prev_direction = Direction::kNone;
  for (size_t i = 1; i < coord_list.size(); i++) {
    PlanarCoord curr_coord = coord_list[i];
    if (curr_coord == prev_coord) {
      continue;
    }
    Direction curr_direction = RTUTIL.getDirection(prev_coord, curr_coord);
    if (curr_direction == Direction::kOblique) {
      return {};
    }
    if (prev_direction != Direction::kNone && curr_direction != prev_direction) {
      routing_segment_list.emplace_back(segment_first_coord, prev_coord);
      segment_first_coord = prev_coord;
    }
    prev_coord = curr_coord;
    prev_direction = curr_direction;
  }
  if (segment_first_coord != prev_coord) {
    routing_segment_list.emplace_back(segment_first_coord, prev_coord);
  }
  return routing_segment_list;
}

bool PlanarRouter::isLongObliqueTopo(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  int32_t topo_spilt_length = pr_model.get_pr_com_param().get_topo_spilt_length();
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  int32_t span_x = std::abs(first_coord.get_x() - second_coord.get_x());
  int32_t span_y = std::abs(first_coord.get_y() - second_coord.get_y());
  return (span_x > 1 && span_y > 1 && (span_x > topo_spilt_length || span_y > topo_spilt_length));
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByStraight(Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isOblique(first_coord, second_coord)) {
    return {};
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(first_coord, second_coord);
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByLPattern(Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  PlanarCoord inflection_coord1(first_coord.get_x(), second_coord.get_y());
  inflection_list_list.push_back({inflection_coord1});
  PlanarCoord inflection_coord2(second_coord.get_x(), first_coord.get_y());
  inflection_list_list.push_back({inflection_coord2});

  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByZPattern(Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  std::vector<int32_t> x_mid_index_list = getMidIndexList(first_coord.get_x(), second_coord.get_x());
  std::vector<int32_t> y_mid_index_list = getMidIndexList(first_coord.get_y(), second_coord.get_y());
  if (x_mid_index_list.empty() && y_mid_index_list.empty()) {
    return {};
  }
  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  for (size_t i = 0; i < x_mid_index_list.size(); i++) {
    PlanarCoord inflection_coord1(x_mid_index_list[i], first_coord.get_y());
    PlanarCoord inflection_coord2(x_mid_index_list[i], second_coord.get_y());
    inflection_list_list.push_back({inflection_coord1, inflection_coord2});
  }
  for (size_t i = 0; i < y_mid_index_list.size(); i++) {
    PlanarCoord inflection_coord1(first_coord.get_x(), y_mid_index_list[i]);
    PlanarCoord inflection_coord2(second_coord.get_x(), y_mid_index_list[i]);
    inflection_list_list.push_back({inflection_coord1, inflection_coord2});
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<int32_t> PlanarRouter::getMidIndexList(int32_t first_idx, int32_t second_idx)
{
  std::vector<int32_t> mid_index_list;
  RTUTIL.swapByASC(first_idx, second_idx);
  mid_index_list.reserve(second_idx - first_idx - 1);
  for (int32_t i = (first_idx + 1); i <= (second_idx - 1); i++) {
    mid_index_list.push_back(i);
  }
  return mid_index_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByUPattern(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  Die& die = RTDM.getDatabase().get_die();
  int32_t expand_step_num = pr_model.get_pr_com_param().get_expand_step_num();
  int32_t expand_step_length = pr_model.get_pr_com_param().get_expand_step_length();

  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.getManhattanDistance(first_coord, second_coord) <= 1) {
    return {};
  }
  int32_t first_x = first_coord.get_x();
  int32_t second_x = second_coord.get_x();
  int32_t first_y = first_coord.get_y();
  int32_t second_y = second_coord.get_y();
  RTUTIL.swapByASC(first_x, second_x);
  RTUTIL.swapByASC(first_y, second_y);

  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  if (!RTUTIL.isHorizontal(first_coord, second_coord)) {
    for (int32_t i = 0; i < expand_step_num; i++) {
      first_x -= expand_step_length;
      if (first_x >= die.get_grid_ll_x()) {
        PlanarCoord inflection_coord1(first_x, first_coord.get_y());
        PlanarCoord inflection_coord2(first_x, second_coord.get_y());
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
      second_x += expand_step_length;
      if (second_x <= die.get_grid_ur_x()) {
        PlanarCoord inflection_coord1(second_x, first_coord.get_y());
        PlanarCoord inflection_coord2(second_x, second_coord.get_y());
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
    }
  }
  if (!RTUTIL.isVertical(first_coord, second_coord)) {
    for (int32_t i = 0; i < expand_step_num; i++) {
      first_y -= expand_step_length;
      if (first_y >= die.get_grid_ll_y()) {
        PlanarCoord inflection_coord1(first_coord.get_x(), first_y);
        PlanarCoord inflection_coord2(second_coord.get_x(), first_y);
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
      second_y += expand_step_length;
      if (second_y <= die.get_grid_ur_y()) {
        PlanarCoord inflection_coord1(first_coord.get_x(), second_y);
        PlanarCoord inflection_coord2(second_coord.get_x(), second_y);
        inflection_list_list.push_back({inflection_coord1, inflection_coord2});
      }
    }
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByInner3Bends(Segment<PlanarCoord>& planar_topo)
{
  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  std::vector<int32_t> x_mid_index_list = getMidIndexList(first_coord.get_x(), second_coord.get_x());
  std::vector<int32_t> y_mid_index_list = getMidIndexList(first_coord.get_y(), second_coord.get_y());
  if (x_mid_index_list.empty() || y_mid_index_list.empty()) {
    return {};
  }
  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  for (size_t i = 0; i < x_mid_index_list.size(); i++) {
    for (size_t j = 0; j < y_mid_index_list.size(); j++) {
      PlanarCoord inflection_coord1(x_mid_index_list[i], first_coord.get_y());
      PlanarCoord inflection_coord2(x_mid_index_list[i], y_mid_index_list[j]);
      PlanarCoord inflection_coord3(second_coord.get_x(), y_mid_index_list[j]);
      inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
    }
  }

  for (size_t i = 0; i < x_mid_index_list.size(); i++) {
    for (size_t j = 0; j < y_mid_index_list.size(); j++) {
      PlanarCoord inflection_coord1(first_coord.get_x(), y_mid_index_list[j]);
      PlanarCoord inflection_coord2(x_mid_index_list[i], y_mid_index_list[j]);
      PlanarCoord inflection_coord3(x_mid_index_list[i], second_coord.get_y());
      inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
    }
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

std::vector<std::vector<Segment<PlanarCoord>>> PlanarRouter::getRoutingSegmentListByOuter3Bends(PRModel& pr_model, Segment<PlanarCoord>& planar_topo)
{
  Die& die = RTDM.getDatabase().get_die();
  int32_t expand_step_num = pr_model.get_pr_com_param().get_expand_step_num();
  int32_t expand_step_length = pr_model.get_pr_com_param().get_expand_step_length();

  PlanarCoord& first_coord = planar_topo.get_first();
  PlanarCoord& second_coord = planar_topo.get_second();
  if (RTUTIL.isRightAngled(first_coord, second_coord)) {
    return {};
  }
  int32_t start_x = first_coord.get_x();
  int32_t end_x = second_coord.get_x();
  int32_t start_y = first_coord.get_y();
  int32_t end_y = second_coord.get_y();

  int32_t box_lb_x = std::min(start_x, end_x);
  int32_t box_rt_x = std::max(start_x, end_x);
  int32_t box_lb_y = std::min(start_y, end_y);
  int32_t box_rt_y = std::max(start_y, end_y);

  std::vector<std::vector<PlanarCoord>> inflection_list_list;
  for (int32_t i = 0; i < expand_step_num; i++) {
    box_lb_x -= expand_step_length;
    box_rt_x += expand_step_length;
    box_lb_y -= expand_step_length;
    box_rt_y += expand_step_length;
    if (start_x < end_x) {
      if (start_y < end_y) {
        /**
         *    line style
         *
         *            x(e)
         *          x
         *        x
         *      x(s)
         *
         */
        if (die.get_grid_ll_y() <= box_lb_y && box_rt_x <= die.get_grid_ur_x()) {
          PlanarCoord inflection_coord1(start_x, box_lb_y);
          PlanarCoord inflection_coord2(box_rt_x, box_lb_y);
          PlanarCoord inflection_coord3(box_rt_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(box_lb_x, start_y);
          PlanarCoord inflection_coord2(box_lb_x, box_rt_y);
          PlanarCoord inflection_coord3(end_x, box_rt_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      } else {
        /**
         *    line style
         *
         *   x(s)
         *     x
         *       x
         *         x(e)
         *
         */
        if (box_rt_x <= die.get_grid_ur_x() && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(start_x, box_rt_y);
          PlanarCoord inflection_coord2(box_rt_x, box_rt_y);
          PlanarCoord inflection_coord3(box_rt_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && die.get_grid_ll_y() <= box_lb_y) {
          PlanarCoord inflection_coord1(box_lb_x, start_y);
          PlanarCoord inflection_coord2(box_lb_x, box_lb_y);
          PlanarCoord inflection_coord3(end_x, box_lb_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      }

    } else {
      if (start_y < end_y) {
        /**
         *    line style
         *
         *   x(e)
         *     x
         *       x
         *         x(s)
         *
         */
        if (box_rt_x <= die.get_grid_ur_x() && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(box_rt_x, start_y);
          PlanarCoord inflection_coord2(box_rt_x, box_rt_y);
          PlanarCoord inflection_coord3(end_x, box_rt_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && die.get_grid_ll_y() <= box_lb_y) {
          PlanarCoord inflection_coord1(start_x, box_lb_y);
          PlanarCoord inflection_coord2(box_lb_x, box_lb_y);
          PlanarCoord inflection_coord3(box_lb_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      } else {
        /**
         *    line style
         *
         *            x(s)
         *          x
         *        x
         *      x(e)
         *
         */
        if (die.get_grid_ll_y() <= box_lb_y && box_rt_x <= die.get_grid_ur_x()) {
          PlanarCoord inflection_coord1(box_rt_x, start_y);
          PlanarCoord inflection_coord2(box_rt_x, box_lb_y);
          PlanarCoord inflection_coord3(end_x, box_lb_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
        if (die.get_grid_ll_x() <= box_lb_x && box_rt_y <= die.get_grid_ur_y()) {
          PlanarCoord inflection_coord1(start_x, box_rt_y);
          PlanarCoord inflection_coord2(box_lb_x, box_rt_y);
          PlanarCoord inflection_coord3(box_lb_x, end_y);
          inflection_list_list.push_back({inflection_coord1, inflection_coord2, inflection_coord3});
        }
      }
    }
  }
  std::vector<std::vector<Segment<PlanarCoord>>> routing_segment_list_list;
  for (std::vector<PlanarCoord>& inflection_list : inflection_list_list) {
    std::vector<Segment<PlanarCoord>> routing_segment_list;
    routing_segment_list.emplace_back(planar_topo.get_first(), inflection_list.front());
    for (size_t i = 1; i < inflection_list.size(); i++) {
      routing_segment_list.emplace_back(inflection_list[i - 1], inflection_list[i]);
    }
    routing_segment_list.emplace_back(inflection_list.back(), planar_topo.get_second());
    routing_segment_list_list.push_back(routing_segment_list);
  }
  return routing_segment_list_list;
}

void PlanarRouter::updatePRCandidate(PRModel& pr_model, PRCandidate& pr_candidate)
{
  double overflow_unit = pr_model.get_pr_com_param().get_overflow_unit();
  int32_t curr_net_idx = pr_model.get_curr_pr_task()->get_net_idx();

  const std::set<RoutingEdge*>& routing_edge_set = pr_model.get_curr_pr_task()->get_routing_edge_set();

  PRCandidateCost candidate_cost;
  Direction pre_direction = Direction::kNone;
  for (Segment<PlanarCoord>& coord_segment : pr_candidate.get_routing_segment_list()) {
    PlanarCoord& first_coord = coord_segment.get_first();
    PlanarCoord& second_coord = coord_segment.get_second();
    if (!RTUTIL.isRightAngled(first_coord, second_coord)) {
      RTLOG.error(Loc::current(), "The direction is error!");
    }
    Direction direction = RTUTIL.getDirection(first_coord, second_coord);
    if (pre_direction != Direction::kNone && pre_direction != direction) {
      candidate_cost.total_corner_num++;
    }
    pre_direction = direction;
  }
  struct CandidateEdge
  {
    RoutingEdge* routing_edge = nullptr;
    bool is_horizontal = false;
  };
  std::vector<CandidateEdge> candidate_edge_list;
  GridMap<RoutingEdge>& routing_h_edge_map = RTDM.getDatabase().get_planar_routing_h_edge_map();
  GridMap<RoutingEdge>& routing_v_edge_map = RTDM.getDatabase().get_planar_routing_v_edge_map();
  for (Segment<PlanarCoord>& routing_segment : pr_candidate.get_routing_segment_list()) {
    PlanarCoord first_coord = routing_segment.get_first();
    PlanarCoord second_coord = routing_segment.get_second();
    int32_t first_x = std::min(first_coord.get_x(), second_coord.get_x());
    int32_t second_x = std::max(first_coord.get_x(), second_coord.get_x());
    int32_t first_y = std::min(first_coord.get_y(), second_coord.get_y());
    int32_t second_y = std::max(first_coord.get_y(), second_coord.get_y());
    if (RTUTIL.isHorizontal(first_coord, second_coord)) {
      for (int32_t x = first_x; x < second_x; x++) {
        candidate_edge_list.push_back({&routing_h_edge_map[x][first_y], true});
      }
    } else {
      for (int32_t y = first_y; y < second_y; y++) {
        candidate_edge_list.push_back({&routing_v_edge_map[first_x][y], false});
      }
    }
  }
  std::sort(candidate_edge_list.begin(), candidate_edge_list.end(), [](const CandidateEdge& a, const CandidateEdge& b) {
    return std::less<RoutingEdge*>()(a.routing_edge, b.routing_edge);
  });
  auto unique_end = std::unique(candidate_edge_list.begin(), candidate_edge_list.end(), [](const CandidateEdge& a, const CandidateEdge& b) {
    return a.routing_edge == b.routing_edge;
  });
  candidate_edge_list.erase(unique_end, candidate_edge_list.end());
  for (const CandidateEdge& edge_record : candidate_edge_list) {
    RoutingEdge* routing_edge = edge_record.routing_edge;
    if (routing_edge_set.count(routing_edge)) {
      continue;
    }
    candidate_cost.total_wire_length++;
    bool is_ignored = routing_edge->get_ignore_net_set().count(curr_net_idx);
    if (routing_edge->get_supply() == 0 && !is_ignored) {
      candidate_cost.is_path_blocked = true;
    }
    if (is_ignored) {
      continue;
    }
    RoutingEdge candidate_edge = *routing_edge;
    std::map<Orientation, int32_t>& orient_demand_map = candidate_edge.get_orient_demand_map();
    orient_demand_map[edge_record.is_horizontal ? Orientation::kEast : Orientation::kSouth]++;
    orient_demand_map[edge_record.is_horizontal ? Orientation::kWest : Orientation::kNorth]++;
    PREdgeCost edge_cost = getRoutingEdgeCost(candidate_edge, overflow_unit);
    if (edge_cost.is_overflow) {
      candidate_cost.is_overflow = true;
    }
    candidate_cost.total_cost += edge_cost.getTotalCost();
    candidate_cost.max_usage_ratio = std::max(candidate_cost.max_usage_ratio, edge_cost.max_usage_ratio);
    if (edge_cost.is_saturated) {
      candidate_cost.saturation_edge_num++;
    }
    if (edge_cost.is_hotspot) {
      candidate_cost.hotspot_edge_num++;
    }
  }

  pr_candidate.set_candidate_cost(candidate_cost);
}

MTree<PlanarCoord> PlanarRouter::getCoordTree(PRModel& pr_model, std::vector<Segment<PlanarCoord>>& routing_segment_list)
{
  std::vector<PlanarCoord> candidate_root_coord_list;
  std::map<PlanarCoord, std::set<int32_t>, CmpPlanarCoordByXASC> key_coord_pin_map;
  std::vector<PRPin>& pr_pin_list = pr_model.get_curr_pr_task()->get_pr_pin_list();
  for (size_t i = 0; i < pr_pin_list.size(); i++) {
    PlanarCoord coord = pr_pin_list[i].get_access_point().get_grid_coord();
    candidate_root_coord_list.push_back(coord);
    key_coord_pin_map[coord].insert(static_cast<int32_t>(i));
  }
  return RTUTIL.getTreeByFullFlow(candidate_root_coord_list, routing_segment_list, key_coord_pin_map);
}

void PlanarRouter::uploadNetResult(PRNet& pr_net)
{
  for (Segment<PlanarCoord>& routing_segment : pr_net.get_routing_segment_list()) {
    Segment<LayerCoord>* segment = new Segment<LayerCoord>({routing_segment.get_first(), 0}, {routing_segment.get_second(), 0});
    RTDM.updateNetGlobalResultToGCellMap(ChangeType::kAdd, pr_net.get_net_idx(), segment);
  }
}

// exhibit

void PlanarRouter::updateSummary(PRModel& pr_model)
{
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  GridMap<GCell>& gcell_map = RTDM.getDatabase().get_gcell_map();
  Summary& summary = RTDM.getDatabase().get_summary();
  int32_t enable_timing = RTDM.getConfig().enable_timing;

  double& total_demand = summary.pr_summary.total_demand;
  double& total_overflow = summary.pr_summary.total_overflow;
  double& total_wire_length = summary.pr_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.pr_summary.clock_timing_map;

  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();

  total_demand = 0;
  total_overflow = 0;
  total_wire_length = 0;
  clock_timing_map.clear();

  for (GridMap<RoutingEdge>* routing_edge_map : {&RTDM.getDatabase().get_planar_routing_h_edge_map(),
                                                  &RTDM.getDatabase().get_planar_routing_v_edge_map()}) {
    for (int32_t x = 0; x < routing_edge_map->get_x_size(); x++) {
      for (int32_t y = 0; y < routing_edge_map->get_y_size(); y++) {
        RoutingEdge& routing_edge = (*routing_edge_map)[x][y];
        total_demand += routing_edge.get_usage();
        total_overflow += routing_edge.get_overflow();
      }
    }
  }
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    for (Segment<LayerCoord>* segment : segment_set) {
      LayerCoord& first_coord = segment->get_first();
      int32_t first_layer_idx = first_coord.get_layer_idx();
      LayerCoord& second_coord = segment->get_second();
      int32_t second_layer_idx = second_coord.get_layer_idx();

      if (first_layer_idx == second_layer_idx) {
        GCell& first_gcell = gcell_map[first_coord.get_x()][first_coord.get_y()];
        GCell& second_gcell = gcell_map[second_coord.get_x()][second_coord.get_y()];
        double wire_length = RTUTIL.getManhattanDistance(first_gcell.getMidPoint(), second_gcell.getMidPoint()) / 1.0 / micron_dbu;
        total_wire_length += wire_length;
      } else {
        RTLOG.error(Loc::current(), "first_layer_idx != second_layer_idx!");
      }
    }
  }
  if (enable_timing) {
    std::vector<std::map<std::string, std::vector<LayerCoord>>> real_pin_coord_map_list;
    real_pin_coord_map_list.resize(pr_net_list.size());
    std::vector<std::vector<Segment<LayerCoord>>> routing_segment_list_list;
    routing_segment_list_list.resize(pr_net_list.size());
    for (PRNet& pr_net : pr_net_list) {
      for (PRPin& pr_pin : pr_net.get_pr_pin_list()) {
        LayerCoord layer_coord = pr_pin.get_access_point().getGridLayerCoord();
        real_pin_coord_map_list[pr_net.get_net_idx()][pr_pin.get_pin_name()].emplace_back(RTUTIL.getRealRectByGCell(layer_coord, gcell_axis).getMidPoint(), 0);
      }
    }
    for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
      for (Segment<LayerCoord>* segment : segment_set) {
        LayerCoord first_layer_coord = segment->get_first();
        LayerCoord first_real_coord(RTUTIL.getRealRectByGCell(first_layer_coord, gcell_axis).getMidPoint(), first_layer_coord.get_layer_idx());
        LayerCoord second_layer_coord = segment->get_second();
        LayerCoord second_real_coord(RTUTIL.getRealRectByGCell(second_layer_coord, gcell_axis).getMidPoint(), second_layer_coord.get_layer_idx());

        routing_segment_list_list[net_idx].emplace_back(first_real_coord, second_real_coord);
      }
    }
    RTI.updateTiming(real_pin_coord_map_list, routing_segment_list_list, clock_timing_map);
  }
}

void PlanarRouter::printSummary(PRModel& pr_model)
{
  Summary& summary = RTDM.getDatabase().get_summary();
  int32_t enable_timing = RTDM.getConfig().enable_timing;

  double& total_demand = summary.pr_summary.total_demand;
  double& total_overflow = summary.pr_summary.total_overflow;
  double& total_wire_length = summary.pr_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.pr_summary.clock_timing_map;

  fort::char_table summary_table;
  {
    summary_table.set_cell_text_align(fort::text_align::right);
    summary_table << fort::header << "total_demand" << total_demand << fort::endr;
    summary_table << fort::header << "total_overflow" << total_overflow << fort::endr;
    summary_table << fort::header << "total_wire_length" << total_wire_length << fort::endr;
  }
  fort::char_table timing_table;
  timing_table.set_cell_text_align(fort::text_align::right);
  if (enable_timing) {
    timing_table << fort::header << "clock_name"
                 << "tns"
                 << "wns"
                 << "freq" << fort::endr;
    for (auto& [clock_name, timing_map] : clock_timing_map) {
      timing_table << clock_name << timing_map["TNS"] << timing_map["WNS"] << timing_map["Freq(MHz)"] << fort::endr;
    }
  }
  RTUTIL.printTableList({summary_table});
  RTUTIL.printTableList({timing_table});
}

void PlanarRouter::outputGuide(PRModel& pr_model)
{
  int32_t micron_dbu = RTDM.getDatabase().get_micron_dbu();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  std::vector<PRNet>& pr_net_list = pr_model.get_pr_net_list();

  std::ofstream* guide_file_stream = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, "route.guide"));
  if (guide_file_stream == nullptr) {
    return;
  }
  RTUTIL.pushStream(guide_file_stream, "guide net_name\n");
  RTUTIL.pushStream(guide_file_stream, "pin grid_x grid_y real_x real_y layer energy name\n");
  RTUTIL.pushStream(guide_file_stream, "wire grid1_x grid1_y grid2_x grid2_y real1_x real1_y real2_x real2_y layer\n");
  RTUTIL.pushStream(guide_file_stream, "via grid_x grid_y real_x real_y layer1 layer2\n");

  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    PRNet& pr_net = pr_net_list[net_idx];
    RTUTIL.pushStream(guide_file_stream, "guide ", pr_net.get_origin_net()->get_net_name(), "\n");

    for (PRPin& pr_pin : pr_net.get_pr_pin_list()) {
      AccessPoint& access_point = pr_pin.get_access_point();
      double grid_x = access_point.get_grid_x();
      double grid_y = access_point.get_grid_y();
      double real_x = access_point.get_real_x() / 1.0 / micron_dbu;
      double real_y = access_point.get_real_y() / 1.0 / micron_dbu;
      std::string layer = routing_layer_list[access_point.get_layer_idx()].get_layer_name();
      std::string connnect;
      if (pr_pin.get_is_driven()) {
        connnect = "driven";
      } else {
        connnect = "load";
      }
      RTUTIL.pushStream(guide_file_stream, "pin ", grid_x, " ", grid_y, " ", real_x, " ", real_y, " ", layer, " ", connnect, " ", pr_pin.get_pin_name(), "\n");
    }
    for (Segment<LayerCoord>* segment : segment_set) {
      LayerCoord first_layer_coord = segment->get_first();
      double grid1_x = first_layer_coord.get_x();
      double grid1_y = first_layer_coord.get_y();
      int32_t first_layer_idx = first_layer_coord.get_layer_idx();

      PlanarCoord first_mid_coord = RTUTIL.getRealRectByGCell(first_layer_coord, gcell_axis).getMidPoint();
      double real1_x = first_mid_coord.get_x() / 1.0 / micron_dbu;
      double real1_y = first_mid_coord.get_y() / 1.0 / micron_dbu;

      LayerCoord second_layer_coord = segment->get_second();
      double grid2_x = second_layer_coord.get_x();
      double grid2_y = second_layer_coord.get_y();
      int32_t second_layer_idx = second_layer_coord.get_layer_idx();

      PlanarCoord second_mid_coord = RTUTIL.getRealRectByGCell(second_layer_coord, gcell_axis).getMidPoint();
      double real2_x = second_mid_coord.get_x() / 1.0 / micron_dbu;
      double real2_y = second_mid_coord.get_y() / 1.0 / micron_dbu;

      if (first_layer_idx != second_layer_idx) {
        RTUTIL.swapByASC(first_layer_idx, second_layer_idx);
        std::string layer1 = routing_layer_list[first_layer_idx].get_layer_name();
        std::string layer2 = routing_layer_list[second_layer_idx].get_layer_name();
        RTUTIL.pushStream(guide_file_stream, "via ", grid1_x, " ", grid1_y, " ", real1_x, " ", real1_y, " ", layer1, " ", layer2, "\n");
      } else {
        std::string layer = routing_layer_list[first_layer_idx].get_layer_name();
        RTUTIL.pushStream(guide_file_stream, "wire ", grid1_x, " ", grid1_y, " ", grid2_x, " ", grid2_y, " ", real1_x, " ", real1_y, " ", real2_x, " ", real2_y,
                          " ", layer, "\n");
      }
    }
  }
  RTUTIL.closeFileStream(guide_file_stream);
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputNetCSV(PRModel& pr_model)
{
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  int32_t output_inter_result = RTDM.getConfig().output_inter_result;
  if (!output_inter_result) {
    return;
  }
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  for (std::pair<std::string, GridMap<RoutingEdge>*> edge_map_pair : {
           std::make_pair("h_net_map.csv", &RTDM.getDatabase().get_planar_routing_h_edge_map()),
           std::make_pair("v_net_map.csv", &RTDM.getDatabase().get_planar_routing_v_edge_map())}) {
    std::ofstream* net_csv_file = RTUTIL.getOutputFileStream(RTUTIL.getString(pr_temp_directory_path, edge_map_pair.first));
    GridMap<RoutingEdge>& routing_edge_map = *edge_map_pair.second;
    for (int32_t y = routing_edge_map.get_y_size() - 1; y >= 0; y--) {
      for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
        RTUTIL.pushStream(net_csv_file, routing_edge_map[x][y].get_usage(), ",");
      }
      RTUTIL.pushStream(net_csv_file, "\n");
    }
    RTUTIL.closeFileStream(net_csv_file);
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputUsageCSV(PRModel& pr_model)
{
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  for (std::pair<std::string, GridMap<RoutingEdge>*> edge_map_pair : {
           std::make_pair("h_usage_map.csv", &RTDM.getDatabase().get_planar_routing_h_edge_map()),
           std::make_pair("v_usage_map.csv", &RTDM.getDatabase().get_planar_routing_v_edge_map())}) {
    std::ofstream csv_file(pr_temp_directory_path + edge_map_pair.first);
    if (!csv_file.is_open()) {
      RTLOG.error(Loc::current(), "Failed to open file '", pr_temp_directory_path + edge_map_pair.first, "'!");
      continue;
    }
    GridMap<RoutingEdge>& routing_edge_map = *edge_map_pair.second;
    for (int32_t y = routing_edge_map.get_y_size() - 1; y >= 0; y--) {
      for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
        csv_file << routing_edge_map[x][y].get_usage() << ",";
      }
      csv_file << "\n";
    }
    csv_file.close();
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputCongestionCostCSV(PRModel& pr_model)
{
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;
  Monitor monitor;
  RTLOG.info(Loc::current(), "Starting...");

  for (std::pair<std::string, GridMap<RoutingEdge>*> edge_map_pair : {
           std::make_pair("h_congestion_cost_map.csv", &RTDM.getDatabase().get_planar_routing_h_edge_map()),
           std::make_pair("v_congestion_cost_map.csv", &RTDM.getDatabase().get_planar_routing_v_edge_map())}) {
    std::ofstream csv_file(pr_temp_directory_path + edge_map_pair.first);
    if (!csv_file.is_open()) {
      RTLOG.error(Loc::current(), "Failed to open file '", pr_temp_directory_path + edge_map_pair.first, "'!");
      continue;
    }
    GridMap<RoutingEdge>& routing_edge_map = *edge_map_pair.second;
    for (int32_t y = routing_edge_map.get_y_size() - 1; y >= 0; y--) {
      for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
        csv_file << routing_edge_map[x][y].get_congestion_cost() << ",";
      }
      csv_file << "\n";
    }
    csv_file.close();
  }
  RTLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void PlanarRouter::outputJson(PRModel& pr_model)
{
  int32_t enable_notification = RTDM.getConfig().enable_notification;
  if (!enable_notification) {
    return;
  }
  std::map<std::string, std::string> json_path_map;
  json_path_map["net_map"] = outputNetJson(pr_model);
  json_path_map["overflow_map"] = outputOverflowJson(pr_model);
  json_path_map["summary"] = outputSummaryJson(pr_model);
  RTI.sendNotification("PR", 1, json_path_map);
}

std::string PlanarRouter::outputNetJson(PRModel& pr_model)
{
  Die& die = RTDM.getDatabase().get_die();
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::vector<Net>& net_list = RTDM.getDatabase().get_net_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  std::vector<nlohmann::json> net_json_list;
  {
    nlohmann::json result_shape_json;
    for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
      std::string net_name = net_list[net_idx].get_net_name();
      for (Segment<LayerCoord>* segment : segment_set) {
        PlanarRect first_gcell = RTUTIL.getRealRectByGCell(segment->get_first(), gcell_axis);
        PlanarRect second_gcell = RTUTIL.getRealRectByGCell(segment->get_second(), gcell_axis);
        if (segment->get_first().get_layer_idx() != segment->get_second().get_layer_idx()) {
          result_shape_json["result_shape"][net_name]["path"].push_back({first_gcell.get_ll_x(), first_gcell.get_ll_y(), first_gcell.get_ur_x(),
                                                                         first_gcell.get_ur_y(),
                                                                         routing_layer_list[segment->get_first().get_layer_idx()].get_layer_name()});
          result_shape_json["result_shape"][net_name]["path"].push_back({second_gcell.get_ll_x(), second_gcell.get_ll_y(), second_gcell.get_ur_x(),
                                                                         second_gcell.get_ur_y(),
                                                                         routing_layer_list[segment->get_second().get_layer_idx()].get_layer_name()});
        } else {
          PlanarRect gcell = RTUTIL.getBoundingBox({first_gcell, second_gcell});
          result_shape_json["result_shape"][net_name]["path"].push_back({gcell.get_ll_x(), gcell.get_ll_y(), gcell.get_ur_x(), gcell.get_ur_y(),
                                                                         routing_layer_list[segment->get_first().get_layer_idx()].get_layer_name()});
        }
      }
    }
    net_json_list.push_back(result_shape_json);
  }
  std::string net_json_file_path = RTUTIL.getString(pr_temp_directory_path, "net_map.json");
  std::ofstream* net_json_file = RTUTIL.getOutputFileStream(net_json_file_path);
  (*net_json_file) << net_json_list;
  RTUTIL.closeFileStream(net_json_file);
  return net_json_file_path;
}

std::string PlanarRouter::outputOverflowJson(PRModel& pr_model)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  std::vector<nlohmann::json> overflow_json_list;
  for (std::pair<std::string, GridMap<RoutingEdge>*> edge_map_pair : {
           std::make_pair("horizontal", &RTDM.getDatabase().get_planar_routing_h_edge_map()),
           std::make_pair("vertical", &RTDM.getDatabase().get_planar_routing_v_edge_map())}) {
    GridMap<RoutingEdge>& routing_edge_map = *edge_map_pair.second;
    bool is_horizontal = edge_map_pair.first == "horizontal";
    for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
      for (int32_t y = 0; y < routing_edge_map.get_y_size(); y++) {
        PlanarCoord first_coord(x, y);
        PlanarCoord second_coord = is_horizontal ? PlanarCoord(x + 1, y) : PlanarCoord(x, y + 1);
        PlanarRect edge_rect = RTUTIL.getBoundingBox({RTUTIL.getRealRectByGCell(first_coord, gcell_axis),
                                                       RTUTIL.getRealRectByGCell(second_coord, gcell_axis)});
        overflow_json_list.push_back({edge_rect.get_ll_x(), edge_rect.get_ll_y(), edge_rect.get_ur_x(), edge_rect.get_ur_y(),
                                      edge_map_pair.first, routing_edge_map[x][y].get_overflow()});
      }
    }
  }
  std::string overflow_json_file_path = RTUTIL.getString(pr_temp_directory_path, "overflow_map.json");
  std::ofstream* overflow_json_file = RTUTIL.getOutputFileStream(overflow_json_file_path);
  (*overflow_json_file) << overflow_json_list;
  RTUTIL.closeFileStream(overflow_json_file);
  return overflow_json_file_path;
}

std::string PlanarRouter::outputSummaryJson(PRModel& pr_model)
{
  Summary& summary = RTDM.getDatabase().get_summary();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  double& total_demand = summary.pr_summary.total_demand;
  double& total_overflow = summary.pr_summary.total_overflow;
  double& total_wire_length = summary.pr_summary.total_wire_length;
  std::map<std::string, std::map<std::string, double>>& clock_timing_map = summary.pr_summary.clock_timing_map;

  nlohmann::json summary_json;
  summary_json["total_demand"] = total_demand;
  summary_json["total_overflow"] = total_overflow;
  summary_json["total_wire_length"] = total_wire_length;
  for (auto& [clock_name, timing] : clock_timing_map) {
    summary_json["clock_timing_map"]["clock_name"] = clock_name;
    summary_json["clock_timing_map"]["timing"] = timing;
  }
  std::string summary_json_file_path = RTUTIL.getString(pr_temp_directory_path, "summary.json");
  std::ofstream* summary_json_file = RTUTIL.getOutputFileStream(summary_json_file_path);
  (*summary_json_file) << summary_json;
  RTUTIL.closeFileStream(summary_json_file);
  return summary_json_file_path;
}

// debug

void PlanarRouter::debugPlotPRModel(PRModel& pr_model, std::string flag)
{
  ScaleAxis& gcell_axis = RTDM.getDatabase().get_gcell_axis();
  Die& die = RTDM.getDatabase().get_die();
  std::vector<RoutingLayer>& routing_layer_list = RTDM.getDatabase().get_routing_layer_list();
  std::string& pr_temp_directory_path = RTDM.getConfig().pr_temp_directory_path;

  int32_t point_size = 5;

  GPGDS gp_gds;

  // base_region
  {
    GPStruct base_region_struct("base_region");
    GPBoundary gp_boundary;
    gp_boundary.set_layer_idx(0);
    gp_boundary.set_data_type(0);
    gp_boundary.set_rect(die.get_real_rect());
    base_region_struct.push(gp_boundary);
    gp_gds.addStruct(base_region_struct);
  }

  // gcell_axis
  {
    GPStruct gcell_axis_struct("gcell_axis");
    std::vector<int32_t> gcell_x_list = RTUTIL.getScaleList(die.get_real_ll_x(), die.get_real_ur_x(), gcell_axis.get_x_grid_list());
    std::vector<int32_t> gcell_y_list = RTUTIL.getScaleList(die.get_real_ll_y(), die.get_real_ur_y(), gcell_axis.get_y_grid_list());
    for (int32_t x : gcell_x_list) {
      GPPath gp_path;
      gp_path.set_layer_idx(0);
      gp_path.set_data_type(1);
      gp_path.set_segment(x, die.get_real_ll_y(), x, die.get_real_ur_y());
      gcell_axis_struct.push(gp_path);
    }
    for (int32_t y : gcell_y_list) {
      GPPath gp_path;
      gp_path.set_layer_idx(0);
      gp_path.set_data_type(1);
      gp_path.set_segment(die.get_real_ll_x(), y, die.get_real_ur_x(), y);
      gcell_axis_struct.push(gp_path);
    }
    gp_gds.addStruct(gcell_axis_struct);
  }

  // track_axis_struct
  {
    GPStruct track_axis_struct("track_axis_struct");
    for (RoutingLayer& routing_layer : routing_layer_list) {
      std::vector<int32_t> x_list = RTUTIL.getScaleList(die.get_real_ll_x(), die.get_real_ur_x(), routing_layer.getXTrackGridList());
      std::vector<int32_t> y_list = RTUTIL.getScaleList(die.get_real_ll_y(), die.get_real_ur_y(), routing_layer.getYTrackGridList());
      for (int32_t x : x_list) {
        GPPath gp_path;
        gp_path.set_data_type(static_cast<int32_t>(GPDataType::kAxis));
        gp_path.set_segment(x, die.get_real_ll_y(), x, die.get_real_ur_y());
        gp_path.set_layer_idx(RTGP.getGDSIdxByRouting(routing_layer.get_layer_idx()));
        track_axis_struct.push(gp_path);
      }
      for (int32_t y : y_list) {
        GPPath gp_path;
        gp_path.set_data_type(static_cast<int32_t>(GPDataType::kAxis));
        gp_path.set_segment(die.get_real_ll_x(), y, die.get_real_ur_x(), y);
        gp_path.set_layer_idx(RTGP.getGDSIdxByRouting(routing_layer.get_layer_idx()));
        track_axis_struct.push(gp_path);
      }
    }
    gp_gds.addStruct(track_axis_struct);
  }

  // fixed_rect
  for (auto& [is_routing, layer_net_rect_map] : RTDM.getTypeLayerNetFixedRectMap(die)) {
    for (auto& [layer_idx, net_rect_map] : layer_net_rect_map) {
      for (auto& [net_idx, rect_set] : net_rect_map) {
        GPStruct fixed_rect_struct(RTUTIL.getString("fixed_rect(net_", net_idx, ")"));
        for (EXTLayerRect* rect : rect_set) {
          GPBoundary gp_boundary;
          gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kShape));
          gp_boundary.set_rect(rect->get_real_rect());
          if (is_routing) {
            gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(layer_idx));
          } else {
            gp_boundary.set_layer_idx(RTGP.getGDSIdxByCut(layer_idx));
          }
          fixed_rect_struct.push(gp_boundary);
        }
        gp_gds.addStruct(fixed_rect_struct);
      }
    }
  }

  // access_point
  for (auto& [net_idx, access_point_set] : RTDM.getNetAccessPointMap(die)) {
    GPStruct access_point_struct(RTUTIL.getString("access_point(net_", net_idx, ")"));
    for (AccessPoint* access_point : access_point_set) {
      int32_t x = access_point->get_real_x();
      int32_t y = access_point->get_real_y();

      GPBoundary access_point_boundary;
      access_point_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(access_point->get_layer_idx()));
      access_point_boundary.set_data_type(static_cast<int32_t>(GPDataType::kAccessPoint));
      access_point_boundary.set_rect(x - point_size, y - point_size, x + point_size, y + point_size);
      access_point_struct.push(access_point_boundary);
    }
    gp_gds.addStruct(access_point_struct);
  }

  // routing result
  for (auto& [net_idx, segment_set] : RTDM.getNetGlobalResultMap(die)) {
    GPStruct global_result_struct(RTUTIL.getString("global_result(net_", net_idx, ")"));
    for (Segment<LayerCoord>* segment : segment_set) {
      for (NetShape& net_shape : RTDM.getNetGlobalShapeList(net_idx, *segment)) {
        GPBoundary gp_boundary;
        gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kGlobalPath));
        gp_boundary.set_rect(net_shape.get_rect());
        if (net_shape.get_is_routing()) {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(net_shape.get_layer_idx()));
        } else {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByCut(net_shape.get_layer_idx()));
        }
        global_result_struct.push(gp_boundary);
      }
    }
    gp_gds.addStruct(global_result_struct);
  }

  // routing result
  for (auto& [net_idx, segment_set] : RTDM.getNetDetailedResultMap(die)) {
    GPStruct detailed_result_struct(RTUTIL.getString("detailed_result(net_", net_idx, ")"));
    for (Segment<LayerCoord>* segment : segment_set) {
      for (NetShape& net_shape : RTDM.getNetDetailedShapeList(net_idx, *segment)) {
        GPBoundary gp_boundary;
        gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kShape));
        gp_boundary.set_rect(net_shape.get_rect());
        if (net_shape.get_is_routing()) {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(net_shape.get_layer_idx()));
        } else {
          gp_boundary.set_layer_idx(RTGP.getGDSIdxByCut(net_shape.get_layer_idx()));
        }
        detailed_result_struct.push(gp_boundary);
      }
    }
    gp_gds.addStruct(detailed_result_struct);
  }

  // routing patch
  for (auto& [net_idx, patch_set] : RTDM.getNetDetailedPatchMap(die)) {
    GPStruct detailed_patch_struct(RTUTIL.getString("detailed_patch(net_", net_idx, ")"));
    for (EXTLayerRect* patch : patch_set) {
      GPBoundary gp_boundary;
      gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kShape));
      gp_boundary.set_rect(patch->get_real_rect());
      gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(patch->get_layer_idx()));
      detailed_patch_struct.push(gp_boundary);
    }
    gp_gds.addStruct(detailed_patch_struct);
  }

  GPStruct overflow_struct("overflow");
  for (std::pair<GridMap<RoutingEdge>*, bool> edge_map_pair : {
           std::make_pair(&RTDM.getDatabase().get_planar_routing_h_edge_map(), true),
           std::make_pair(&RTDM.getDatabase().get_planar_routing_v_edge_map(), false)}) {
    GridMap<RoutingEdge>& routing_edge_map = *edge_map_pair.first;
    for (int32_t x = 0; x < routing_edge_map.get_x_size(); x++) {
      for (int32_t y = 0; y < routing_edge_map.get_y_size(); y++) {
        RoutingEdge& routing_edge = routing_edge_map[x][y];
        if (routing_edge.get_overflow() <= 0) {
          continue;
        }
        PlanarCoord first_coord(x, y);
        PlanarCoord second_coord = edge_map_pair.second ? PlanarCoord(x + 1, y) : PlanarCoord(x, y + 1);
        PlanarRect edge_rect = RTUTIL.getBoundingBox(
            {RTUTIL.getRealRectByGCell(first_coord, gcell_axis), RTUTIL.getRealRectByGCell(second_coord, gcell_axis)});

        GPBoundary gp_boundary;
        gp_boundary.set_data_type(static_cast<int32_t>(GPDataType::kOverflow));
        gp_boundary.set_rect(edge_rect);
        gp_boundary.set_layer_idx(RTGP.getGDSIdxByRouting(0));
        overflow_struct.push(gp_boundary);
      }
    }
  }
  gp_gds.addStruct(overflow_struct);

  std::string gds_file_path = RTUTIL.getString(pr_temp_directory_path, flag, "_pr_model.gds");
  RTGP.plot(gp_gds, gds_file_path);
}

}  // namespace irt
