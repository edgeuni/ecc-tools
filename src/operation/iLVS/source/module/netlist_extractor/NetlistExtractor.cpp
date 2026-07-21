// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#include "NetlistExtractor.hpp"

#include <algorithm>
#include <numeric>
#include <unordered_set>

#include "IdbDesign.h"
#include "IdbInstance.h"
#include "IdbNet.h"
#include "IdbPins.h"
#include "IdbSpecialNet.h"
#include "IdbVias.h"

namespace ilvs {

namespace {

struct GraphNode
{
  std::string net_name;
  idb::IdbRect rect;
  int32_t layer_id = -1;
  bool is_terminal = false;
  bool is_io_terminal = false;
  bool is_power_terminal = false;
  bool is_ground_terminal = false;
};

class DisjointSet
{
 public:
  explicit DisjointSet(size_t size) : _parent(size), _rank(size, 0) { std::iota(_parent.begin(), _parent.end(), 0); }

  size_t find(size_t node)
  {
    if (_parent[node] != node) {
      _parent[node] = find(_parent[node]);
    }
    return _parent[node];
  }
  bool unite(size_t first, size_t second)
  {
    first = find(first);
    second = find(second);
    if (first == second) {
      return false;
    }
    if (_rank[first] < _rank[second]) {
      std::swap(first, second);
    }
    _parent[second] = first;
    if (_rank[first] == _rank[second]) {
      _rank[first]++;
    }
    return true;
  }

