#include <algorithm>
#include <cassert>

#include "LVSChecker.hpp"

int main()
{
  ilvs::LVSNetlist expected;
  expected.net_map["n1"] = {"n1", {"u1/A", "u2/Z"}};
  expected.net_map["n2"] = {"n2", {"u3/A", "u4/Z"}};

  ilvs::LVSNetlist physical;
  physical.net_map = expected.net_map;
  physical.net_map["n_extra"] = {"n_extra", {"u5/A"}};
  physical.physical_graph.terminal_component_map = {{"u1/A", 1}, {"u2/Z", 2}, {"u3/A", 3}, {"u4/Z", 3}};
  physical.physical_graph.component_net_map = {{1, {"n1"}}, {2, {"n1"}}, {3, {"n2", "n3"}}};
  physical.physical_graph.power_net_set.insert("VDD");
  physical.physical_graph.ground_net_set.insert("VSS");
  physical.physical_graph.component_net_map[4] = {"VDD", "VSS"};
  physical.physical_graph.floating_power_port_list = {"PIN/VDD"};
  physical.physical_graph.floating_power_port_num = 1;
  physical.physical_graph.floating_ground_port_list = {"PIN/VSS"};
  physical.physical_graph.floating_ground_port_num = 1;
  physical.physical_graph.floating_power_pin_list = {"u1/VDD"};
  physical.physical_graph.floating_power_pin_num = 1;
  physical.physical_graph.floating_ground_pin_list = {"u2/VSS"};
  physical.physical_graph.floating_ground_pin_num = 1;

  ilvs::LVSCheckResult result = ilvs::LVSChecker::check(expected, physical);
  assert(result.open_net_num == 1);
  assert(result.short_component_num == 2);
  assert(result.power_ground_short_num == 1);
  assert(result.floating_power_port_num == 1);
  assert(result.floating_ground_port_num == 1);
  assert(result.floating_power_pin_num == 1);
  assert(result.floating_ground_pin_num == 1);
  assert(result.unexpected_net_num == 1);
  assert(result.unrouted_net_num == 2);
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "UnexpectedNet";
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "Unrouted";
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "FloatingPowerPin";
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "FloatingGroundPin";
  }));
  return 0;
}
