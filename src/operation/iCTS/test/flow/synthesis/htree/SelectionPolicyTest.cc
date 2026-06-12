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
 * @file SelectionPolicyTest.cc
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-06-13
 * @brief Unit tests for the delay-margin bounded selection policy applied to
 *        delay-power Pareto fronts: margin <= 0 keeps the legacy Pareto-median
 *        index, margin > 0 picks the min-power entry whose delay stays within
 *        (1 + margin) x front-min delay (task 06-11-latency-align).
 */

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <vector>

#include "flow/synthesis/htree/topology_pruning/SelectionPolicy.hh"

namespace icts_test {
namespace {

struct PolicyEntry
{
  double delay = 0.0;
  double power = 0.0;
};

auto SelectIndex(const std::vector<PolicyEntry>& front, double margin) -> std::optional<std::size_t>
{
  return icts::htree::SelectDelayBoundedIndex(
      front, margin, [](const PolicyEntry& entry) -> double { return entry.delay; },
      [](const PolicyEntry& entry) -> double { return entry.power; });
}

// Power-ascending / delay-descending order, mirroring how the production call
// sites order their delay-power Pareto fronts before selection.
auto MakeParetoFront() -> std::vector<PolicyEntry>
{
  return {
      PolicyEntry{.delay = 10.0, .power = 1.0},
      PolicyEntry{.delay = 6.0, .power = 2.0},
      PolicyEntry{.delay = 5.0, .power = 3.0},
      PolicyEntry{.delay = 4.0, .power = 8.0},
  };
}

TEST(SelectionPolicyTest, ZeroMarginFallsBackToMedianIndex)
{
  const auto even_front = MakeParetoFront();
  EXPECT_EQ(SelectIndex(even_front, 0.0), std::optional<std::size_t>(1U));
  EXPECT_EQ(SelectIndex(even_front, -0.5), std::optional<std::size_t>(1U));

  auto odd_front = MakeParetoFront();
  odd_front.push_back(PolicyEntry{.delay = 3.0, .power = 9.0});
  EXPECT_EQ(SelectIndex(odd_front, 0.0), std::optional<std::size_t>(2U));
}

TEST(SelectionPolicyTest, PositiveMarginPicksMinPowerWithinBound)
{
  // front_min_delay = 4.0, margin 0.5 -> bound 6.0: indices {1, 2, 3} qualify
  // and index 1 carries the minimum power.
  EXPECT_EQ(SelectIndex(MakeParetoFront(), 0.5), std::optional<std::size_t>(1U));

  // Power tie within the bound prefers the smaller delay.
  const std::vector<PolicyEntry> power_tie_front{
      PolicyEntry{.delay = 10.0, .power = 2.0},
      PolicyEntry{.delay = 8.0, .power = 2.0},
      PolicyEntry{.delay = 4.0, .power = 9.0},
  };
  EXPECT_EQ(SelectIndex(power_tie_front, 2.0), std::optional<std::size_t>(1U));

  // A full delay-power tie keeps the first entry in front order.
  const std::vector<PolicyEntry> full_tie_front{
      PolicyEntry{.delay = 8.0, .power = 2.0},
      PolicyEntry{.delay = 8.0, .power = 2.0},
      PolicyEntry{.delay = 4.0, .power = 9.0},
  };
  EXPECT_EQ(SelectIndex(full_tie_front, 2.0), std::optional<std::size_t>(0U));
}

TEST(SelectionPolicyTest, TinyMarginSelectsFrontMinDelayEntry)
{
  EXPECT_EQ(SelectIndex(MakeParetoFront(), 1e-9), std::optional<std::size_t>(3U));
}

TEST(SelectionPolicyTest, HugeMarginSelectsGlobalMinPowerEntry)
{
  EXPECT_EQ(SelectIndex(MakeParetoFront(), 1e9), std::optional<std::size_t>(0U));
}

TEST(SelectionPolicyTest, SingleEntryFrontAlwaysSelectsIndexZero)
{
  const std::vector<PolicyEntry> single_front{PolicyEntry{.delay = 3.0, .power = 7.0}};
  EXPECT_EQ(SelectIndex(single_front, 0.0), std::optional<std::size_t>(0U));
  EXPECT_EQ(SelectIndex(single_front, 0.2), std::optional<std::size_t>(0U));
  EXPECT_EQ(SelectIndex(single_front, 1e9), std::optional<std::size_t>(0U));
}

TEST(SelectionPolicyTest, EmptyFrontReturnsNullopt)
{
  const std::vector<PolicyEntry> empty_front;
  EXPECT_EQ(SelectIndex(empty_front, 0.0), std::nullopt);
  EXPECT_EQ(SelectIndex(empty_front, 0.5), std::nullopt);
}

}  // namespace
}  // namespace icts_test
