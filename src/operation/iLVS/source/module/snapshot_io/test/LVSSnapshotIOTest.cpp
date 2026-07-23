// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include "LVSSnapshotIO.hpp"

namespace {

ilvs::LVSNetlist makeLogicalNetlist()
{
  ilvs::LVSNetlist netlist;
  netlist.design_name = "top";
  netlist.net_map["n1"] = {"n1", {"PIN/in", "u1/A"}, 0, 0, 0, 0};
  netlist.net_map["n2"] = {"n2", {"u1/Z", "u2/A"}, 0, 0, 0, 0};
  netlist.logical_graph.instance_map["u1"] = {"u1", {"A", "Z"}, "NAND2_X1"};
  netlist.logical_graph.instance_map["u2"] = {"u2", {"A"}, "INV_X1"};
  netlist.logical_graph.io_pin_list = {"PIN/in"};
  netlist.logical_graph.net_edge_num = 4;
  netlist.physical_graph.node_num = 99;
  netlist.physical_graph.power_net_set = {"IGNORED"};
  return netlist;
}

ilvs::LVSNetlist makePhysicalNetlist()
{
  ilvs::LVSNetlist netlist;
  netlist.design_name = "top";
  netlist.net_map["n1"] = {"n1", {"PIN/in", "u1/A"}, 3, 2, 1, 0};
  ilvs::LVSPhysicalGraph& graph = netlist.physical_graph;
  graph.node_num = 7;
  graph.edge_num = 6;
  graph.candidate_pair_num = 8;
  graph.max_active_shape_num = 4;
  graph.component_num = 2;
  graph.short_component_num = 1;
  graph.power_port_num = 1;
  graph.floating_power_port_num = 1;
  graph.ground_port_num = 1;
  graph.floating_ground_port_num = 0;
  graph.power_pin_num = 2;
  graph.floating_power_pin_num = 1;
  graph.ground_pin_num = 2;
  graph.floating_ground_pin_num = 1;
  graph.floating_power_port_list = {"PIN/VDD"};
  graph.floating_ground_port_list = {};
  graph.floating_power_pin_list = {"u1/VDD"};
  graph.floating_ground_pin_list = {"u2/VSS"};
  graph.component_terminal_map = {{0, {"PIN/in", "u1/A"}}, {1, {"PIN/VDD", "u1/VDD"}}};
  graph.component_net_map = {{0, {"n1"}}, {1, {"VDD", "VSS"}}};
  graph.component_shape_map = {{0, {{2, 0, 0, 10, 10}}}, {1, {{3, 20, 20, 30, 30}}}};
  graph.terminal_component_map = {{"PIN/in", 0}, {"u1/A", 0}, {"PIN/VDD", 1}, {"u1/VDD", 1}};
  ilvs::LVSNetRoutingGraph& n1_routing_graph = graph.net_routing_graph_map["n1"];
  n1_routing_graph.driver_terminal_name = "PIN/in";
  n1_routing_graph.shape_list = {{2, 0, 0, 10, 10}, {2, 10, 0, 20, 10}, {3, 10, 0, 20, 10}, {3, 20, 0, 30, 10}};
  n1_routing_graph.via_shape_pair_list = {{1, 2}};
  n1_routing_graph.terminal_shape_map = {{"PIN/in", {0}}, {"u1/A", {3}}};
  graph.power_net_set = {"VDD"};
  graph.ground_net_set = {"VSS"};
  graph.instance_map["u1"] = {"u1", {}, "NAND2_X1"};
  graph.instance_map["u2"] = {"u2", {}, "INV_X1"};
  graph.io_pin_list = {"PIN/in"};
  netlist.logical_graph.instance_map["ignored"] = {"ignored", {"A"}};
  netlist.logical_graph.io_pin_list = {"PIN/ignored"};
  netlist.logical_graph.net_edge_num = 1;
  return netlist;
}

void assertShapeMapEqual(const std::unordered_map<uint64_t, std::vector<ilvs::LVSPhysicalGraph::ShapeLocation>>& expected,
                         const std::unordered_map<uint64_t, std::vector<ilvs::LVSPhysicalGraph::ShapeLocation>>& actual)
{
  assert(expected.size() == actual.size());
  for (const auto& [component_id, expected_shape_list] : expected) {
    const auto actual_iter = actual.find(component_id);
    assert(actual_iter != actual.end());
    assert(expected_shape_list.size() == actual_iter->second.size());
    for (size_t idx = 0; idx < expected_shape_list.size(); idx++) {
      const ilvs::LVSPhysicalGraph::ShapeLocation& expected_shape = expected_shape_list[idx];
      const ilvs::LVSPhysicalGraph::ShapeLocation& actual_shape = actual_iter->second[idx];
      assert(expected_shape.layer_id == actual_shape.layer_id);
      assert(expected_shape.ll_x == actual_shape.ll_x);
      assert(expected_shape.ll_y == actual_shape.ll_y);
      assert(expected_shape.ur_x == actual_shape.ur_x);
      assert(expected_shape.ur_y == actual_shape.ur_y);
    }
  }
}

void assertShapeListEqual(const std::vector<ilvs::LVSShapeLocation>& expected, const std::vector<ilvs::LVSShapeLocation>& actual)
{
  assert(expected.size() == actual.size());
  for (size_t idx = 0; idx < expected.size(); idx++) {
    assert(expected[idx].layer_id == actual[idx].layer_id);
    assert(expected[idx].ll_x == actual[idx].ll_x);
    assert(expected[idx].ll_y == actual[idx].ll_y);
    assert(expected[idx].ur_x == actual[idx].ur_x);
    assert(expected[idx].ur_y == actual[idx].ur_y);
  }
}

void assertNetRoutingGraphMapEqual(const std::unordered_map<std::string, ilvs::LVSNetRoutingGraph>& expected,
                                   const std::unordered_map<std::string, ilvs::LVSNetRoutingGraph>& actual)
{
  assert(expected.size() == actual.size());
  for (const auto& [net_name, expected_routing_graph] : expected) {
    const auto actual_iter = actual.find(net_name);
    assert(actual_iter != actual.end());
    const ilvs::LVSNetRoutingGraph& actual_routing_graph = actual_iter->second;
    assert(expected_routing_graph.driver_terminal_name == actual_routing_graph.driver_terminal_name);
    assertShapeListEqual(expected_routing_graph.shape_list, actual_routing_graph.shape_list);
    assert(expected_routing_graph.via_shape_pair_list == actual_routing_graph.via_shape_pair_list);
    assert(expected_routing_graph.terminal_shape_map == actual_routing_graph.terminal_shape_map);
  }
}

void assertLogicalEqual(const ilvs::LVSNetlist& expected, const ilvs::LVSNetlist& actual)
{
  assert(expected.design_name == actual.design_name);
  assert(expected.net_map.size() == actual.net_map.size());
  assert(actual.net_map.at("n1").terminal_list == expected.net_map.at("n1").terminal_list);
  assert(actual.logical_graph.instance_map.at("u1").pin_list == expected.logical_graph.instance_map.at("u1").pin_list);
  assert(actual.logical_graph.instance_map.at("u1").master_name == expected.logical_graph.instance_map.at("u1").master_name);
  assert(actual.logical_graph.io_pin_list == expected.logical_graph.io_pin_list);
  assert(actual.logical_graph.net_edge_num == expected.logical_graph.net_edge_num);
  assert(actual.physical_graph.node_num == 0);
  assert(actual.physical_graph.power_net_set.empty());
  assert(actual.physical_graph.net_routing_graph_map.empty());
}

void assertPhysicalEqual(const ilvs::LVSNetlist& expected, const ilvs::LVSNetlist& actual)
{
  assert(expected.design_name == actual.design_name);
  assert(actual.net_map.size() == expected.net_map.size());
  assert(actual.net_map.at("n1").terminal_list == expected.net_map.at("n1").terminal_list);
  assert(actual.net_map.at("n1").wire_segment_num == expected.net_map.at("n1").wire_segment_num);
  assert(actual.net_map.at("n1").via_num == expected.net_map.at("n1").via_num);
  assert(actual.net_map.at("n1").terminal_component_num == expected.net_map.at("n1").terminal_component_num);
  assert(actual.net_map.at("n1").floating_terminal_num == expected.net_map.at("n1").floating_terminal_num);
  assert(actual.physical_graph.node_num == expected.physical_graph.node_num);
  assert(actual.physical_graph.edge_num == expected.physical_graph.edge_num);
  assert(actual.physical_graph.candidate_pair_num == expected.physical_graph.candidate_pair_num);
  assert(actual.physical_graph.max_active_shape_num == expected.physical_graph.max_active_shape_num);
  assert(actual.physical_graph.component_num == expected.physical_graph.component_num);
  assert(actual.physical_graph.short_component_num == expected.physical_graph.short_component_num);
  assert(actual.physical_graph.power_port_num == expected.physical_graph.power_port_num);
  assert(actual.physical_graph.floating_power_port_num == expected.physical_graph.floating_power_port_num);
  assert(actual.physical_graph.ground_port_num == expected.physical_graph.ground_port_num);
  assert(actual.physical_graph.floating_ground_port_num == expected.physical_graph.floating_ground_port_num);
  assert(actual.physical_graph.power_pin_num == expected.physical_graph.power_pin_num);
  assert(actual.physical_graph.floating_power_pin_num == expected.physical_graph.floating_power_pin_num);
  assert(actual.physical_graph.ground_pin_num == expected.physical_graph.ground_pin_num);
  assert(actual.physical_graph.floating_ground_pin_num == expected.physical_graph.floating_ground_pin_num);
  assert(actual.physical_graph.floating_power_port_list == expected.physical_graph.floating_power_port_list);
  assert(actual.physical_graph.floating_ground_port_list == expected.physical_graph.floating_ground_port_list);
  assert(actual.physical_graph.floating_power_pin_list == expected.physical_graph.floating_power_pin_list);
  assert(actual.physical_graph.floating_ground_pin_list == expected.physical_graph.floating_ground_pin_list);
  assert(actual.physical_graph.component_terminal_map == expected.physical_graph.component_terminal_map);
  assert(actual.physical_graph.component_net_map == expected.physical_graph.component_net_map);
  assertShapeMapEqual(expected.physical_graph.component_shape_map, actual.physical_graph.component_shape_map);
  assert(actual.physical_graph.terminal_component_map == expected.physical_graph.terminal_component_map);
  assertNetRoutingGraphMapEqual(expected.physical_graph.net_routing_graph_map, actual.physical_graph.net_routing_graph_map);
  assert(actual.physical_graph.power_net_set == expected.physical_graph.power_net_set);
  assert(actual.physical_graph.ground_net_set == expected.physical_graph.ground_net_set);
  assert(actual.physical_graph.instance_map.size() == expected.physical_graph.instance_map.size());
  assert(actual.physical_graph.instance_map.at("u1").master_name == expected.physical_graph.instance_map.at("u1").master_name);
  assert(actual.physical_graph.instance_map.at("u2").master_name == expected.physical_graph.instance_map.at("u2").master_name);
  assert(actual.physical_graph.io_pin_list == expected.physical_graph.io_pin_list);
  assert(actual.logical_graph.instance_map.empty());
  assert(actual.logical_graph.io_pin_list.empty());
  assert(actual.logical_graph.net_edge_num == 0);
}

}  // namespace

