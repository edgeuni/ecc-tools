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
  rpt_file << "Open Nets: " << check_result.open_net_num << "\n";
  rpt_file << "Short Components: " << check_result.short_component_num << "\n";
  rpt_file << "Power/Ground Shorts: " << check_result.power_ground_short_num << "\n";
  rpt_file << "Floating Power Ports: " << check_result.floating_power_port_num << "\n";
  rpt_file << "Floating Ground Ports: " << check_result.floating_ground_port_num << "\n";
  rpt_file << "Floating Power Pins: " << check_result.floating_power_pin_num << "\n";
  rpt_file << "Floating Ground Pins: " << check_result.floating_ground_pin_num << "\n";
  rpt_file << "Total: " << check_result.violation_list.size() << "\n\n";
  for (const LVSViolation& violation : check_result.violation_list) {
    rpt_file << "[" << violation.type << "] net=" << violation.net_name << " components=";
    for (uint64_t component_id : violation.component_id_list) rpt_file << component_id << " ";
    rpt_file << " terminals=";
    for (const std::string& terminal : violation.terminal_list) rpt_file << terminal << " ";
    rpt_file << "\n";
  }

  nlohmann::json json;
  json["summary"] = {{"expected_nets", check_result.expected_net_num}, {"physical_nets", check_result.physical_net_num},
                     {"open_nets", check_result.open_net_num}, {"short_components", check_result.short_component_num},
                     {"power_ground_shorts", check_result.power_ground_short_num},
                     {"floating_power_pins", check_result.floating_power_pin_num}, {"floating_ground_pins", check_result.floating_ground_pin_num},
                     {"total", check_result.violation_list.size()}};
  json["physical_graph"] = {{"nodes", physical_netlist.physical_graph.node_num}, {"edges", physical_netlist.physical_graph.edge_num},
                            {"components", physical_netlist.physical_graph.component_num}};
  for (const LVSViolation& violation : check_result.violation_list) {
    json["violations"].push_back({{"type", violation.type}, {"net", violation.net_name}, {"terminals", violation.terminal_list},
                                  {"components", violation.component_id_list}});
  }
  std::ofstream json_file(std::filesystem::path(report_directory_path) / "ilvs.json");
  json_file << json.dump(2) << "\n";
}

}  // namespace ilvs
