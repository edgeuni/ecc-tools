#include "LVSReporter.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include "LVSHeader.hpp"

namespace ilvs {

namespace {

template <typename T>
std::string getJoinedString(const std::vector<T>& value_list)
{
  if (value_list.empty()) {
    return "-";
  }
  std::ostringstream stream;
  for (size_t value_idx = 0; value_idx < value_list.size(); value_idx++) {
    if (value_idx > 0) {
      stream << " ";
    }
    stream << value_list[value_idx];
  }
  return stream.str();
}

}  // namespace

std::vector<fort::char_table> LVSReporter::getSummaryTableList(const LVSCheckResult& check_result, const LVSNetlist& /* expected_netlist */,
                                                                const LVSNetlist& /* physical_netlist */)
{
  fort::char_table netlist_summary_table;
  {
    netlist_summary_table.set_cell_text_align(fort::text_align::right);
    netlist_summary_table << fort::header << "Entity Comparison"
                          << "NETLIST"
                          << "DEF"
                          << "Difference" << fort::endr;
    auto append_summary_row = [&netlist_summary_table](const std::string& name, uint64_t expected_num, uint64_t physical_num,
                                                        uint64_t difference_num) {
      netlist_summary_table << name << expected_num << physical_num << difference_num << fort::endr;
    };
    append_summary_row("IO", check_result.expected_io_num, check_result.physical_io_num,
                       check_result.missing_io_num + check_result.unexpected_io_num);
    append_summary_row("Instance", check_result.expected_instance_num, check_result.physical_instance_num,
                       check_result.missing_instance_num + check_result.unexpected_instance_num);
    append_summary_row("Net", check_result.expected_net_num, check_result.physical_net_num,
                       check_result.missing_net_num + check_result.unexpected_net_num + check_result.net_pin_mismatch_num);
  }

  fort::char_table routing_connectivity_table;
  {
    routing_connectivity_table.set_cell_text_align(fort::text_align::right);
    routing_connectivity_table << fort::header << "Routing Connectivity"
                               << "Count" << fort::endr;
    routing_connectivity_table << "Checked Net" << check_result.routing_checked_net_num << fort::endr;
    routing_connectivity_table << "Connected Net" << check_result.routing_connected_net_num << fort::endr;
    routing_connectivity_table << "Open Net" << check_result.routing_open_net_num << fort::endr;
    routing_connectivity_table << "Open Load Pin" << check_result.routing_open_load_pin_num << fort::endr;
    routing_connectivity_table << "Missing Driver Pin" << check_result.routing_missing_driver_num << fort::endr;
    routing_connectivity_table << "Short Component" << check_result.routing_short_component_num << fort::endr;
  }

  std::vector<fort::char_table> summary_table_list;
  summary_table_list.push_back(std::move(netlist_summary_table));
  summary_table_list.push_back(std::move(routing_connectivity_table));
  return summary_table_list;
}

void LVSReporter::report(const LVSCheckResult& check_result, const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist,
                         const std::string& report_directory_path)
{
  std::filesystem::create_directories(report_directory_path);
  std::ofstream rpt_file(std::filesystem::path(report_directory_path) / "ilvs.rpt");

  rpt_file << "iLVS Report\n\n";
  rpt_file << "[Statistics]\n\n";
  rpt_file << "IO comparison excludes power/ground ports (NETLIST=" << check_result.expected_power_ground_io_num
           << ", DEF=" << check_result.physical_power_ground_io_num << ").\n\n";
  for (const fort::char_table& summary_table : getSummaryTableList(check_result, expected_netlist, physical_netlist)) {
    rpt_file << summary_table.to_string() << "\n";
  }

  rpt_file << "[Violation Details]\n";
  if (check_result.violation_list.empty()) {
    rpt_file << "None\n";
  }
  for (size_t violation_idx = 0; violation_idx < check_result.violation_list.size(); violation_idx++) {
    const LVSViolation& violation = check_result.violation_list[violation_idx];
    fort::char_table violation_table;
    {
      violation_table.set_cell_text_align(fort::text_align::right);
      violation_table << fort::header << "Violation"
                      << "Value" << fort::endr;
      violation_table << "Type" << violation.type << fort::endr;
      violation_table << "Net" << (violation.net_name.empty() ? "-" : violation.net_name) << fort::endr;
      if (!violation.instance_name.empty()) {
        violation_table << "Instance" << violation.instance_name << fort::endr;
      }
      if (!violation.driver_terminal_name.empty()) {
        violation_table << "Driver" << violation.driver_terminal_name << fort::endr;
      }
      if (!violation.related_net_name_list.empty()) {
        violation_table << "Net Count" << violation.related_net_name_list.size() << fort::endr;
      }
      violation_table << "Component Count" << violation.component_id_list.size() << fort::endr;
      violation_table << "Terminal Count" << violation.terminal_list.size() << fort::endr;
    }

    fort::char_table coordinate_table;
    coordinate_table.set_cell_text_align(fort::text_align::right);
    coordinate_table << fort::header << "Component"
                     << "Layer"
                     << "LLX"
                     << "LLY"
                     << "URX"
                     << "URY" << fort::endr;
    bool has_coordinate = false;
    for (const LVSShapeLocation& shape : violation.shape_list) {
      coordinate_table << "-" << shape.layer_id << shape.ll_x << shape.ll_y << shape.ur_x << shape.ur_y << fort::endr;
      has_coordinate = true;
    }
    for (uint64_t component_id : violation.component_id_list) {
      auto shape_iter = physical_netlist.physical_graph.component_shape_map.find(component_id);
      if (shape_iter == physical_netlist.physical_graph.component_shape_map.end()) continue;
      for (const LVSPhysicalGraph::ShapeLocation& shape : shape_iter->second) {
        coordinate_table << component_id << shape.layer_id << shape.ll_x << shape.ll_y << shape.ur_x << shape.ur_y << fort::endr;
        has_coordinate = true;
      }
    }

    rpt_file << "\n[" << violation_idx + 1 << "] " << violation.type << "\n";
    rpt_file << violation_table.to_string();
    rpt_file << "Components: " << getJoinedString(violation.component_id_list) << "\n";
    if (!violation.related_net_name_list.empty()) {
      rpt_file << "Nets: " << getJoinedString(violation.related_net_name_list) << "\n";
    }
    rpt_file << "Terminals: " << getJoinedString(violation.terminal_list) << "\n";
    rpt_file << "Coordinates (DBU)\n";
    if (has_coordinate) {
      rpt_file << coordinate_table.to_string();
    } else {
      rpt_file << "None\n";
    }
  }

  nlohmann::json json;
  json["summary"] = {{"expected_ios", check_result.expected_io_num}, {"physical_ios", check_result.physical_io_num},
                     {"expected_power_ground_ios", check_result.expected_power_ground_io_num},
                     {"physical_power_ground_ios", check_result.physical_power_ground_io_num},
                     {"missing_ios", check_result.missing_io_num}, {"unexpected_ios", check_result.unexpected_io_num},
                     {"expected_instances", check_result.expected_instance_num}, {"physical_instances", check_result.physical_instance_num},
                     {"missing_instances", check_result.missing_instance_num}, {"unexpected_instances", check_result.unexpected_instance_num},
                     {"expected_nets", check_result.expected_net_num}, {"physical_nets", check_result.physical_net_num},
                     {"missing_nets", check_result.missing_net_num}, {"unexpected_nets", check_result.unexpected_net_num},
                     {"net_pin_mismatches", check_result.net_pin_mismatch_num},
                     {"routing_checked_nets", check_result.routing_checked_net_num},
                     {"routing_connected_nets", check_result.routing_connected_net_num},
                     {"routing_open_nets", check_result.routing_open_net_num},
                     {"routing_open_load_pins", check_result.routing_open_load_pin_num},
                     {"routing_missing_driver_pins", check_result.routing_missing_driver_num},
                     {"routing_short_components", check_result.routing_short_component_num},
                     {"total", check_result.violation_list.size()}};
  json["physical_graph"] = {{"nodes", physical_netlist.physical_graph.node_num}, {"edges", physical_netlist.physical_graph.edge_num},
                            {"components", physical_netlist.physical_graph.component_num},
                            {"candidate_pairs", physical_netlist.physical_graph.candidate_pair_num},
                            {"max_active_shapes", physical_netlist.physical_graph.max_active_shape_num}};
  for (const LVSViolation& violation : check_result.violation_list) {
    nlohmann::json violation_json = {{"type", violation.type}, {"net", violation.net_name}, {"terminals", violation.terminal_list},
                                     {"components", violation.component_id_list}};
    if (!violation.instance_name.empty()) {
      violation_json["instance"] = violation.instance_name;
    }
    if (!violation.driver_terminal_name.empty()) {
      violation_json["driver"] = violation.driver_terminal_name;
    }
    if (!violation.related_net_name_list.empty()) {
      violation_json["nets"] = violation.related_net_name_list;
    }
    for (const LVSShapeLocation& shape : violation.shape_list) {
      violation_json["shapes"].push_back({{"layer", shape.layer_id}, {"rect", {shape.ll_x, shape.ll_y, shape.ur_x, shape.ur_y}}});
    }
    for (uint64_t component_id : violation.component_id_list) {
      auto shape_iter = physical_netlist.physical_graph.component_shape_map.find(component_id);
      if (shape_iter == physical_netlist.physical_graph.component_shape_map.end()) continue;
      for (const LVSPhysicalGraph::ShapeLocation& shape : shape_iter->second) {
        violation_json["shapes"].push_back({{"component", component_id}, {"layer", shape.layer_id},
                                             {"rect", {shape.ll_x, shape.ll_y, shape.ur_x, shape.ur_y}}});
      }
    }
    json["violations"].push_back(std::move(violation_json));
  }
  std::ofstream json_file(std::filesystem::path(report_directory_path) / "ilvs.json");
  json_file << json.dump(2) << "\n";
}

}  // namespace ilvs
