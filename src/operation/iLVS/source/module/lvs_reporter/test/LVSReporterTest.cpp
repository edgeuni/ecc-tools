#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "LVSReporter.hpp"
#include "json.hpp"

int main()
{
  const std::filesystem::path output_dir = std::filesystem::temp_directory_path() / "ilvs_lvs_reporter_test";
  std::filesystem::remove_all(output_dir);

  ilvs::LVSCheckResult result;
  result.expected_io_num = 2;
  result.physical_io_num = 2;
  result.expected_power_ground_io_num = 1;
  result.physical_power_ground_io_num = 2;
  result.missing_io_num = 1;
  result.unexpected_io_num = 2;
  result.expected_instance_num = 3;
  result.physical_instance_num = 3;
  result.missing_instance_num = 1;
  result.unexpected_instance_num = 2;
  result.expected_net_num = 1;
  result.physical_net_num = 1;
  result.missing_net_num = 1;
  result.unexpected_net_num = 2;
  result.net_pin_mismatch_num = 3;
  result.routing_checked_net_num = 4;
  result.routing_connected_net_num = 2;
  result.routing_open_net_num = 2;
  result.routing_open_load_pin_num = 3;
  result.routing_missing_driver_num = 1;
  result.routing_short_component_num = 1;
  ilvs::LVSViolation routing_violation;
  routing_violation.type = "RoutingOpen";
  routing_violation.net_name = "n1";
  routing_violation.driver_terminal_name = "u1/A";
  routing_violation.terminal_list = {"u1/B"};
  routing_violation.shape_list = {{1, 0, 0, 10, 10}};
  result.violation_list.push_back(std::move(routing_violation));
  ilvs::LVSViolation short_violation;
  short_violation.type = "RoutingShort";
  short_violation.component_id_list = {0};
  short_violation.related_net_name_list = {"n1", "n2"};
  result.violation_list.push_back(std::move(short_violation));
  ilvs::LVSViolation instance_violation;
  instance_violation.type = "MissingInstance";
  instance_violation.instance_name = "u2";
  result.violation_list.push_back(std::move(instance_violation));
  ilvs::LVSNetlist expected;
  ilvs::LVSNet expected_net;
  expected_net.name = "n1";
  expected_net.terminal_list = {"u1/A", "u1/B"};
  expected.net_map[expected_net.name] = expected_net;
  expected.logical_graph.io_pin_list = {"PIN/clk", "PIN/rst"};
  expected.logical_graph.instance_map["u1"] = {"u1", {"A", "B"}, "NAND2_X1"};
  expected.logical_graph.net_edge_num = 2;
  ilvs::LVSNetlist physical;
  ilvs::LVSNet physical_net;
  physical_net.name = "n1";
  physical_net.terminal_list = {"u1/A"};
  physical_net.wire_segment_num = 5;
  physical_net.via_num = 6;
  physical.net_map[physical_net.name] = physical_net;
  physical.physical_graph.node_num = 1;
  physical.physical_graph.edge_num = 2;
  physical.physical_graph.candidate_pair_num = 3;
  physical.physical_graph.max_active_shape_num = 4;
  physical.physical_graph.component_num = 1;
  physical.physical_graph.component_shape_map[0] = {{1, 0, 0, 10, 10}};
  physical.physical_graph.io_pin_list = {"PIN/clk", "PIN/rst"};
  physical.physical_graph.instance_map["u1"] = {"u1", {}, "NAND2_X1"};

  const std::vector<fort::char_table> summary_table_list = ilvs::LVSReporter::getSummaryTableList(result, expected, physical);
  assert(summary_table_list.size() == 2);
  assert(summary_table_list.front().to_string().find("Entity Comparison") != std::string::npos);
  assert(summary_table_list.front().to_string().find("NETLIST") != std::string::npos);
  assert(summary_table_list.front().to_string().find("DEF") != std::string::npos);
  assert(summary_table_list.front().to_string().find("Difference") != std::string::npos);
  assert(summary_table_list.front().to_string().find("IO") != std::string::npos);
  assert(summary_table_list.front().to_string().find("Instance") != std::string::npos);
  assert(summary_table_list.front().to_string().find("Net") != std::string::npos);
  assert(summary_table_list.front().to_string().find("Graph") == std::string::npos);
  assert(summary_table_list.front().to_string().find("Total") == std::string::npos);
  assert(summary_table_list.back().to_string().find("Routing Connectivity") != std::string::npos);
  assert(summary_table_list.back().to_string().find("Checked Net") != std::string::npos);
  assert(summary_table_list.back().to_string().find("Open Net") != std::string::npos);
  assert(summary_table_list.back().to_string().find("Short Component") != std::string::npos);
  assert(summary_table_list.back().to_string().find("Check Summary") == std::string::npos);

  ilvs::LVSReporter::report(result, expected, physical, output_dir.string());
  assert(std::filesystem::exists(output_dir / "ilvs.rpt"));
  assert(std::filesystem::exists(output_dir / "ilvs.json"));
  std::ifstream rpt_file(output_dir / "ilvs.rpt");
  std::string rpt((std::istreambuf_iterator<char>(rpt_file)), std::istreambuf_iterator<char>());
  const size_t statistics_pos = rpt.find("[Statistics]");
  const size_t violation_details_pos = rpt.find("[Violation Details]");
  assert(statistics_pos != std::string::npos);
  assert(rpt.find("Entity Comparison") != std::string::npos);
  assert(rpt.find("NETLIST") != std::string::npos);
  assert(rpt.find("DEF") != std::string::npos);
  assert(rpt.find("Routing Connectivity") != std::string::npos);
  assert(rpt.find("Checked Net") != std::string::npos);
  assert(rpt.find("Open Net") != std::string::npos);
  assert(rpt.find("Open Load Pin") != std::string::npos);
  assert(rpt.find("Missing Driver Pin") != std::string::npos);
  assert(rpt.find("Short Component") != std::string::npos);
  assert(rpt.find("Check Summary") == std::string::npos);
  assert(rpt.find("Net Pin Mismatch") == std::string::npos);
  assert(rpt.find("IO comparison excludes power/ground ports (NETLIST=1, DEF=2)") != std::string::npos);
  assert(rpt.find("Graph Candidate Pair") == std::string::npos);
  assert(rpt.find("Total") == std::string::npos);
  assert(violation_details_pos != std::string::npos);
  assert(statistics_pos < violation_details_pos);
  assert(rpt.find("[1] RoutingOpen") != std::string::npos);
  assert(rpt.find("[2] RoutingShort") != std::string::npos);
  assert(rpt.find("[3] MissingInstance") != std::string::npos);
  assert(rpt.find("Driver") != std::string::npos);
  assert(rpt.find("Nets: n1 n2") != std::string::npos);
  assert(rpt.find("NETLIST Master") == std::string::npos);
  assert(rpt.find("DEF Master") == std::string::npos);
  assert(rpt.find("Coordinates (DBU)") != std::string::npos);
  assert(rpt.find("Component") != std::string::npos);
  assert(rpt.find("LLX") != std::string::npos);
  rpt_file.close();
  std::ifstream json_file(output_dir / "ilvs.json");
  nlohmann::json json;
  json_file >> json;
  assert(json["summary"]["missing_nets"] == 1);
  assert(json["summary"]["missing_ios"] == 1);
  assert(json["summary"]["physical_power_ground_ios"] == 2);
  assert(json["summary"]["net_pin_mismatches"] == 3);
  assert(json["summary"]["routing_checked_nets"] == 4);
  assert(json["summary"]["routing_open_nets"] == 2);
  assert(json["summary"]["routing_open_load_pins"] == 3);
  assert(json["summary"]["routing_missing_driver_pins"] == 1);
  assert(json["summary"]["routing_short_components"] == 1);
  assert(json["physical_graph"]["candidate_pairs"] == 3);
  assert(json["violations"][0]["driver"] == "u1/A");
  assert(json["violations"][0]["shapes"][0]["layer"] == 1);
  assert((json["violations"][1]["nets"] == std::vector<std::string>{"n1", "n2"}));
  assert(json["violations"][2]["instance"] == "u2");
  assert(!json["violations"][2].contains("expected_master"));
  assert(!json["violations"][2].contains("def_master"));
  json_file.close();

  result.violation_list.clear();
  ilvs::LVSReporter::report(result, expected, physical, output_dir.string());
  std::ifstream no_violation_rpt_file(output_dir / "ilvs.rpt");
  std::string no_violation_rpt((std::istreambuf_iterator<char>(no_violation_rpt_file)), std::istreambuf_iterator<char>());
  assert(no_violation_rpt.find("[Violation Details]\nNone") != std::string::npos);
  std::filesystem::remove_all(output_dir);
  return 0;
}
