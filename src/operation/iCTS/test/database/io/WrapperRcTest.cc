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
 * @file WrapperRcTest.cc
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-06-12
 * @brief Unit-contract tests for Wrapper wire RC queries: iDB stores LEF values
 *        verbatim (RPERSQ in ohm/sq), so per-length queries must return ohms /
 *        picofarads with no further unit scaling. Guards the historical 1000x
 *        resistance underestimation regression (task 06-12-fix-wire-res-unit).
 */

#include <gtest/gtest.h>

#include <vector>

#include "IdbDesign.h"
#include "IdbLayer.h"
#include "IdbLayout.h"
#include "IdbUnits.h"
#include "database/config/Config.hh"
#include "database/io/Wrapper.hh"
#include "database/routing/ClockRouteSegmentRc.hh"

namespace icts_test {
namespace {

// Mirrors ICsprout55 N551P6M MET4: RPERSQ 0.0914 ohm/sq, WIDTH 0.1 um,
// CPERSQDIST 0.0011069 pF/um^2, EDGECAPACITANCE 0.0000409 pF/um, 1000 DBU/um.
constexpr int32_t kDbuPerUm = 1000;
constexpr int32_t kWidthDbu = 100;
constexpr double kWidthUm = 0.1;
constexpr double kRPerSqOhm = 0.0914;
constexpr double kCPerSqDistPf = 0.0011069;
constexpr double kEdgeCapPf = 0.0000409;

constexpr double kExpectedResistancePerUmOhm = kRPerSqOhm / kWidthUm;  // 0.914
constexpr double kRelTol = 1e-9;

auto expectedCapacitancePf(double length_um) -> double
{
  return (kCPerSqDistPf * length_um * kWidthUm) + (kEdgeCapPf * 2.0 * (length_um + kWidthUm));
}

class WrapperRcTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    _layout = new idb::IdbLayout();
    auto* layout_units = new idb::IdbUnits();
    layout_units->set_microns_dbu(kDbuPerUm);
    _layout->set_units(layout_units);

    auto* layer = dynamic_cast<idb::IdbLayerRouting*>(_layout->get_layers()->set_layer("MET4", "ROUTING"));
    ASSERT_NE(layer, nullptr);
    layer->set_width(kWidthDbu);
    layer->set_resistance(kRPerSqOhm);
    layer->set_capacitance(kCPerSqDistPf);
    layer->set_edge_capacitance(kEdgeCapPf);
    // set_layer registers into the flat layer list only; the routing-layer
    // index used by Wrapper::queryRoutingLayer is maintained separately.
    _layout->get_layers()->add_routing_layer(layer);
    ASSERT_EQ(_layout->get_layers()->get_routing_layers_number(), 1);

    _design = new idb::IdbDesign(_layout);
    auto* design_units = new idb::IdbUnits();
    design_units->set_microns_dbu(kDbuPerUm);
    _design->set_units(design_units);

    _wrapper.set_idb_layout(_layout);
    _wrapper.set_idb_design(_design);
  }

  void TearDown() override
  {
    // Wrapper holds non-owning pointers; release the fixture-owned iDB objects.
    // ~IdbDesign frees neither its units nor the layout; ~IdbLayout frees its
    // own units and the registered layers.
    if (_design != nullptr) {
      delete _design->get_units();
      delete _design;
    }
    delete _layout;
  }

  icts::Wrapper _wrapper;
  idb::IdbLayout* _layout = nullptr;
  idb::IdbDesign* _design = nullptr;
};

TEST_F(WrapperRcTest, RequiredWireResistanceReturnsOhmsFromLefSheetResistance)
{
  const double resistance_one_um = _wrapper.queryRequiredWireResistance(1, 1.0, std::nullopt);
  EXPECT_NEAR(resistance_one_um, kExpectedResistancePerUmOhm, kExpectedResistancePerUmOhm * kRelTol);

  const double resistance_ten_um = _wrapper.queryRequiredWireResistance(1, 10.0, std::nullopt);
  EXPECT_NEAR(resistance_ten_um, 10.0 * kExpectedResistancePerUmOhm, 10.0 * kExpectedResistancePerUmOhm * kRelTol);
}

TEST_F(WrapperRcTest, RequiredWireCapacitanceReturnsPlateAndFringePf)
{
  const double capacitance_one_um = _wrapper.queryRequiredWireCapacitance(1, 1.0, std::nullopt);
  EXPECT_NEAR(capacitance_one_um, expectedCapacitancePf(1.0), expectedCapacitancePf(1.0) * kRelTol);
}

TEST_F(WrapperRcTest, ConfiguredClockRouteSegmentRcKeepsOhmScale)
{
  icts::Config config;
  config.set_routing_layers({1U});

  const auto segment_rc = _wrapper.queryConfiguredClockRouteSegmentRc(config);
  EXPECT_EQ(segment_rc.dbu_per_um, kDbuPerUm);
  EXPECT_NEAR(segment_rc.resistance_per_um_ohm, kExpectedResistancePerUmOhm, kExpectedResistancePerUmOhm * kRelTol);
  EXPECT_NEAR(segment_rc.capacitance_per_um_pf, expectedCapacitancePf(1.0), expectedCapacitancePf(1.0) * kRelTol);

  // Regression sentinel: the historical bug divided ohms by 1000 and yielded
  // 9.14e-4 ohm/um for this layer; any reappearance must trip this hard floor.
  EXPECT_GT(segment_rc.resistance_per_um_ohm, 0.1);
}

TEST_F(WrapperRcTest, ExplicitWireWidthOverridesLayerWidth)
{
  const double doubled_width_um = 2.0 * kWidthUm;
  const double resistance = _wrapper.queryRequiredWireResistance(1, 1.0, doubled_width_um);
  EXPECT_NEAR(resistance, kRPerSqOhm / doubled_width_um, (kRPerSqOhm / doubled_width_um) * kRelTol);
}

}  // namespace
}  // namespace icts_test
