#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include "LVSReporter.hpp"
#include "json.hpp"

int main()
{
  const std::filesystem::path output_dir = std::filesystem::temp_directory_path() / "ilvs_lvs_reporter_test";
  std::filesystem::remove_all(output_dir);

  ilvs::LVSCheckResult result;
  result.expected_net_num = 1;
  result.physical_net_num = 1;
  result.missing_net_num = 1;
  result.unexpected_net_num = 2;
  result.missing_terminal_num = 3;
  result.unrouted_net_num = 4;
  result.violation_list.push_back({"Open", "n1", {"u1/A"}, {0}});
  ilvs::LVSNetlist expected;
  ilvs::LVSNetlist physical;
  physical.physical_graph.node_num = 1;
  physical.physical_graph.edge_num = 2;
  physical.physical_graph.candidate_pair_num = 3;
  physical.physical_graph.max_active_shape_num = 4;
  physical.physical_graph.component_num = 1;
  physical.physical_graph.component_shape_map[0] = {{1, 0, 0, 10, 10}};

  ilvs::LVSReporter::report(result, expected, physical, output_dir.string());
  assert(std::filesystem::exists(output_dir / "ilvs.rpt"));
  assert(std::filesystem::exists(output_dir / "ilvs.json"));
  std::ifstream rpt_file(output_dir / "ilvs.rpt");
  std::string rpt((std::istreambuf_iterator<char>(rpt_file)), std::istreambuf_iterator<char>());
  assert(rpt.find("Missing Nets: 1") != std::string::npos);
  assert(rpt.find("Physical Graph Candidate Pairs: 3") != std::string::npos);
  assert(rpt.find("Total: 1") != std::string::npos);
  assert(rpt.find("shape layer=1") != std::string::npos);
  std::ifstream json_file(output_dir / "ilvs.json");
  nlohmann::json json;
  json_file >> json;
  assert(json["summary"]["missing_nets"] == 1);
  assert(json["physical_graph"]["candidate_pairs"] == 3);
  std::filesystem::remove_all(output_dir);
  return 0;
}
