// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
/**
 * @file SinkLoadRegionSplitTest.cc
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-06-12
 * @brief Unit tests for the sink-load-region split remediation rule: a
 *        boundary group whose load count exceeds max_fanout is bisected into
 *        deterministic sub-fanout subgroups (one local sub-buffer each) so a
 *        single dense pocket no longer vetoes shallower H-tree depths
 *        (task 06-11-htree-depth-unlock).
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <random>
#include <vector>

#include "Pin.hh"
#include "Point.hh"
#include "synthesis/htree/region/SinkLoadRegion.hh"

namespace icts_test {
namespace {

class SplitPinFactory
{
 public:
  auto make(int x, int y) -> icts::Pin*
  {
    auto pin = std::make_unique<icts::Pin>("split_pin_" + std::to_string(_pins.size()), icts::PinType::kIn, icts::Point<int>(x, y), nullptr,
                                           nullptr, false);
    _pins.push_back(std::move(pin));
    return _pins.back().get();
  }

 private:
  std::vector<std::unique_ptr<icts::Pin>> _pins;
};

auto collectAllPins(const icts::htree::SinkLoadRegionSplitPlan& plan) -> std::vector<icts::Pin*>
{
  std::vector<icts::Pin*> all_pins;
  for (const auto& subgroup : plan.subgroups) {
    all_pins.insert(all_pins.end(), subgroup.begin(), subgroup.end());
  }
  std::sort(all_pins.begin(), all_pins.end());
  return all_pins;
}

TEST(SinkLoadRegionSplitTest, FiveLoadsSplitIntoTwoSubgroups)
{
  SplitPinFactory factory;
  std::vector<icts::Pin*> loads = {factory.make(0, 0), factory.make(10, 0), factory.make(20, 0), factory.make(30, 0), factory.make(40, 0)};

  const auto plan = icts::htree::SplitSinkLoadRegionGroup(loads, 4U);
  ASSERT_TRUE(plan.feasible);
  ASSERT_EQ(plan.subgroups.size(), 2U);
  ASSERT_EQ(plan.centers.size(), 2U);
  for (const auto& subgroup : plan.subgroups) {
    EXPECT_LE(subgroup.size(), 4U);
    EXPECT_GE(subgroup.size(), 2U);
  }

  auto original = loads;
  std::sort(original.begin(), original.end());
  EXPECT_EQ(collectAllPins(plan), original);

  // Median cut on the x axis: lower half {0,10}, upper half {20,30,40}.
  EXPECT_EQ(plan.centers.at(1).get_x(), 5);
  EXPECT_EQ(plan.centers.at(0).get_x(), 30);
}

TEST(SinkLoadRegionSplitTest, NotApplicableWithinFanout)
{
  SplitPinFactory factory;
  std::vector<icts::Pin*> loads = {factory.make(0, 0), factory.make(1, 1), factory.make(2, 2), factory.make(3, 3)};
  const auto plan = icts::htree::SplitSinkLoadRegionGroup(loads, 4U);
  EXPECT_FALSE(plan.feasible);
}

TEST(SinkLoadRegionSplitTest, InfeasibleBeyondSquare)
{
  SplitPinFactory factory;
  std::vector<icts::Pin*> loads;
  for (int index = 0; index < 17; ++index) {
    loads.push_back(factory.make(index, 0));
  }
  const auto plan = icts::htree::SplitSinkLoadRegionGroup(loads, 4U);
  EXPECT_FALSE(plan.feasible);
}

TEST(SinkLoadRegionSplitTest, SixteenLoadsQuadSplit)
{
  SplitPinFactory factory;
  std::vector<icts::Pin*> loads;
  for (int index = 0; index < 16; ++index) {
    loads.push_back(factory.make(index * 5, (index % 2) * 3));
  }
  const auto plan = icts::htree::SplitSinkLoadRegionGroup(loads, 4U);
  ASSERT_TRUE(plan.feasible);
  EXPECT_EQ(plan.subgroups.size(), 4U);
  for (const auto& subgroup : plan.subgroups) {
    EXPECT_EQ(subgroup.size(), 4U);
  }
}

TEST(SinkLoadRegionSplitTest, DeterministicAcrossInputOrder)
{
  SplitPinFactory factory;
  std::vector<icts::Pin*> loads;
  for (int index = 0; index < 7; ++index) {
    loads.push_back(factory.make(index * 11, 97 - index * 13));
  }

  const auto reference_plan = icts::htree::SplitSinkLoadRegionGroup(loads, 4U);
  ASSERT_TRUE(reference_plan.feasible);

  auto shuffled = loads;
  std::mt19937 rng(7U);
  std::shuffle(shuffled.begin(), shuffled.end(), rng);
  const auto shuffled_plan = icts::htree::SplitSinkLoadRegionGroup(shuffled, 4U);
  ASSERT_TRUE(shuffled_plan.feasible);

  ASSERT_EQ(shuffled_plan.subgroups.size(), reference_plan.subgroups.size());
  for (std::size_t subgroup_index = 0; subgroup_index < reference_plan.subgroups.size(); ++subgroup_index) {
    EXPECT_EQ(shuffled_plan.subgroups.at(subgroup_index), reference_plan.subgroups.at(subgroup_index));
    EXPECT_EQ(shuffled_plan.centers.at(subgroup_index).get_x(), reference_plan.centers.at(subgroup_index).get_x());
    EXPECT_EQ(shuffled_plan.centers.at(subgroup_index).get_y(), reference_plan.centers.at(subgroup_index).get_y());
  }
}

TEST(SinkLoadRegionSplitTest, BisectionMayExceedSubgroupBudget)
{
  // 9 loads at fanout 3 fit 3x3 in theory, but median bisection yields four
  // subgroups; the plan stays infeasible by design (conservative single-stage
  // split, documented in the task design).
  SplitPinFactory factory;
  std::vector<icts::Pin*> loads;
  for (int index = 0; index < 9; ++index) {
    loads.push_back(factory.make(index, index));
  }
  const auto plan = icts::htree::SplitSinkLoadRegionGroup(loads, 3U);
  EXPECT_FALSE(plan.feasible);
}

}  // namespace
}  // namespace icts_test
