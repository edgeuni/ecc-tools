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
 * @file WirelengthGridTest.cc
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-06-12
 * @brief Unit tests for the H-tree characterization grid plan: auto-derived
 *        grids bound direct characterization by auto_direct_bins_cap (not the
 *        legacy wirelength_iterations knob), runtime-configured grids stay
 *        untouched, and direct length indices switch between dense and sparse
 *        coverage (task 06-12-char-wirelength-coverage).
 */

#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "flow/synthesis/htree/characterization/wirelength/WirelengthGrid.hh"

namespace icts_test {
namespace {

constexpr double kRelTol = 1e-9;

// vga_lcd-like request set: 13 lengths, max 506.74 um -> auto unit
// 506.74 / 13 = 38.98 um, required covering iterations = 13.
auto makeVgaLikeRequests() -> std::vector<double>
{
  return {6.0, 8.0, 10.0, 12.0, 20.0, 30.0, 40.0, 60.0, 90.0, 140.0, 144.0, 300.0, 506.74};
}

TEST(WirelengthGridTest, AutoModeIgnoresLegacyIterationsAndAppliesDefaultCap)
{
  icts::CharBuilder::Config config;
  config.wirelength_iterations = 3U;  // legacy template knob must not cap the auto grid

  const auto plan = icts::htree::ResolveCharacterizationGridPlan(config, makeVgaLikeRequests());
  ASSERT_TRUE(plan.adapted);
  EXPECT_EQ(plan.source, icts::htree::CharGridSource::kAutoDerived);
  EXPECT_NEAR(plan.wirelength_unit_um, 506.74 / 13.0, (506.74 / 13.0) * kRelTol);
  EXPECT_EQ(plan.required_covering_iterations, 13U);
  EXPECT_EQ(plan.auto_direct_bins_cap, 6U);
  EXPECT_EQ(plan.wirelength_iterations, 6U);
}

TEST(WirelengthGridTest, AutoModeCoversFullyBelowCap)
{
  icts::CharBuilder::Config config;
  config.wirelength_iterations = 3U;

  const std::vector<double> requests = {20.0, 40.0, 60.0, 100.0};  // unit 25, required 4
  const auto plan = icts::htree::ResolveCharacterizationGridPlan(config, requests);
  ASSERT_TRUE(plan.adapted);
  EXPECT_EQ(plan.required_covering_iterations, 4U);
  EXPECT_EQ(plan.wirelength_iterations, 4U);
}

TEST(WirelengthGridTest, AutoModeHonorsConfiguredCapAndFloorsAtOne)
{
  icts::CharBuilder::Config config;
  config.auto_direct_bins_cap = 4U;
  auto plan = icts::htree::ResolveCharacterizationGridPlan(config, makeVgaLikeRequests());
  ASSERT_TRUE(plan.adapted);
  EXPECT_EQ(plan.auto_direct_bins_cap, 4U);
  EXPECT_EQ(plan.wirelength_iterations, 4U);

  config.auto_direct_bins_cap = 0U;  // degenerate config clamps to one direct bin
  plan = icts::htree::ResolveCharacterizationGridPlan(config, makeVgaLikeRequests());
  ASSERT_TRUE(plan.adapted);
  EXPECT_EQ(plan.auto_direct_bins_cap, 1U);
  EXPECT_EQ(plan.wirelength_iterations, 1U);
}

TEST(WirelengthGridTest, RuntimeConfiguredGridStaysUntouched)
{
  icts::CharBuilder::Config config;
  config.wirelength_unit_um = 10.0;
  config.wirelength_iterations = 3U;

  const std::vector<double> requests = {20.0, 40.0, 60.0};  // bins {2,4,6}: no collapse
  const auto plan = icts::htree::ResolveCharacterizationGridPlan(config, requests);
  EXPECT_FALSE(plan.adapted);
  EXPECT_EQ(plan.source, icts::htree::CharGridSource::kRuntimeConfig);
  EXPECT_EQ(plan.unique_level_bins, 3U);
  // The plan does not override an explicitly configured grid: CharBuilder
  // keeps consuming the runtime unit + iterations directly.
  EXPECT_EQ(plan.wirelength_iterations, 0U);
  EXPECT_EQ(plan.auto_direct_bins_cap, 0U);
}

TEST(WirelengthGridTest, CollapsedConfiguredGridAdaptsWithAutoCap)
{
  icts::CharBuilder::Config config;
  config.wirelength_unit_um = 1000.0;  // collapses {20,40,60} into a single bin
  config.wirelength_iterations = 3U;

  const std::vector<double> requests = {20.0, 40.0, 60.0};
  const auto plan = icts::htree::ResolveCharacterizationGridPlan(config, requests);
  ASSERT_TRUE(plan.adapted);
  EXPECT_TRUE(plan.configured_grid_collapsed);
  EXPECT_EQ(plan.source, icts::htree::CharGridSource::kAutoDerived);
  EXPECT_NEAR(plan.wirelength_unit_um, 20.0, 20.0 * kRelTol);
  EXPECT_EQ(plan.required_covering_iterations, 3U);
  EXPECT_EQ(plan.wirelength_iterations, 3U);
  EXPECT_EQ(plan.auto_direct_bins_cap, 6U);
}

TEST(WirelengthGridTest, DenseDirectIndicesWhenRequiredExceedsCap)
{
  icts::CharBuilder::Config config;
  const auto requests = makeVgaLikeRequests();
  const auto plan = icts::htree::ResolveCharacterizationGridPlan(config, requests);
  ASSERT_TRUE(plan.adapted);
  ASSERT_EQ(plan.wirelength_iterations, 6U);

  const auto indices = icts::htree::ResolveDirectCharacterizationLengthIndices(requests, plan);
  const std::vector<unsigned> expected_dense = {1U, 2U, 3U, 4U, 5U, 6U};
  EXPECT_EQ(indices, expected_dense);
}

TEST(WirelengthGridTest, SparseDirectIndicesWhenFullyCovered)
{
  icts::CharBuilder::Config config;
  const std::vector<double> requests = {10.0, 30.0, 200.0, 400.0};  // unit 100, bins {1,1,2,4}
  const auto plan = icts::htree::ResolveCharacterizationGridPlan(config, requests);
  ASSERT_TRUE(plan.adapted);
  ASSERT_EQ(plan.required_covering_iterations, 4U);
  ASSERT_EQ(plan.wirelength_iterations, 4U);

  const auto indices = icts::htree::ResolveDirectCharacterizationLengthIndices(requests, plan);
  const std::vector<unsigned> expected_sparse = {1U, 2U, 4U};  // bin 3 has no request: skipped
  EXPECT_EQ(indices, expected_sparse);
}

}  // namespace
}  // namespace icts_test
