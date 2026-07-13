#include <iostream>
#include <string>
#include <vector>

#include "GridMap.hpp"
#include "Logger.hpp"
#include "TOPOBuilder.hpp"

namespace {

using irt::PlanarCoord;
using irt::Segment;

bool isSameTopo(const std::vector<Segment<PlanarCoord>>& expected, const std::vector<Segment<PlanarCoord>>& actual)
{
  if (expected.size() != actual.size()) {
    return false;
  }
  for (size_t i = 0; i < expected.size(); ++i) {
    if (expected[i].get_first() != actual[i].get_first() || expected[i].get_second() != actual[i].get_second()) {
      return false;
    }
  }
  return true;
}

bool check(bool condition, const std::string& case_name)
{
  if (!condition) {
    std::cerr << "Failed: " << case_name << '\n';
  }
  return condition;
}

irt::TBTask makeTask(const std::vector<PlanarCoord>& planar_coord_list, const irt::GridMap<bool>* steiner_forbidden_map = nullptr)
{
  irt::TBTask tb_task;
  tb_task.set_planar_coord_list(planar_coord_list);
  tb_task.set_steiner_forbidden_map(steiner_forbidden_map);
  return tb_task;
}

bool checkFluteGolden()
{
  bool passed = true;
  passed = check(RTTB.getPlanarTopoList(makeTask({})).empty(), "empty FLUTE topology") && passed;
  passed = check(RTTB.getPlanarTopoList(makeTask({PlanarCoord(10, 20)})).empty(), "single-pin FLUTE topology") && passed;

  std::vector<Segment<PlanarCoord>> two_pin_expected = {{PlanarCoord(10, 20), PlanarCoord(40, 20)}};
  passed = check(isSameTopo(two_pin_expected, RTTB.getPlanarTopoList(makeTask({PlanarCoord(10, 20), PlanarCoord(40, 20)}))),
                 "two-pin FLUTE topology")
           && passed;

  std::vector<PlanarCoord> terminal_list = {PlanarCoord(0, 0), PlanarCoord(10, 30), PlanarCoord(30, 10), PlanarCoord(40, 40)};
  std::vector<Segment<PlanarCoord>> multi_pin_expected = {{PlanarCoord(0, 0), PlanarCoord(30, 10)},
                                                         {PlanarCoord(10, 30), PlanarCoord(30, 30)},
                                                         {PlanarCoord(40, 40), PlanarCoord(30, 30)},
                                                         {PlanarCoord(30, 10), PlanarCoord(30, 30)}};
  passed = check(isSameTopo(multi_pin_expected, RTTB.getPlanarTopoList(makeTask(terminal_list))), "multi-pin FLUTE topology") && passed;
  return passed;
}

bool checkSteinerLegalization()
{
  std::vector<PlanarCoord> terminal_list = {PlanarCoord(0, 0), PlanarCoord(10, 30), PlanarCoord(30, 10), PlanarCoord(40, 40)};
  irt::GridMap<bool> forbidden_map(50, 50, false);
  forbidden_map[30][30] = true;
  forbidden_map[30][10] = true;

  irt::TBResult tb_result = RTTB.buildPlanarTopo(makeTask(terminal_list, &forbidden_map));
  std::vector<Segment<PlanarCoord>> expected = {{PlanarCoord(0, 0), PlanarCoord(30, 10)},
                                               {PlanarCoord(10, 30), PlanarCoord(29, 30)},
                                               {PlanarCoord(40, 40), PlanarCoord(29, 30)},
                                               {PlanarCoord(30, 10), PlanarCoord(29, 30)}};
  const irt::TBSteinerRepairStat& stat = tb_result.get_steiner_repair_stat();
  bool passed = true;
  passed = check(isSameTopo(expected, tb_result.get_planar_topo_list()), "move Steiner point and keep forbidden terminal") && passed;
  passed = check(stat.raw_steiner_in_macro == 1, "count raw forbidden Steiner point") && passed;
  passed = check(stat.fixed_steiner_in_macro == 1, "count fixed Steiner point") && passed;
  passed = check(stat.failed_steiner_legalize_num == 0, "no failed Steiner legalization") && passed;
  return passed;
}

bool checkFailedSteinerLegalization()
{
  std::vector<PlanarCoord> terminal_list = {PlanarCoord(0, 0), PlanarCoord(10, 30), PlanarCoord(30, 10), PlanarCoord(40, 40)};
  irt::GridMap<bool> forbidden_map(50, 50, true);

  irt::TBResult tb_result = RTTB.buildPlanarTopo(makeTask(terminal_list, &forbidden_map));
  std::vector<Segment<PlanarCoord>> expected = {{PlanarCoord(0, 0), PlanarCoord(30, 10)},
                                               {PlanarCoord(10, 30), PlanarCoord(30, 30)},
                                               {PlanarCoord(40, 40), PlanarCoord(30, 30)},
                                               {PlanarCoord(30, 10), PlanarCoord(30, 30)}};
  const irt::TBSteinerRepairStat& stat = tb_result.get_steiner_repair_stat();
  bool passed = true;
  passed = check(isSameTopo(expected, tb_result.get_planar_topo_list()), "keep topology when Steiner legalization fails") && passed;
  passed = check(stat.raw_steiner_in_macro == 1, "count unrepairable Steiner point") && passed;
  passed = check(stat.fixed_steiner_in_macro == 0, "do not count failed Steiner point as fixed") && passed;
  passed = check(stat.failed_steiner_legalize_num == 1, "count failed Steiner legalization") && passed;
  return passed;
}

bool checkCollapsedSteinerEdgeRemoval()
{
  std::vector<PlanarCoord> terminal_list = {PlanarCoord(0, 0), PlanarCoord(10, 30), PlanarCoord(30, 10), PlanarCoord(40, 40)};
  irt::GridMap<bool> forbidden_map(50, 50, true);
  forbidden_map[30][10] = false;

  irt::TBResult tb_result = RTTB.buildPlanarTopo(makeTask(terminal_list, &forbidden_map));
  std::vector<Segment<PlanarCoord>> expected = {{PlanarCoord(0, 0), PlanarCoord(30, 10)},
                                               {PlanarCoord(10, 30), PlanarCoord(30, 10)},
                                               {PlanarCoord(40, 40), PlanarCoord(30, 10)}};
  return check(isSameTopo(expected, tb_result.get_planar_topo_list()), "remove edge collapsed by Steiner movement");
}

}  // namespace

int main()
{
  irt::Logger::initInst();
  irt::TOPOBuilder::initInst();
  RTTB.init();

  bool passed = true;
  passed = checkFluteGolden() && passed;
  passed = checkSteinerLegalization() && passed;
  passed = checkFailedSteinerLegalization() && passed;
  passed = checkCollapsedSteinerEdgeRemoval() && passed;

  RTTB.destroy();
  irt::TOPOBuilder::destroyInst();
  irt::Logger::destroyInst();
  return passed ? 0 : 1;
}
