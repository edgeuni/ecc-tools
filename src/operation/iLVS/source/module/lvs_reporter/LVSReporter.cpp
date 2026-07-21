#include "LVSReporter.hpp"

#include <filesystem>
#include <fstream>

#include "LVSHeader.hpp"

namespace ilvs {

void LVSReporter::report(const LVSCheckResult& check_result, const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist,
                         const std::string& report_directory_path)
{
  std::filesystem::create_directories(report_directory_path);
  std::ofstream rpt_file(std::filesystem::path(report_directory_path) / "ilvs.rpt");
  rpt_file << "iLVS Report\n";
  rpt_file << "Expected Nets: " << check_result.expected_net_num << "\n";
  rpt_file << "Physical Nets: " << check_result.physical_net_num << "\n";
  rpt_file << "Missing Nets: " << check_result.missing_net_num << "\n";
  rpt_file << "Unexpected Nets: " << check_result.unexpected_net_num << "\n";
  rpt_file << "Open Nets: " << check_result.open_net_num << "\n";
  rpt_file << "Missing Terminals: " << check_result.missing_terminal_num << "\n";
  rpt_file << "Unrouted Nets: " << check_result.unrouted_net_num << "\n";
  rpt_file << "Short Components: " << check_result.short_component_num << "\n";
  rpt_file << "Power/Ground Shorts: " << check_result.power_ground_short_num << "\n";
  rpt_file << "Floating Power Ports: " << check_result.floating_power_port_num << "\n";
  rpt_file << "Floating Ground Ports: " << check_result.floating_ground_port_num << "\n";
  rpt_file << "Floating Power Pins: " << check_result.floating_power_pin_num << "\n";
  rpt_file << "Floating Ground Pins: " << check_result.floating_ground_pin_num << "\n";
  rpt_file << "Physical Graph Nodes: " << physical_netlist.physical_graph.node_num << "\n";
  rpt_file << "Physical Graph Edges: " << physical_netlist.physical_graph.edge_num << "\n";
  rpt_file << "Physical Graph Components: " << physical_netlist.physical_graph.component_num << "\n";
  rpt_file << "Physical Graph Candidate Pairs: " << physical_netlist.physical_graph.candidate_pair_num << "\n";
  rpt_file << "Physical Graph Max Active Shapes: " << physical_netlist.physical_graph.max_active_shape_num << "\n";
  rpt_file << "Total: " << check_result.violation_list.size() << "\n\n";
  for (const LVSViolation& violation : check_result.violation_list) {
    rpt_file << "[" << violation.type << "] net=" << violation.net_name << " components=";
    for (uint64_t component_id : violation.component_id_list) rpt_file << component_id << " ";
    rpt_file << " terminals=";
    for (const std::string& terminal : violation.terminal_list) rpt_file << terminal << " ";
    rpt_file << "\n";
    for (uint64_t component_id : violation.component_id_list) {
      auto shape_iter = physical_netlist.physical_graph.component_shape_map.find(component_id);
      if (shape_iter == physical_netlist.physical_graph.component_shape_map.end()) continue;
      for (const LVSPhysicalGraph::ShapeLocation& shape : shape_iter->second) {
        rpt_file << "  shape layer=" << shape.layer_id << " rect=(" << shape.ll_x << "," << shape.ll_y << "," << shape.ur_x << "," << shape.ur_y << ")\n";
      }
    }
  }

  nlohmann::json json;
  json["summary"] = {{"expected_nets", check_result.expected_net_num}, {"physical_nets", check_result.physical_net_num},
                     {"missing_nets", check_result.missing_net_num}, {"unexpected_nets", check_result.unexpected_net_num},
                     {"open_nets", check_result.open_net_num}, {"missing_terminals", check_result.missing_terminal_num},
                     {"unrouted_nets", check_result.unrouted_net_num}, {"short_components", check_result.short_component_num},
                     {"power_ground_shorts", check_result.power_ground_short_num},
                     {"floating_power_ports", check_result.floating_power_port_num}, {"floating_ground_ports", check_result.floating_ground_port_num},
                     {"floating_power_pins", check_result.floating_power_pin_num}, {"floating_ground_pins", check_result.floating_ground_pin_num},
                     {"total", check_result.violation_list.size()}};
  json["physical_graph"] = {{"nodes", physical_netlist.physical_graph.node_num}, {"edges", physical_netlist.physical_graph.edge_num},
                            {"components", physical_netlist.physical_graph.component_num},
                            {"candidate_pairs", physical_netlist.physical_graph.candidate_pair_num},
                            {"max_active_shapes", physical_netlist.physical_graph.max_active_shape_num}};
  for (const LVSViolation& violation : check_result.violation_list) {
    nlohmann::json violation_json = {{"type", violation.type}, {"net", violation.net_name}, {"terminals", violation.terminal_list},
                                     {"components", violation.component_id_list}};
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
