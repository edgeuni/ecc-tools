#include <iostream>
#include <vector>

#include "Logger.hpp"
#include "RTInterface.hpp"
#include "TOPOBuilder.hpp"

namespace {

bool isSameTopo(const std::vector<irt::Segment<irt::PlanarCoord>>& expected,
                const std::vector<irt::Segment<irt::PlanarCoord>>& actual)
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

bool checkTopo(const std::vector<irt::PlanarCoord>& planar_coord_list)
{
  irt::TBTask tb_task;
  tb_task.set_planar_coord_list(planar_coord_list);

  std::vector<irt::Segment<irt::PlanarCoord>> expected = RTI.getPlanarTopoList(planar_coord_list);
  std::vector<irt::Segment<irt::PlanarCoord>> actual = RTTB.getPlanarTopoList(tb_task);
  return isSameTopo(expected, actual);
}

}  // namespace

int main()
{
  irt::Logger::initInst();
  irt::TOPOBuilder::initInst();
  RTTB.init();

  bool passed = true;
  passed = passed && checkTopo({});
  passed = passed && checkTopo({irt::PlanarCoord(10, 20)});
  passed = passed && checkTopo({irt::PlanarCoord(10, 20), irt::PlanarCoord(40, 20)});
  passed = passed && checkTopo({irt::PlanarCoord(0, 0), irt::PlanarCoord(10, 30), irt::PlanarCoord(30, 10), irt::PlanarCoord(40, 40)});

  RTTB.destroy();
  irt::TOPOBuilder::destroyInst();
  irt::Logger::destroyInst();

  if (!passed) {
    std::cerr << "TOPOBuilder changed the planar topology result\n";
    return 1;
  }
  return 0;
}
