#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include "LVSReporter.hpp"

int main()
{
  const std::filesystem::path output_dir = std::filesystem::temp_directory_path() / "ilvs_lvs_reporter_test";
  std::filesystem::remove_all(output_dir);

  ilvs::LVSCheckResult result;
  result.expected_net_num = 1;
  result.physical_net_num = 1;
  result.violation_list.push_back({"Open", "n1", {"u1/A"}, {0}});
  ilvs::LVSNetlist expected;
  ilvs::LVSNetlist physical;
  physical.physical_graph.node_num = 1;
  physical.physical_graph.component_num = 1;
  physical.physical_graph.component_shape_map[0] = {{1, 0, 0, 10, 10}};

  ilvs::LVSReporter::report(result, expected, physical, output_dir.string());
  assert(std::filesystem::exists(output_dir / "ilvs.rpt"));
  assert(std::filesystem::exists(output_dir / "ilvs.json"));
  std::ifstream rpt_file(output_dir / "ilvs.rpt");
  std::string rpt((std::istreambuf_iterator<char>(rpt_file)), std::istreambuf_iterator<char>());
  assert(rpt.find("Total: 1") != std::string::npos);
  assert(rpt.find("shape layer=1") != std::string::npos);
  std::filesystem::remove_all(output_dir);
  return 0;
}