 private:
  std::vector<size_t> _parent;
  std::vector<uint8_t> _rank;
};

}  // namespace

LVSNetlist NetlistExtractor::extract(idb::IdbDesign* design)
{
  LVSNetlist netlist;
  if (design == nullptr || design->get_net_list() == nullptr) {
    return netlist;
  }

  netlist.design_name = design->get_design_name();
  for (idb::IdbNet* idb_net : design->get_net_list()->get_net_list()) {
    if (idb_net == nullptr) {
      continue;
    }
    LVSNet net;
    net.name = idb_net->get_net_name();
    net.wire_segment_num = idb_net->get_segment_wire_num();
    net.via_num = idb_net->get_via_num();

    auto add_terminal_list = [&net, &netlist](idb::IdbPins* pin_list) {
      if (pin_list == nullptr) {
        return;
      }
      for (idb::IdbPin* pin : pin_list->get_pin_list()) {
        if (pin != nullptr) {
          net.terminal_list.push_back(getTerminalName(pin));
          if (pin->is_io_pin()) {
            netlist.logical_graph.io_pin_list.push_back(getTerminalName(pin));
          } else if (idb::IdbInstance* instance = pin->get_instance(); instance != nullptr) {
            LVSInstanceNode& instance_node = netlist.logical_graph.instance_map[instance->get_name()];
            instance_node.name = instance->get_name();
            instance_node.pin_list.push_back(pin->get_pin_name());
          }
        }
      }
    };
    add_terminal_list(idb_net->get_io_pins());
    add_terminal_list(idb_net->get_instance_pin_list());
    std::sort(net.terminal_list.begin(), net.terminal_list.end());
    net.terminal_list.erase(std::unique(net.terminal_list.begin(), net.terminal_list.end()), net.terminal_list.end());
    netlist.net_map.emplace(net.name, std::move(net));
  }
  std::sort(netlist.logical_graph.io_pin_list.begin(), netlist.logical_graph.io_pin_list.end());
  netlist.logical_graph.io_pin_list.erase(std::unique(netlist.logical_graph.io_pin_list.begin(), netlist.logical_graph.io_pin_list.end()),
                                          netlist.logical_graph.io_pin_list.end());
  for (auto& [instance_name, instance_node] : netlist.logical_graph.instance_map) {
    (void) instance_name;
    std::sort(instance_node.pin_list.begin(), instance_node.pin_list.end());
    instance_node.pin_list.erase(std::unique(instance_node.pin_list.begin(), instance_node.pin_list.end()), instance_node.pin_list.end());
  }
  for (const auto& [net_name, net] : netlist.net_map) {
    (void) net_name;
    netlist.logical_graph.net_edge_num += net.terminal_list.size();
  }
  std::vector<GraphNode> graph_node_list;
  std::vector<std::pair<size_t, size_t>> via_node_pair_list;
  std::unordered_map<std::string, std::vector<std::vector<size_t>>> terminal_node_map;
  std::unordered_map<std::string, std::vector<std::string>> terminal_name_map;
  const auto add_shape = [&graph_node_list](const std::string& net_name, idb::IdbLayer* layer, const idb::IdbRect& rect, bool is_terminal = false,
                                            bool is_io_terminal = false, bool is_power_terminal = false, bool is_ground_terminal = false) {
    if (layer == nullptr || !layer->is_routing()) {
      return static_cast<size_t>(-1);
    }
    graph_node_list.push_back({net_name, rect, layer->get_id(), is_terminal, is_io_terminal, is_power_terminal, is_ground_terminal});
    return graph_node_list.size() - 1;
  };
  const auto add_pin = [&add_shape, &terminal_node_map, &terminal_name_map](const std::string& net_name, idb::IdbPin* pin, bool is_power_port,
                                                                             bool is_ground_port) {
    std::vector<size_t> pin_node_list;
    for (idb::IdbLayerShape* layer_shape : pin->get_port_box_list()) {
      if (layer_shape == nullptr) {
        continue;
      }
      for (idb::IdbRect* rect : layer_shape->get_rect_list()) {
        if (rect != nullptr) {
          size_t node = add_shape(net_name, layer_shape->get_layer(), *rect, true, pin->is_io_pin(), is_power_port, is_ground_port);
          if (node != static_cast<size_t>(-1)) {
            pin_node_list.push_back(node);
          }
        }
      }
    }
    if (!pin_node_list.empty()) {
      terminal_node_map[net_name].push_back(std::move(pin_node_list));
      terminal_name_map[net_name].push_back(getTerminalName(pin));
    }
  };
  const auto add_via = [&add_shape, &via_node_pair_list](const std::string& net_name, idb::IdbVia* via) {
    if (via == nullptr) {
      return;
    }
    idb::IdbLayerShape bottom_shape = via->get_bottom_layer_shape();
    idb::IdbLayerShape top_shape = via->get_top_layer_shape();
    size_t bottom_node = add_shape(net_name, bottom_shape.get_layer(), bottom_shape.get_bounding_box());
    size_t top_node = add_shape(net_name, top_shape.get_layer(), top_shape.get_bounding_box());
    if (bottom_node != static_cast<size_t>(-1) && top_node != static_cast<size_t>(-1)) {
      via_node_pair_list.emplace_back(bottom_node, top_node);
    }
  };

  for (idb::IdbNet* idb_net : design->get_net_list()->get_net_list()) {
    if (idb_net == nullptr) {
      continue;
    }
    const std::string net_name = idb_net->get_net_name();
    for (idb::IdbPin* pin : idb_net->get_io_pins()->get_pin_list()) {
      if (pin != nullptr) {
        add_pin(net_name, pin, false, false);
      }
    }
    for (idb::IdbPin* pin : idb_net->get_instance_pin_list()->get_pin_list()) {
      if (pin != nullptr) {
        add_pin(net_name, pin, false, false);
      }
    }
    for (idb::IdbRegularWire* wire : idb_net->get_wire_list()->get_wire_list()) {
      for (idb::IdbRegularWireSegment* segment : wire->get_segment_list()) {
        if (segment == nullptr) {
          continue;
        }
        if (segment->is_via()) {
          for (idb::IdbVia* via : segment->get_via_list()) {
            add_via(net_name, via);
          }
        } else if (segment->get_layer() != nullptr && (segment->is_wire() || segment->is_rect())) {
          add_shape(net_name, segment->get_layer(), segment->get_segment_rect());
        }
      }
    }
  }
  for (idb::IdbSpecialNet* special_net : design->get_special_net_list()->get_net_list()) {
    if (special_net == nullptr) {
      continue;
    }
    const std::string net_name = special_net->get_net_name();
    for (idb::IdbPin* pin : special_net->get_io_pins()->get_pin_list()) {
      if (pin != nullptr) {
        add_pin(net_name, pin, special_net->is_vdd(), special_net->is_vss());
      }
    }
    for (idb::IdbPin* pin : special_net->get_instance_pin_list()->get_pin_list()) {
      if (pin != nullptr) {
        add_pin(net_name, pin, special_net->is_vdd(), special_net->is_vss());
      }
    }
    for (idb::IdbSpecialWire* wire : special_net->get_wire_list()->get_wire_list()) {
      for (idb::IdbSpecialWireSegment* segment : wire->get_segment_list()) {
        if (segment == nullptr) {
          continue;
        }
        if (segment->is_via()) {
          add_via(net_name, segment->get_via());
        } else if (segment->get_layer() != nullptr && segment->is_line()) {
          add_shape(net_name, segment->get_layer(), idb::IdbRect(segment->get_point_start(), segment->get_point_second(), segment->get_route_width()));
        } else if (segment->get_layer() != nullptr && segment->is_rect() && segment->get_delta_rect() != nullptr) {
          add_shape(net_name, segment->get_layer(), *segment->get_delta_rect());
        }
      }
    }
  }

  DisjointSet graph(graph_node_list.size());
  uint64_t edge_num = 0;
  std::unordered_map<int32_t, std::vector<size_t>> layer_node_map;
  for (size_t node_idx = 0; node_idx < graph_node_list.size(); node_idx++) {
    layer_node_map[graph_node_list[node_idx].layer_id].push_back(node_idx);
  }
  for (auto& [layer_id, node_list] : layer_node_map) {
    (void) layer_id;
    std::sort(node_list.begin(), node_list.end(), [&graph_node_list](size_t first, size_t second) {
      return graph_node_list[first].rect.get_low_x() < graph_node_list[second].rect.get_low_x();
    });
    std::vector<size_t> active_node_list;
    for (size_t node_idx : node_list) {
      idb::IdbRect& node_rect = graph_node_list[node_idx].rect;
      active_node_list.erase(std::remove_if(active_node_list.begin(), active_node_list.end(), [&graph_node_list, &node_rect](size_t active_node_idx) {
                               return graph_node_list[active_node_idx].rect.get_high_x() < node_rect.get_low_x();
                             }),
                             active_node_list.end());
      for (size_t active_node_idx : active_node_list) {
        if (graph_node_list[active_node_idx].rect.isIntersection(node_rect) && graph.unite(active_node_idx, node_idx)) {
          edge_num++;
        }
      }
      active_node_list.push_back(node_idx);
    }
  }
  for (const auto& [bottom_node, top_node] : via_node_pair_list) {
    if (graph.unite(bottom_node, top_node)) {
      edge_num++;
    }
  }
  for (const auto& [net_name, pin_node_list] : terminal_node_map) {
    (void) net_name;
    for (const std::vector<size_t>& nodes : pin_node_list) {
      for (size_t node_idx = 1; node_idx < nodes.size(); node_idx++) {
        if (graph.unite(nodes.front(), nodes[node_idx])) {
          edge_num++;
        }
      }
    }
  }

  std::unordered_map<size_t, std::unordered_set<std::string>> component_net_map;
  std::unordered_map<size_t, bool> component_metal_map;
  for (size_t node_idx = 0; node_idx < graph_node_list.size(); node_idx++) {
    size_t root = graph.find(node_idx);
    component_net_map[root].insert(graph_node_list[node_idx].net_name);
    component_metal_map[root] = component_metal_map[root] || !graph_node_list[node_idx].is_terminal;
  }
  netlist.physical_graph.node_num = graph_node_list.size();
  netlist.physical_graph.edge_num = edge_num;
  netlist.physical_graph.component_num = component_net_map.size();
  std::unordered_map<size_t, uint64_t> component_id_map;
  uint64_t component_id = 0;
  for (const auto& [root, net_name_set] : component_net_map) {
    component_id_map[root] = component_id;
    netlist.physical_graph.component_net_map[component_id] = {net_name_set.begin(), net_name_set.end()};
    if (net_name_set.size() > 1) {
      netlist.physical_graph.short_component_num++;
    }
    component_id++;
  }
  for (const auto& [net_name, pin_node_list] : terminal_node_map) {
    std::unordered_set<size_t> terminal_component_set;
    uint64_t floating_terminal_num = 0;
    for (size_t pin_idx = 0; pin_idx < pin_node_list.size(); pin_idx++) {
      const std::vector<size_t>& nodes = pin_node_list[pin_idx];
      size_t root = graph.find(nodes.front());
      terminal_component_set.insert(root);
      netlist.physical_graph.component_terminal_map[component_id_map[root]].push_back(terminal_name_map[net_name][pin_idx]);
      netlist.physical_graph.terminal_component_map[terminal_name_map[net_name][pin_idx]] = component_id_map[root];
      if (!component_metal_map[root]) {
        floating_terminal_num++;
      }
      const GraphNode& node = graph_node_list[nodes.front()];
      if (node.is_power_terminal) {
        if (node.is_io_terminal) {
          netlist.physical_graph.power_port_num++;
          netlist.physical_graph.floating_power_port_num += !component_metal_map[root];
          if (!component_metal_map[root]) netlist.physical_graph.floating_power_port_list.push_back(terminal_name_map[net_name][pin_idx]);
        } else {
          netlist.physical_graph.power_pin_num++;
          netlist.physical_graph.floating_power_pin_num += !component_metal_map[root];
          if (!component_metal_map[root]) netlist.physical_graph.floating_power_pin_list.push_back(terminal_name_map[net_name][pin_idx]);
        }
      } else if (node.is_ground_terminal) {
        if (node.is_io_terminal) {
          netlist.physical_graph.ground_port_num++;
          netlist.physical_graph.floating_ground_port_num += !component_metal_map[root];
          if (!component_metal_map[root]) netlist.physical_graph.floating_ground_port_list.push_back(terminal_name_map[net_name][pin_idx]);
        } else {
          netlist.physical_graph.ground_pin_num++;
          netlist.physical_graph.floating_ground_pin_num += !component_metal_map[root];
          if (!component_metal_map[root]) netlist.physical_graph.floating_ground_pin_list.push_back(terminal_name_map[net_name][pin_idx]);
        }
      }
    }
    auto net_iter = netlist.net_map.find(net_name);
    if (net_iter != netlist.net_map.end()) {
      net_iter->second.terminal_component_num = terminal_component_set.size();
      net_iter->second.floating_terminal_num = floating_terminal_num;
    }
  }
  return netlist;
}

std::string NetlistExtractor::getTerminalName(idb::IdbPin* pin)
{
  if (pin->is_io_pin()) {
    return "PIN/" + pin->get_pin_name();
  }
  idb::IdbInstance* instance = pin->get_instance();
  if (instance == nullptr) {
    return "PIN/" + pin->get_pin_name();
  }
  return instance->get_name() + "/" + pin->get_pin_name();
}

}  // namespace ilvs
