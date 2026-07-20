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
#include "TopologyBuilder.hpp"

namespace ircx {

// public

void TopologyBuilder::initInst()
{
  if (_tb_instance == nullptr) {
    _tb_instance = new TopologyBuilder();
  }
}

TopologyBuilder& TopologyBuilder::getInst()
{
  if (_tb_instance == nullptr) {
    RCXLOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tb_instance;
}

void TopologyBuilder::destroyInst()
{
  if (_tb_instance != nullptr) {
    delete _tb_instance;
    _tb_instance = nullptr;
  }
}

// function

void TopologyBuilder::build()
{
  Monitor monitor;
  RCXLOG.info(Loc::current(), "Starting...");

  TBModel tb_model = initTBModel();
  buildTBModel(tb_model);

  RCXLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TopologyBuilder* TopologyBuilder::_tb_instance = nullptr;

TBModel TopologyBuilder::initTBModel()
{
  TBModel tb_model;
  tb_model.set_database(&RCXDM.getDatabase());
  tb_model.set_temp_directory_path(RCXDM.getConfig().tb_temp_directory_path);
  return tb_model;
}

void TopologyBuilder::buildTBModel(TBModel&)
{
  buildAll();
  buildSpecial();
}

void TopologyBuilder::buildAll()
{
  LayoutData& layout_data = RCXDM.getDatabase().get_layout_data();
  std::vector<Net>& net_list = layout_data.get_net_list();
  Size net_num = layout_data.get_regular_net_count();
  if (net_num == 0) {
    return;
  }

  std::vector<TBNetTopology> net_topology_list(net_num);
  int32_t thread_num = std::max(1, std::min(RCXDM.getConfig().thread_number, static_cast<int32_t>(net_num)));
#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
  for (Size net_idx = 0; net_idx < net_num; net_idx++) {
    net_topology_list[net_idx] = buildNet(net_list[net_idx]);
  }

  Size node_num = 0;
  Size edge_num = 0;
  for (TBNetTopology& net_topology : net_topology_list) {
    node_num += net_topology.get_node_list().size();
    edge_num += net_topology.get_edge_list().size();
  }

  TopoPool& topo_pool = RCXDM.getDatabase().get_topo_pool();
  topo_pool.reserve(net_num, node_num, edge_num);
  for (TBNetTopology& net_topology : net_topology_list) {
    topo_pool.add_net(std::move(net_topology.get_node_list()), std::move(net_topology.get_edge_list()));
  }

#pragma omp parallel for schedule(dynamic) num_threads(thread_num)
  for (Size net_idx = 0; net_idx < net_num; net_idx++) {
    std::span<TopoEdge> edge_list = topo_pool.get_net_edge_list(net_idx);
    for (TopoEdge& edge : edge_list) {
      edge.set_start_node_idx(topo_pool.get_node_idx(net_idx, edge.get_start_node_idx()));
      edge.set_end_node_idx(topo_pool.get_node_idx(net_idx, edge.get_end_node_idx()));
    }
  }
}

TBNetTopology TopologyBuilder::buildNet(Net& net)
{
  TBNetTopology net_topology;
  std::vector<TopoNode>& node_list = net_topology.get_node_list();
  std::vector<TopoEdge>& edge_list = net_topology.get_edge_list();
  std::vector<bool> node_shape_valid_list;
  std::map<std::tuple<Size, Dbu, Dbu>, Size> node_key_to_idx_map;
  std::map<std::string, bool> pin_consumed_map;

  for (Pin& pin : net.get_pin_list()) {
    pin_consumed_map[pin.get_pin_name()] = false;
  }

  auto appendNode = [&node_list, &node_shape_valid_list](TopoNode node, bool is_shape_valid) {
    node_list.push_back(std::move(node));
    node_shape_valid_list.push_back(is_shape_valid);
    return node_list.size() - 1;
  };
  auto mergeNodeShape = [&node_list, &node_shape_valid_list](Size node_idx, const GtlRectI& shape) {
    TopoNode& node = node_list[node_idx];
    if (!node_shape_valid_list[node_idx]) {
      node.set_shape(shape);
      node_shape_valid_list[node_idx] = true;
      return;
    }

    GtlRectI& old_shape = node.get_shape();
    node.set_shape(GtlRectI(std::min(boost::polygon::xl(old_shape), boost::polygon::xl(shape)),
                            std::min(boost::polygon::yl(old_shape), boost::polygon::yl(shape)),
                            std::max(boost::polygon::xh(old_shape), boost::polygon::xh(shape)),
                            std::max(boost::polygon::yh(old_shape), boost::polygon::yh(shape))));
  };
  auto getEndpointShape = [](Segment& segment, const GtlPointI& point) {
    bool is_horizontal = std::abs(boost::polygon::x(segment.get_start_point()) - boost::polygon::x(segment.get_end_point()))
                         >= std::abs(boost::polygon::y(segment.get_start_point()) - boost::polygon::y(segment.get_end_point()));
    GtlRectI& shape = segment.get_shape();
    if (is_horizontal) {
      return GtlRectI(boost::polygon::x(point), boost::polygon::yl(shape), boost::polygon::x(point), boost::polygon::yh(shape));
    }
    return GtlRectI(boost::polygon::xl(shape), boost::polygon::y(point), boost::polygon::xh(shape), boost::polygon::y(point));
  };
  auto appendNodeIfAbsent = [&](Size layer_id, const GtlPointI& point) {
    std::tuple<Size, Dbu, Dbu> node_key = std::make_tuple(layer_id, boost::polygon::x(point), boost::polygon::y(point));
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
        if (layer_shape.first != layer_id) {
          continue;
        }

        GtlRectI& pin_shape = layer_shape.second;
        if (boost::polygon::xl(pin_shape) <= boost::polygon::x(point) && boost::polygon::x(point) <= boost::polygon::xh(pin_shape)
            && boost::polygon::yl(pin_shape) <= boost::polygon::y(point) && boost::polygon::y(point) <= boost::polygon::yh(pin_shape)) {
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
      node.set_shape(GtlRectI(boost::polygon::x(point) - 1, boost::polygon::y(point) - 1, boost::polygon::x(point) + 1, boost::polygon::y(point) + 1));
    }

    node_key_to_idx_map[node_key] = appendNode(std::move(node), is_shape_valid);
  };

  for (Segment& segment : net.get_segment_list()) {
    appendNodeIfAbsent(segment.get_layer_id(), segment.get_start_point());
    appendNodeIfAbsent(segment.get_layer_id(), segment.get_end_point());
  }
  for (Via& via : net.get_via_list()) {
    appendNodeIfAbsent(via.get_top_layer_shape().first, via.get_point());
    appendNodeIfAbsent(via.get_bottom_layer_shape().first, via.get_point());
  }

  for (Segment& segment : net.get_segment_list()) {
    Size layer_id = segment.get_layer_id();
    Size start_node_idx = node_key_to_idx_map.at(std::make_tuple(layer_id, boost::polygon::x(segment.get_start_point()), boost::polygon::y(segment.get_start_point())));
    Size end_node_idx = node_key_to_idx_map.at(std::make_tuple(layer_id, boost::polygon::x(segment.get_end_point()), boost::polygon::y(segment.get_end_point())));
    mergeNodeShape(start_node_idx, getEndpointShape(segment, segment.get_start_point()));
    mergeNodeShape(end_node_idx, getEndpointShape(segment, segment.get_end_point()));

    TopoEdge edge(net.get_net_id());
    edge.set_layer_id(layer_id);
    if (boost::polygon::x(segment.get_start_point()) < boost::polygon::x(segment.get_end_point())
        || (boost::polygon::x(segment.get_start_point()) == boost::polygon::x(segment.get_end_point())
            && boost::polygon::y(segment.get_start_point()) <= boost::polygon::y(segment.get_end_point()))) {
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
    Size top_layer_id = via.get_top_layer_shape().first;
    Size bottom_layer_id = via.get_bottom_layer_shape().first;
    Size top_node_idx = node_key_to_idx_map.at(std::make_tuple(top_layer_id, boost::polygon::x(via.get_point()), boost::polygon::y(via.get_point())));
    Size bottom_node_idx = node_key_to_idx_map.at(std::make_tuple(bottom_layer_id, boost::polygon::x(via.get_point()), boost::polygon::y(via.get_point())));
    mergeNodeShape(top_node_idx, via.get_top_layer_shape().second);
    mergeNodeShape(bottom_node_idx, via.get_bottom_layer_shape().second);

    TopoEdge edge(net.get_net_id());
    edge.set_layer_id(via.get_cut_layer_shape().first);
    edge.set_start_node_idx(top_node_idx);
    edge.set_end_node_idx(bottom_node_idx);
    edge.set_shape(via.get_cut_layer_shape().second);
    edge.set_via_name(via.get_via_name());
    edge_list.push_back(std::move(edge));
  }
  return net_topology;
}

void TopologyBuilder::buildSpecial()
{
  Net& special_net = RCXDM.getDatabase().get_layout_data().get_special_net();
  std::vector<TopoEdge> special_edge_list;
  special_edge_list.reserve(special_net.get_segment_list().size() + special_net.get_patch_list().size());

  for (Segment& segment : special_net.get_segment_list()) {
    TopoEdge edge(kSpecialNetId);
    edge.set_layer_id(segment.get_layer_id());
    edge.set_shape(segment.get_shape());
    special_edge_list.push_back(std::move(edge));
  }
  for (Patch& patch : special_net.get_patch_list()) {
    TopoEdge edge(kSpecialNetId);
    edge.set_layer_id(patch.get_layer_id());
    edge.set_shape(patch.get_shape());
    special_edge_list.push_back(std::move(edge));
  }
  RCXDM.getDatabase().get_topo_pool().add_special_edge_list(std::move(special_edge_list));
}

}  // namespace ircx
