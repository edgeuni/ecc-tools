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
#include "TopoBuilder.hpp"

#include "LayerShape.hpp"
#include "Utility.hpp"

namespace ircx {

// public

void TopoBuilder::initInst()
{
  if (_tb_instance == nullptr) {
    _tb_instance = new TopoBuilder();
  }
}

TopoBuilder& TopoBuilder::getInst()
{
  if (_tb_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tb_instance;
}

void TopoBuilder::destroyInst()
{
  if (_tb_instance != nullptr) {
    delete _tb_instance;
    _tb_instance = nullptr;
  }
}

// function

void TopoBuilder::build()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  buildAll();
  buildSpecial();

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TopoBuilder* TopoBuilder::_tb_instance = nullptr;

void TopoBuilder::buildAll()
{
  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  std::vector<Net>& net_list = layout_data.get_net_list();
  size_t net_num = layout_data.get_regular_net_count();
  if (net_num == 0) {
    return;
  }

  std::vector<TBNetTopo> net_topo_list(net_num);
  int32_t thread_num = std::max(1, std::min(RCXDM.getConfig().thread_number, static_cast<int32_t>(net_num)));
#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
  for (size_t net_idx = 0; net_idx < net_num; net_idx++) {
    net_topo_list[net_idx] = buildNet(net_list[net_idx]);
  }

  size_t node_num = 0;
  size_t edge_num = 0;
  for (TBNetTopo& net_topo : net_topo_list) {
    node_num += net_topo.get_node_list().size();
    edge_num += net_topo.get_edge_list().size();
  }

  TopoPool& topo_pool = RCXDM.getDatabase().get_topo_pool();
  topo_pool.reserve(net_num, node_num, edge_num);
  for (TBNetTopo& net_topo : net_topo_list) {
    topo_pool.add_net(std::move(net_topo.get_node_list()), std::move(net_topo.get_edge_list()));
  }

#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
  for (size_t net_idx = 0; net_idx < net_num; net_idx++) {
    std::span<TopoEdge> edge_list = topo_pool.get_net_edge_list(net_idx);
    for (TopoEdge& edge : edge_list) {
      edge.set_start_node_idx(topo_pool.get_node_idx(net_idx, edge.get_start_node_idx()));
      edge.set_end_node_idx(topo_pool.get_node_idx(net_idx, edge.get_end_node_idx()));
    }
  }
}

TBNetTopo TopoBuilder::buildNet(Net& net)
{
  TBNetTopo net_topo;
  std::vector<TopoNode>& node_list = net_topo.get_node_list();
  std::vector<TopoEdge>& edge_list = net_topo.get_edge_list();
  std::vector<bool> node_shape_valid_list;
  std::map<std::tuple<size_t, int32_t, int32_t>, size_t> node_key_to_idx_map;
  std::map<std::string, bool> pin_consumed_map;

  for (Pin& pin : net.get_pin_list()) {
    pin_consumed_map[pin.get_pin_name()] = false;
  }

  for (Segment& segment : net.get_segment_list()) {
    appendNodeIfAbsent(net, node_list, node_shape_valid_list, node_key_to_idx_map, pin_consumed_map, segment.get_layer_id(),
                       segment.get_start_point());
    appendNodeIfAbsent(net, node_list, node_shape_valid_list, node_key_to_idx_map, pin_consumed_map, segment.get_layer_id(),
                       segment.get_end_point());
  }
  for (Via& via : net.get_via_list()) {
    appendNodeIfAbsent(net, node_list, node_shape_valid_list, node_key_to_idx_map, pin_consumed_map,
                       via.get_top_layer_shape().get_layer_id(), via.get_point());
    appendNodeIfAbsent(net, node_list, node_shape_valid_list, node_key_to_idx_map, pin_consumed_map,
                       via.get_bottom_layer_shape().get_layer_id(), via.get_point());
  }

  for (Segment& segment : net.get_segment_list()) {
    size_t layer_id = segment.get_layer_id();
    size_t start_node_idx = node_key_to_idx_map.at(
        std::make_tuple(layer_id, RCXUTIL.x(segment.get_start_point()), RCXUTIL.y(segment.get_start_point())));
    size_t end_node_idx = node_key_to_idx_map.at(
        std::make_tuple(layer_id, RCXUTIL.x(segment.get_end_point()), RCXUTIL.y(segment.get_end_point())));
    mergeNodeShape(node_list, node_shape_valid_list, start_node_idx, getEndpointShape(segment, segment.get_start_point()));
    mergeNodeShape(node_list, node_shape_valid_list, end_node_idx, getEndpointShape(segment, segment.get_end_point()));

    TopoEdge edge(net.get_net_id());
    edge.set_layer_id(layer_id);
    if (RCXUTIL.x(segment.get_start_point()) < RCXUTIL.x(segment.get_end_point())
        || (RCXUTIL.x(segment.get_start_point()) == RCXUTIL.x(segment.get_end_point())
            && RCXUTIL.y(segment.get_start_point()) <= RCXUTIL.y(segment.get_end_point()))) {
      edge.set_start_node_idx(start_node_idx);
      edge.set_end_node_idx(end_node_idx);
    } else {
      edge.set_start_node_idx(end_node_idx);
      edge.set_end_node_idx(start_node_idx);
    }
    edge.set_shape(segment.get_shape());
    edge_list.push_back(std::move(edge));
  }

  for (Via& via : net.get_via_list()) {
    size_t top_layer_id = via.get_top_layer_shape().get_layer_id();
    size_t bottom_layer_id = via.get_bottom_layer_shape().get_layer_id();
    size_t top_node_idx = node_key_to_idx_map.at(
        std::make_tuple(top_layer_id, RCXUTIL.x(via.get_point()), RCXUTIL.y(via.get_point())));
    size_t bottom_node_idx = node_key_to_idx_map.at(
        std::make_tuple(bottom_layer_id, RCXUTIL.x(via.get_point()), RCXUTIL.y(via.get_point())));
    mergeNodeShape(node_list, node_shape_valid_list, top_node_idx, via.get_top_layer_shape().get_shape());
    mergeNodeShape(node_list, node_shape_valid_list, bottom_node_idx, via.get_bottom_layer_shape().get_shape());

    TopoEdge edge(net.get_net_id());
    edge.set_layer_id(via.get_cut_layer_shape().get_layer_id());
    edge.set_start_node_idx(top_node_idx);
    edge.set_end_node_idx(bottom_node_idx);
    edge.set_shape(via.get_cut_layer_shape().get_shape());
    edge.set_via_name(via.get_via_name());
    edge_list.push_back(std::move(edge));
  }
  return net_topo;
}

void TopoBuilder::appendNodeIfAbsent(Net& net,
                                         std::vector<TopoNode>& node_list,
                                         std::vector<bool>& node_shape_valid_list,
                                         std::map<std::tuple<size_t, int32_t, int32_t>, size_t>& node_key_to_idx_map,
                                         std::map<std::string, bool>& pin_consumed_map,
                                         size_t layer_id,
                                         const GTLPointInt& point)
{
  std::tuple<size_t, int32_t, int32_t> node_key = std::make_tuple(layer_id, RCXUTIL.x(point), RCXUTIL.y(point));
  if (node_key_to_idx_map.count(node_key) != 0) {
    return;
  }

  TopoNode node(net.get_net_id());
  node.set_layer_id(layer_id);
  node.set_point(point);

  bool is_shape_valid = false;
  for (Pin& pin : net.get_pin_list()) {
    if (pin_consumed_map[pin.get_pin_name()]) {
      continue;
    }
    for (LayerShape& layer_shape : pin.get_layer_shape_list()) {
      if (layer_shape.get_layer_id() != layer_id) {
        continue;
      }

      GTLRectInt& pin_shape = layer_shape.get_shape();
      if (RCXUTIL.minX(pin_shape) <= RCXUTIL.x(point) && RCXUTIL.x(point) <= RCXUTIL.maxX(pin_shape)
          && RCXUTIL.minY(pin_shape) <= RCXUTIL.y(point) && RCXUTIL.y(point) <= RCXUTIL.maxY(pin_shape)) {
        pin_consumed_map[pin.get_pin_name()] = true;
        node.set_pin_name(pin.get_pin_name());
        node.set_shape(pin_shape);
        is_shape_valid = true;
        break;
      }
    }
    if (is_shape_valid) {
      break;
    }
  }
  if (!is_shape_valid) {
    node.set_shape(GTLRectInt(RCXUTIL.x(point) - 1, RCXUTIL.y(point) - 1, RCXUTIL.x(point) + 1,
                            RCXUTIL.y(point) + 1));
  }

  node_key_to_idx_map[node_key] = appendNode(node_list, node_shape_valid_list, std::move(node), is_shape_valid);
}

size_t TopoBuilder::appendNode(std::vector<TopoNode>& node_list,
                                 std::vector<bool>& node_shape_valid_list,
                                 TopoNode node,
                                 bool is_shape_valid)
{
  node_list.push_back(std::move(node));
  node_shape_valid_list.push_back(is_shape_valid);
  return node_list.size() - 1;
}

void TopoBuilder::mergeNodeShape(std::vector<TopoNode>& node_list,
                                     std::vector<bool>& node_shape_valid_list,
                                     size_t node_idx,
                                     const GTLRectInt& shape)
{
  TopoNode& node = node_list[node_idx];
  if (!node_shape_valid_list[node_idx]) {
    node.set_shape(shape);
    node_shape_valid_list[node_idx] = true;
    return;
  }

  GTLRectInt& old_shape = node.get_shape();
  node.set_shape(GTLRectInt(std::min(RCXUTIL.minX(old_shape), RCXUTIL.minX(shape)),
                          std::min(RCXUTIL.minY(old_shape), RCXUTIL.minY(shape)),
                          std::max(RCXUTIL.maxX(old_shape), RCXUTIL.maxX(shape)),
                          std::max(RCXUTIL.maxY(old_shape), RCXUTIL.maxY(shape))));
}

GTLRectInt TopoBuilder::getEndpointShape(Segment& segment, const GTLPointInt& point)
{
  bool is_horizontal = std::abs(RCXUTIL.x(segment.get_start_point()) - RCXUTIL.x(segment.get_end_point()))
                       >= std::abs(RCXUTIL.y(segment.get_start_point()) - RCXUTIL.y(segment.get_end_point()));
  GTLRectInt& shape = segment.get_shape();
  if (is_horizontal) {
    return GTLRectInt(RCXUTIL.x(point), RCXUTIL.minY(shape), RCXUTIL.x(point), RCXUTIL.maxY(shape));
  }
  return GTLRectInt(RCXUTIL.minX(shape), RCXUTIL.y(point), RCXUTIL.maxX(shape), RCXUTIL.y(point));
}

void TopoBuilder::buildSpecial()
{
  Net& special_net = RCXDM.getDatabase().get_layout_data().get_special_net();
  std::vector<TopoEdge> special_edge_list;
  special_edge_list.reserve(special_net.get_segment_list().size() + special_net.get_patch_list().size());

  for (Segment& segment : special_net.get_segment_list()) {
    TopoEdge edge;
    edge.set_layer_id(segment.get_layer_id());
    edge.set_shape(segment.get_shape());
    special_edge_list.push_back(std::move(edge));
  }
  for (Patch& patch : special_net.get_patch_list()) {
    TopoEdge edge;
    edge.set_layer_id(patch.get_layer_id());
    edge.set_shape(patch.get_shape());
    special_edge_list.push_back(std::move(edge));
  }
  RCXDM.getDatabase().get_topo_pool().add_special_edge_list(std::move(special_edge_list));
}

}  // namespace ircx
