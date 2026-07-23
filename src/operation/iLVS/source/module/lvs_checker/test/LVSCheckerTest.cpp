#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "LVSChecker.hpp"

int main()
{
  ilvs::LVSNetlist expected;
  expected.net_map["n1"] = {"n1", {"u1/A", "u2/Z"}};
  expected.net_map["n2"] = {"n2", {"u3/A", "u4/Z"}};
  expected.net_map["n_missing"] = {"n_missing", {"u_missing/A"}};
  expected.logical_graph.io_pin_list = {"PIN/clk", "PIN/VDD", "PIN/missing"};
  expected.logical_graph.instance_map["u1"] = {"u1", {}, "INV_X1"};
  expected.logical_graph.instance_map["u_missing"] = {"u_missing", {}, "AOI21_X1"};
  expected.logical_graph.instance_map["u_master"] = {"u_master", {}, "NAND2_X1"};

  ilvs::LVSNetlist physical;
  physical.net_map = expected.net_map;
  physical.net_map.erase("n_missing");
  physical.net_map["n2"].terminal_list = {"u3/A", "u4/Y"};
  physical.net_map["n_extra"] = {"n_extra", {"u5/A", "u5/B"}};
  physical.physical_graph.io_pin_list = {"PIN/clk", "PIN/VDD", "PIN/extra"};
  physical.physical_graph.instance_map["u1"] = {"u1", {}, "INV_X1"};
  physical.physical_graph.instance_map["u_extra"] = {"u_extra", {}, "BUF_X1"};
  physical.physical_graph.instance_map["u_master"] = {"u_master", {}, "NOR2_X1"};
  physical.physical_graph.power_net_set.insert("VDD");
  physical.physical_graph.ground_net_set.insert("VSS");
  physical.physical_graph.component_net_map[7] = {"n2", "n1"};

  ilvs::LVSNetRoutingGraph& n1_routing_graph = physical.physical_graph.net_routing_graph_map["n1"];
  n1_routing_graph.driver_terminal_name = "u1/A";
  n1_routing_graph.shape_list = {{0, 0, 0, 1, 1}, {1, 10, 0, 11, 1}, {1, 11, 0, 20, 1},
                                  {1, 20, 0, 21, 1}, {2, 20, 0, 21, 1}, {2, 21, 0, 22, 1},
                                  {1, 0, 10, 11, 11}};
  n1_routing_graph.via_shape_pair_list = {{3, 4}};
  n1_routing_graph.terminal_shape_map = {{"u1/A", {0, 1}}, {"u2/Z", {5}}};

  ilvs::LVSNetRoutingGraph& n2_routing_graph = physical.physical_graph.net_routing_graph_map["n2"];
  n2_routing_graph.driver_terminal_name = "u3/A";
  n2_routing_graph.shape_list = {{1, 0, 10, 1, 11}, {1, 10, 10, 11, 11}};
  n2_routing_graph.terminal_shape_map = {{"u3/A", {0}}, {"u4/Y", {1}}};

  ilvs::LVSNetRoutingGraph& n_extra_routing_graph = physical.physical_graph.net_routing_graph_map["n_extra"];
  n_extra_routing_graph.shape_list = {{1, 0, 20, 1, 21}, {1, 2, 20, 3, 21}};
  n_extra_routing_graph.terminal_shape_map = {{"u5/A", {0}}, {"u5/B", {1}}};

  ilvs::LVSCheckResult result = ilvs::LVSChecker::check(expected, physical);
  assert(result.expected_io_num == 2);
  assert(result.physical_io_num == 2);
  assert(result.expected_power_ground_io_num == 1);
  assert(result.physical_power_ground_io_num == 1);
  assert(result.missing_io_num == 1);
  assert(result.unexpected_io_num == 1);
  assert(result.expected_instance_num == 3);
  assert(result.physical_instance_num == 3);
  assert(result.missing_instance_num == 1);
  assert(result.unexpected_instance_num == 1);
  assert(result.expected_net_num == 3);
  assert(result.physical_net_num == 3);
  assert(result.missing_net_num == 1);
  assert(result.unexpected_net_num == 1);
  assert(result.net_pin_mismatch_num == 1);
  assert(result.routing_checked_net_num == 3);
  assert(result.routing_connected_net_num == 1);
  assert(result.routing_open_net_num == 1);
  assert(result.routing_open_load_pin_num == 1);
  assert(result.routing_missing_driver_num == 1);
  assert(result.routing_short_component_num == 1);
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "MissingNet" && violation.net_name == "n_missing";
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "UnexpectedNet" && violation.net_name == "n_extra";
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "NetPinMismatch" && violation.net_name == "n2"
           && violation.terminal_list == std::vector<std::string>{"NETLIST/u4/Z", "DEF/u4/Y"};
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "RoutingOpen" && violation.net_name == "n2" && violation.driver_terminal_name == "u3/A"
           && violation.terminal_list == std::vector<std::string>{"u4/Y"} && violation.shape_list.size() == 1
           && violation.shape_list.front().ll_x == 10;
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "RoutingDriverMissing" && violation.net_name == "n_extra"
           && violation.terminal_list == std::vector<std::string>{"u5/A", "u5/B"};
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "RoutingShort" && violation.component_id_list == std::vector<uint64_t>{7}
           && violation.related_net_name_list == std::vector<std::string>{"n1", "n2"};
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "MissingIO" && violation.terminal_list == std::vector<std::string>{"PIN/missing"};
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "UnexpectedIO" && violation.terminal_list == std::vector<std::string>{"PIN/extra"};
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "MissingInstance" && violation.instance_name == "u_missing";
  }));
  assert(std::any_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "UnexpectedInstance" && violation.instance_name == "u_extra";
  }));
  assert(std::none_of(result.violation_list.begin(), result.violation_list.end(), [](const ilvs::LVSViolation& violation) {
    return violation.type == "InstanceMasterMismatch" || violation.type == "Open" || violation.type == "Short" || violation.type == "Unrouted";
  }));
  return 0;
}
