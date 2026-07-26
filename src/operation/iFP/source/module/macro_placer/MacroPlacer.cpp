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
#include "MacroPlacer.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"

namespace ifp {

// public

void MacroPlacer::initInst()
{
  if (_mp_instance == nullptr) {
    _mp_instance = new MacroPlacer();
  }
}

MacroPlacer& MacroPlacer::getInst()
{
  if (_mp_instance == nullptr) {
    FPLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_mp_instance;
}

void MacroPlacer::destroyInst()
{
  if (_mp_instance != nullptr) {
    delete _mp_instance;
    _mp_instance = nullptr;
  }
}

// function

void MacroPlacer::place()
{
  Monitor monitor;
  FPLOG.info(Loc::current(), "Starting...");

  MPModel mp_model = initMPModel();
  setMPComParam(mp_model);
  if (mp_model.get_mp_com_param().get_max_iter() > 0 && !mp_model.get_movable_node_idx_list().empty()) {
    optimize(mp_model);
    uploadPlacementResult(mp_model);
  }
  FPLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

MPModel MacroPlacer::initMPModel()
{
  MPModel mp_model;
  buildModel(mp_model);
  return mp_model;
}

void MacroPlacer::buildModel(MPModel& mp_model)
{
  mp_model.clear();

  Core& core = FPDM.getDatabase().get_core();
  mp_model.set_core_rect(MPRect(core.get_ll_x(), core.get_ll_y(), core.get_ur_x(), core.get_ur_y()));

  buildNodeList(mp_model);
  buildNetList(mp_model);
}

void MacroPlacer::buildNodeList(MPModel& mp_model)
{
  Database& database = FPDM.getDatabase();
  for (Instance& instance : database.get_instance_list()) {
    if (!instance.get_macro()) {
      continue;
    }

    MPNode mp_node;
    mp_node.set_name(instance.get_name());
    mp_node.set_width(instance.get_width());
    mp_node.set_height(instance.get_height());
    mp_node.set_fixed(instance.get_fixed() || instance.get_cover());
    mp_node.set_placed(instance.get_placed());
    if (instance.get_placed()) {
      mp_node.set_coord(instance.get_x(), instance.get_y());
    } else {
      mp_node.set_coord(mp_model.get_core_rect().get_lx(), mp_model.get_core_rect().get_ly());
    }

    int32_t node_idx = static_cast<int32_t>(mp_model.get_mp_node_list().size());
    mp_model.get_mp_node_list().push_back(mp_node);
    if (!mp_node.get_fixed()) {
      mp_model.get_movable_node_idx_list().push_back(node_idx);
    }
  }
}

void MacroPlacer::buildNetList(MPModel& mp_model)
{
  std::unordered_map<std::string, int32_t> instance_name_to_node_idx;
  for (int32_t node_idx = 0; node_idx < static_cast<int32_t>(mp_model.get_mp_node_list().size()); node_idx++) {
    instance_name_to_node_idx[mp_model.get_mp_node_list()[node_idx].get_name()] = node_idx;
  }

  for (Net& net : FPDM.getDatabase().get_net_list()) {
    if (net.get_clock() || net.get_pdn() || net.get_power() || net.get_ground() || net.get_pin_num() > 300) {
      continue;
    }

    MPNet mp_net;
    mp_net.set_name(net.get_name());
    bool has_macro = false;
    for (NetPin& net_pin : net.get_net_pin_list()) {
      if (net_pin.get_io()) {
        if (!net_pin.get_placed()) {
          continue;
        }
        MPPin mp_pin;
        mp_pin.set_coord(net_pin.get_x(), net_pin.get_y());
        mp_pin.set_io(true);
        mp_net.get_mp_pin_list().push_back(mp_pin);
        continue;
      }

      std::unordered_map<std::string, int32_t>::iterator node_iter = instance_name_to_node_idx.find(net_pin.get_instance_name());
      if (node_iter != instance_name_to_node_idx.end()) {
        MPNode& mp_node = mp_model.get_mp_node_list()[node_iter->second];
        MPPin mp_pin;
        mp_pin.set_node_idx(node_iter->second);
        if (mp_node.get_placed()) {
          mp_pin.set_offset_x(net_pin.get_x() - mp_node.get_x());
          mp_pin.set_offset_y(net_pin.get_y() - mp_node.get_y());
        } else {
          mp_pin.set_offset_x(net_pin.get_offset_x());
          mp_pin.set_offset_y(net_pin.get_offset_y());
        }
        mp_net.get_mp_pin_list().push_back(mp_pin);
        has_macro = true;
      } else if (net_pin.get_placed()) {
        MPPin mp_pin;
        mp_pin.set_coord(net_pin.get_x(), net_pin.get_y());
        mp_net.get_mp_pin_list().push_back(mp_pin);
      }
    }
    if (has_macro && mp_net.get_mp_pin_list().size() > 1) {
      mp_model.get_mp_net_list().push_back(mp_net);
    }
  }
}

void MacroPlacer::setMPComParam(MPModel& mp_model)
{
  /**
   * wirelength_weight, overlap_weight, out_of_bound_weight, periphery_weight, io_weight, max_iter, cool_rate, initial_temperature
   */
  MPComParam mp_com_param(1.0, 0.05, 0.02, 0.05, 0.0, -1, 0.96, 2000.0);
  FPLOG.info(Loc::current(), "wirelength_weight: ", mp_com_param.get_wirelength_weight());
  FPLOG.info(Loc::current(), "overlap_weight: ", mp_com_param.get_overlap_weight());
  FPLOG.info(Loc::current(), "out_of_bound_weight: ", mp_com_param.get_out_of_bound_weight());
  FPLOG.info(Loc::current(), "periphery_weight: ", mp_com_param.get_periphery_weight());
  FPLOG.info(Loc::current(), "io_weight: ", mp_com_param.get_io_weight());
  FPLOG.info(Loc::current(), "max_iter: ", mp_com_param.get_max_iter());
  FPLOG.info(Loc::current(), "cool_rate: ", mp_com_param.get_cool_rate());
  FPLOG.info(Loc::current(), "initial_temperature: ", mp_com_param.get_initial_temperature());
  mp_model.set_mp_com_param(mp_com_param);
}

void MacroPlacer::optimize(MPModel& mp_model)
{
  initializeNodeLocation(mp_model);

  std::vector<MPNode> current_node_list = mp_model.get_mp_node_list();
  std::vector<MPNode> best_node_list = current_node_list;
  double current_cost = calculateCost(mp_model, current_node_list);
  double best_cost = current_cost;
  double current_conflict = calculateOverlap(mp_model, current_node_list);
  double best_conflict = current_conflict;

  MPComParam& mp_com_param = mp_model.get_mp_com_param();
  std::vector<MPIterParam> mp_iter_param_list;
  double temperature = mp_com_param.get_initial_temperature();
  for (int32_t iter = 0; iter < mp_com_param.get_max_iter(); iter++) {
    mp_iter_param_list.emplace_back(temperature);
    temperature = std::max(temperature * mp_com_param.get_cool_rate(), std::numeric_limits<double>::epsilon());
  }

  std::mt19937 random_generator(0);
  std::uniform_real_distribution<double> probability_distribution(0.0, 1.0);
  std::uniform_int_distribution<int32_t> movable_node_distribution(0, static_cast<int32_t>(mp_model.get_movable_node_idx_list().size()) - 1);

  for (int32_t i = 0, iter = 1; i < static_cast<int32_t>(mp_iter_param_list.size()); i++, iter++) {
    Monitor iter_monitor;
    FPLOG.info(Loc::current(), "***** Begin iteration ", iter, "/", mp_iter_param_list.size(), " *****");
    setMPIterParam(mp_model, iter, mp_iter_param_list[i]);
    double temperature = mp_model.get_mp_iter_param().get_temperature();
    std::vector<MPNode> candidate_node_list = current_node_list;
    int32_t first_node_idx = mp_model.get_movable_node_idx_list()[movable_node_distribution(random_generator)];
    if (mp_model.get_movable_node_idx_list().size() > 1 && probability_distribution(random_generator) < 0.5) {
      int32_t second_node_idx = mp_model.get_movable_node_idx_list()[movable_node_distribution(random_generator)];
      while (first_node_idx == second_node_idx) {
        second_node_idx = mp_model.get_movable_node_idx_list()[movable_node_distribution(random_generator)];
      }
      int32_t first_x = candidate_node_list[first_node_idx].get_x();
      int32_t first_y = candidate_node_list[first_node_idx].get_y();
      candidate_node_list[first_node_idx].set_coord(candidate_node_list[second_node_idx].get_x(), candidate_node_list[second_node_idx].get_y());
      candidate_node_list[second_node_idx].set_coord(first_x, first_y);
    } else {
      MPNode& mp_node = candidate_node_list[first_node_idx];
      double temperature_ratio = std::max(temperature / mp_com_param.get_initial_temperature(), 0.05);
      int32_t max_delta_x = std::max(1, static_cast<int32_t>(mp_model.get_core_rect().get_width() * temperature_ratio / 4.0));
      int32_t max_delta_y = std::max(1, static_cast<int32_t>(mp_model.get_core_rect().get_height() * temperature_ratio / 4.0));
      std::uniform_int_distribution<int32_t> delta_x_distribution(-max_delta_x, max_delta_x);
      std::uniform_int_distribution<int32_t> delta_y_distribution(-max_delta_y, max_delta_y);
      mp_node.set_coord(mp_node.get_x() + delta_x_distribution(random_generator), mp_node.get_y() + delta_y_distribution(random_generator));
    }

    for (int32_t node_idx : mp_model.get_movable_node_idx_list()) {
      MPNode& mp_node = candidate_node_list[node_idx];
      int32_t min_x = mp_model.get_core_rect().get_lx();
      int32_t min_y = mp_model.get_core_rect().get_ly();
      int32_t max_x = mp_model.get_core_rect().get_ux() - mp_node.get_width();
      int32_t max_y = mp_model.get_core_rect().get_uy() - mp_node.get_height();
      mp_node.set_x(std::clamp(mp_node.get_x(), min_x, std::max(min_x, max_x)));
      mp_node.set_y(std::clamp(mp_node.get_y(), min_y, std::max(min_y, max_y)));
    }

    double candidate_cost = calculateCost(mp_model, candidate_node_list);
    double candidate_conflict = calculateOverlap(mp_model, candidate_node_list);
    double cost_delta = candidate_cost - current_cost;
    bool better_conflict = candidate_conflict < current_conflict;
    bool same_conflict = std::abs(candidate_conflict - current_conflict) <= std::numeric_limits<double>::epsilon();
    if (better_conflict || (same_conflict && (cost_delta <= 0.0 || probability_distribution(random_generator) < std::exp(-cost_delta / temperature)))) {
      current_node_list = candidate_node_list;
      current_cost = candidate_cost;
      current_conflict = candidate_conflict;
      if (current_conflict < best_conflict || (std::abs(current_conflict - best_conflict) <= std::numeric_limits<double>::epsilon()
                                               && current_cost < best_cost)) {
        best_node_list = current_node_list;
        best_cost = current_cost;
        best_conflict = current_conflict;
      }
    }
    FPLOG.info(Loc::current(), "***** End iteration ", iter, "/", mp_iter_param_list.size(), " ", iter_monitor.getStatsInfo(), " *****");
  }

  mp_model.set_mp_node_list(best_node_list);
}

void MacroPlacer::initializeNodeLocation(MPModel& mp_model)
{
  int32_t next_x = mp_model.get_core_rect().get_lx();
  int32_t next_y = mp_model.get_core_rect().get_ly();
  int32_t row_height = 0;
  for (int32_t node_idx : mp_model.get_movable_node_idx_list()) {
    MPNode& mp_node = mp_model.get_mp_node_list()[node_idx];
    if (mp_node.get_placed()) {
      continue;
    }
    int32_t node_width = mp_node.get_width();
    int32_t node_height = mp_node.get_height();
    if (next_x + node_width > mp_model.get_core_rect().get_ux()) {
      next_x = mp_model.get_core_rect().get_lx();
      next_y += row_height;
      row_height = 0;
    }
    mp_node.set_coord(next_x, next_y);
    next_x += node_width;
    row_height = std::max(row_height, node_height);
  }
}

double MacroPlacer::calculateCost(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list)
{
  const MPComParam& mp_com_param = mp_model.get_mp_com_param();
  return mp_com_param.get_wirelength_weight() * calculateWirelength(mp_model, mp_node_list)
         + mp_com_param.get_overlap_weight() * calculateOverlap(mp_model, mp_node_list)
         + mp_com_param.get_out_of_bound_weight() * calculateOutOfBound(mp_model, mp_node_list)
         + mp_com_param.get_periphery_weight() * calculatePeriphery(mp_model, mp_node_list)
         + mp_com_param.get_io_weight() * calculateIODistance(mp_model, mp_node_list);
}

double MacroPlacer::calculateWirelength(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list)
{
  double wirelength = 0.0;
  double scale = std::max(1, mp_model.get_core_rect().get_width() + mp_model.get_core_rect().get_height());
  for (const MPNet& mp_net : mp_model.get_mp_net_list()) {
    int32_t min_x = INT32_MAX;
    int32_t min_y = INT32_MAX;
    int32_t max_x = INT32_MIN;
    int32_t max_y = INT32_MIN;
    for (const MPPin& mp_pin : mp_net.get_mp_pin_list()) {
      int32_t x = mp_pin.get_x();
      int32_t y = mp_pin.get_y();
      if (mp_pin.is_node_pin()) {
        const MPNode& mp_node = mp_node_list[mp_pin.get_node_idx()];
        x = mp_node.get_x() + mp_pin.get_offset_x();
        y = mp_node.get_y() + mp_pin.get_offset_y();
      }
      min_x = std::min(min_x, x);
      min_y = std::min(min_y, y);
      max_x = std::max(max_x, x);
      max_y = std::max(max_y, y);
    }
    wirelength += max_x - min_x + max_y - min_y;
  }
  return wirelength / scale;
}

double MacroPlacer::calculateOverlap(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list)
{
  double overlap = 0.0;
  double core_area = std::max<int64_t>(1, static_cast<int64_t>(mp_model.get_core_rect().get_width()) * mp_model.get_core_rect().get_height());
  for (int32_t first_node_idx = 0; first_node_idx < static_cast<int32_t>(mp_node_list.size()); first_node_idx++) {
    MPRect first_rect = getNodeRect(mp_node_list[first_node_idx]);
    for (int32_t second_node_idx = first_node_idx + 1; second_node_idx < static_cast<int32_t>(mp_node_list.size()); second_node_idx++) {
      if (mp_node_list[first_node_idx].get_fixed() && mp_node_list[second_node_idx].get_fixed()) {
        continue;
      }
      overlap += first_rect.get_overlap_area(getNodeRect(mp_node_list[second_node_idx]));
    }
  }
  return overlap / core_area;
}

MPRect MacroPlacer::getNodeRect(const MPNode& mp_node)
{
  return MPRect(mp_node.get_x(), mp_node.get_y(), mp_node.get_x() + mp_node.get_width(), mp_node.get_y() + mp_node.get_height());
}

double MacroPlacer::calculateOutOfBound(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list)
{
  double out_of_bound = 0.0;
  double core_area = std::max<int64_t>(1, static_cast<int64_t>(mp_model.get_core_rect().get_width()) * mp_model.get_core_rect().get_height());
  for (const MPNode& mp_node : mp_node_list) {
    if (mp_node.get_fixed()) {
      continue;
    }
    MPRect node_rect = getNodeRect(mp_node);
    int32_t overlap_lx = std::max(node_rect.get_lx(), mp_model.get_core_rect().get_lx());
    int32_t overlap_ly = std::max(node_rect.get_ly(), mp_model.get_core_rect().get_ly());
    int32_t overlap_ux = std::min(node_rect.get_ux(), mp_model.get_core_rect().get_ux());
    int32_t overlap_uy = std::min(node_rect.get_uy(), mp_model.get_core_rect().get_uy());
    int64_t node_area = static_cast<int64_t>(node_rect.get_width()) * node_rect.get_height();
    int64_t inside_area = overlap_ux > overlap_lx && overlap_uy > overlap_ly
                              ? static_cast<int64_t>(overlap_ux - overlap_lx) * (overlap_uy - overlap_ly)
                              : 0;
    out_of_bound += node_area - inside_area;
  }
  return out_of_bound / core_area;
}

double MacroPlacer::calculatePeriphery(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list)
{
  if (mp_model.get_movable_node_idx_list().empty()) {
    return 0.0;
  }
  double periphery = 0.0;
  double scale = std::max(1, std::min(mp_model.get_core_rect().get_width(), mp_model.get_core_rect().get_height()));
  for (int32_t node_idx : mp_model.get_movable_node_idx_list()) {
    MPRect node_rect = getNodeRect(mp_node_list[node_idx]);
    int32_t distance = std::min({node_rect.get_lx() - mp_model.get_core_rect().get_lx(), node_rect.get_ly() - mp_model.get_core_rect().get_ly(),
                                 mp_model.get_core_rect().get_ux() - node_rect.get_ux(), mp_model.get_core_rect().get_uy() - node_rect.get_uy()});
    periphery += static_cast<double>(distance) * distance;
  }
  return periphery / (scale * scale * mp_model.get_movable_node_idx_list().size());
}

double MacroPlacer::calculateIODistance(const MPModel& mp_model, const std::vector<MPNode>& mp_node_list)
{
  double io_distance = 0.0;
  double scale = std::max(1, mp_model.get_core_rect().get_width() + mp_model.get_core_rect().get_height());
  for (const MPNet& mp_net : mp_model.get_mp_net_list()) {
    for (const MPPin& io_pin : mp_net.get_mp_pin_list()) {
      if (!io_pin.get_io()) {
        continue;
      }
      for (const MPPin& mp_pin : mp_net.get_mp_pin_list()) {
        if (!mp_pin.is_node_pin()) {
          continue;
        }
        const MPNode& mp_node = mp_node_list[mp_pin.get_node_idx()];
        io_distance += std::abs(mp_node.get_x() + mp_pin.get_offset_x() - io_pin.get_x())
                       + std::abs(mp_node.get_y() + mp_pin.get_offset_y() - io_pin.get_y());
      }
    }
  }
  return io_distance / scale;
}

void MacroPlacer::setMPIterParam(MPModel& mp_model, int32_t iter, MPIterParam& mp_iter_param)
{
  mp_model.set_iter(iter);
  FPLOG.info(Loc::current(), "temperature: ", mp_iter_param.get_temperature());
  mp_model.set_mp_iter_param(mp_iter_param);
}

void MacroPlacer::uploadPlacementResult(MPModel& mp_model)
{
  Database& database = FPDM.getDatabase();
  for (int32_t node_idx : mp_model.get_movable_node_idx_list()) {
    MPNode& mp_node = mp_model.get_mp_node_list()[node_idx];
    Instance& instance = database.get_instance_list()[database.get_instance_name_to_idx_map()[mp_node.get_name()]];
    int32_t delta_x = mp_node.get_x() - instance.get_x();
    int32_t delta_y = mp_node.get_y() - instance.get_y();
    instance.set_coord(mp_node.get_x(), mp_node.get_y());
    if (instance.get_orient() == PlacementOrientation::kNone) {
      instance.set_orient(PlacementOrientation::kN);
    }
    PlanarRect& bounding_rect = instance.get_bounding_rect();
    if (bounding_rect.get_width() > 0 && bounding_rect.get_height() > 0) {
      bounding_rect.set_rect(bounding_rect.get_ll_x() + delta_x, bounding_rect.get_ll_y() + delta_y, bounding_rect.get_ur_x() + delta_x,
                             bounding_rect.get_ur_y() + delta_y);
    } else {
      instance.set_bounding_rect(instance.get_x(), instance.get_y(), instance.get_x() + instance.get_width(),
                                 instance.get_y() + instance.get_height());
    }
    for (InstancePinShape& pin_shape : instance.get_pin_shape_list()) {
      pin_shape.set_rect(pin_shape.get_ll_x() + delta_x, pin_shape.get_ll_y() + delta_y, pin_shape.get_ur_x() + delta_x,
                         pin_shape.get_ur_y() + delta_y);
    }
    instance.set_fixed(true);
    instance.set_cover(false);
    instance.set_placed(true);
    instance.set_placement_updated(true);
  }
}

// private

MacroPlacer* MacroPlacer::_mp_instance = nullptr;

}  // namespace ifp