int main()
{
  const std::filesystem::path output_dir = std::filesystem::temp_directory_path() / "ilvs_snapshot_io_test";
  std::filesystem::remove_all(output_dir);
  const std::filesystem::path logical_path = output_dir / "netlist.bin";
  const std::filesystem::path physical_path = output_dir / "def.bin";
  std::string error_message;

  const ilvs::LVSNetlist logical = makeLogicalNetlist();
  assert(ilvs::LVSSnapshotIO::write(logical, ilvs::LVSSnapshotType::kLogical, logical_path.string(), error_message));
  ilvs::LVSNetlist loaded_logical;
  assert(ilvs::LVSSnapshotIO::read(logical_path.string(), ilvs::LVSSnapshotType::kLogical, loaded_logical, error_message));
  assertLogicalEqual(logical, loaded_logical);

  const ilvs::LVSNetlist physical = makePhysicalNetlist();
  assert(ilvs::LVSSnapshotIO::write(physical, ilvs::LVSSnapshotType::kPhysical, physical_path.string(), error_message));
  ilvs::LVSNetlist loaded_physical;
  assert(ilvs::LVSSnapshotIO::read(physical_path.string(), ilvs::LVSSnapshotType::kPhysical, loaded_physical, error_message));
  assertPhysicalEqual(physical, loaded_physical);

  assert(!ilvs::LVSSnapshotIO::read(logical_path.string(), ilvs::LVSSnapshotType::kPhysical, loaded_physical, error_message));
  assert(!error_message.empty());
  assertPhysicalEqual(physical, loaded_physical);

  const std::filesystem::path missing_path = output_dir / "missing.bin";
  assert(!ilvs::LVSSnapshotIO::read(missing_path.string(), ilvs::LVSSnapshotType::kLogical, loaded_logical, error_message));
  assert(!error_message.empty());
  assertLogicalEqual(logical, loaded_logical);

  const std::filesystem::path truncated_path = output_dir / "truncated.bin";
  std::filesystem::copy_file(physical_path, truncated_path);
  std::filesystem::resize_file(truncated_path, std::filesystem::file_size(truncated_path) - 1);
  assert(!ilvs::LVSSnapshotIO::read(truncated_path.string(), ilvs::LVSSnapshotType::kPhysical, loaded_physical, error_message));
  assert(!error_message.empty());
  assertPhysicalEqual(physical, loaded_physical);

  const std::filesystem::path version_path = output_dir / "unsupported_version.bin";
  std::filesystem::copy_file(logical_path, version_path);
  std::fstream version_file(version_path, std::ios::binary | std::ios::in | std::ios::out);
  const uint32_t unsupported_version = 4;
  version_file.seekp(8);
  version_file.write(reinterpret_cast<const char*>(&unsupported_version), sizeof(unsupported_version));
  version_file.close();
  assert(!ilvs::LVSSnapshotIO::read(version_path.string(), ilvs::LVSSnapshotType::kLogical, loaded_logical, error_message));
  assert(!error_message.empty());
  assertLogicalEqual(logical, loaded_logical);

  const std::filesystem::path corrupted_path = output_dir / "corrupted.bin";
  std::filesystem::copy_file(logical_path, corrupted_path);
  std::fstream corrupted_file(corrupted_path, std::ios::binary | std::ios::in | std::ios::out);
  corrupted_file.seekp(0);
  corrupted_file.put('X');
  corrupted_file.close();
  assert(!ilvs::LVSSnapshotIO::read(corrupted_path.string(), ilvs::LVSSnapshotType::kLogical, loaded_logical, error_message));
  assert(!error_message.empty());
  assertLogicalEqual(logical, loaded_logical);

  const std::filesystem::path checksum_path = output_dir / "checksum.bin";
  std::filesystem::copy_file(logical_path, checksum_path);
  std::fstream checksum_file(checksum_path, std::ios::binary | std::ios::in | std::ios::out);
  checksum_file.seekp(24);
  checksum_file.put('X');
  checksum_file.close();
  assert(!ilvs::LVSSnapshotIO::read(checksum_path.string(), ilvs::LVSSnapshotType::kLogical, loaded_logical, error_message));
  assert(error_message.find("checksum") != std::string::npos);
  assertLogicalEqual(logical, loaded_logical);

  std::filesystem::remove_all(output_dir);
  return 0;
}
