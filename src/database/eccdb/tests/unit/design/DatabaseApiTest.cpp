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
#include "DesignTestFixture.h"

#include <stdexcept>

#include "api/detail/DatabaseTestAccess.h"
#include "eccdb/db.h"

namespace eccdb {
namespace {

TEST_F(DesignStorageTest, PublicApiProvidesTypedIdBasedCrud)
{
  auto database = ::eccdb::detail::DatabaseTestAccess::borrow(design);
  auto signal = database.createNet(DesignNet{.name = "signal", .use = DesignSignalUse::kSignal});
  auto power = database.createSpecialNet(DesignNet{.name = "VDD", .use = DesignSignalUse::kPower});
  auto instance = database.createInstance(createInstance("u1", {10, 20}));
  auto input = instance.findPin("A");
  auto io = database.createIoPin(
      DesignIoPin{.name = "IN", .direction = DesignIoPinDirection::kInput, .use = DesignSignalUse::kSignal});

  ASSERT_TRUE(signal);
  ASSERT_TRUE(power);
  ASSERT_TRUE(instance);
  ASSERT_TRUE(input);
  ASSERT_TRUE(io);
  EXPECT_EQ(database.getNet(signal.getId()).getName(), "signal");
  EXPECT_EQ(database.findInstance("u1").getId(), instance.getId());

  input.connect(signal);
  io.connect(signal);
  EXPECT_EQ(input.getNet().getId(), signal.getId());
  EXPECT_EQ(io.getNet().getId(), signal.getId());
  EXPECT_EQ(signal.getInstancePins().front().getId(), input.getId());
  EXPECT_EQ(signal.getIoPins().front().getId(), io.getId());

  signal.rename("signal_main");
  signal.setSource(DesignNetSource::kUser);
  signal.setWeight(5);
  instance.setOrigin({100, 200});
  instance.setPlacementStatus(DesignPlacementStatus::kFixed);
  io.setDirection(DesignIoPinDirection::kInOut);

  EXPECT_FALSE(database.findNet("signal"));
  EXPECT_EQ(database.findNet("signal_main").getId(), signal.getId());
  EXPECT_EQ(design.netlistStorage().net(signal.getId()).weight, 5);
  EXPECT_EQ(design.netlistStorage().instance(instance.getId()).origin, (Point{100, 200}));
  EXPECT_EQ(design.netlistStorage().ioPin(io.getId()).direction, DesignIoPinDirection::kInOut);

  input.disconnect(signal);
  io.disconnect(signal);
  EXPECT_FALSE(input.getNet());
  EXPECT_FALSE(io.getNet());
  EXPECT_TRUE(io.destroy());
  EXPECT_TRUE(instance.destroy());
  const auto stale_signal_id = signal.getId();
  EXPECT_TRUE(signal.destroy());
  EXPECT_FALSE(signal);
  EXPECT_FALSE(database.getNet(stale_signal_id));
}

TEST_F(DesignStorageTest, PublicApiNavigatesRoutingWithoutExposingStorage)
{
  auto database = ::eccdb::detail::DatabaseTestAccess::borrow(design);
  auto net = database.createSpecialNet(DesignNet{.name = "VDD", .use = DesignSignalUse::kPower});
  auto via = database.createVia(
      DesignVia{.name = "LOCAL_VIA", .rectangles = {{.layer = layer, .rectangle = {-5, -5, 5, 5}}}});

  DesignWireRoutingInput routing;
  routing.appendPath(DesignWirePath{.layer = routing_layer,
                                    .flags = DesignWirePathFlag::kHasWidth,
                                    .width = 20,
                                    .points = {{{0, 0}}, {{100, 0}}},
                                    .vias = {{.point_index = 1, .design_via = via.getId()}}});
  auto wire = net.createWire(std::move(routing), DesignWireStatus::kFixed);

  ASSERT_TRUE(wire);
  EXPECT_EQ(database.getWire(wire.getId()).getId(), wire.getId());
  EXPECT_EQ(wire.getNet().getId(), net.getId());
  EXPECT_EQ(wire.getStatus(), DesignWireStatus::kFixed);
  EXPECT_EQ(wire.getPathCount(), 1u);
  EXPECT_EQ(wire.getPath(0).points().size(), 2u);
  ASSERT_EQ(net.getWires().size(), 1u);
  EXPECT_EQ(net.getWires().front().getId(), wire.getId());
  EXPECT_EQ(database.findVia("LOCAL_VIA").getId(), via.getId());
  EXPECT_EQ(via.getRectangles().size(), 1u);
  EXPECT_FALSE(via.destroy());

  EXPECT_TRUE(wire.destroy());
  EXPECT_TRUE(via.destroy());
  EXPECT_TRUE(net.destroy());
}

TEST_F(DesignStorageTest, PublicApiRejectsRelationshipsAcrossDesigns)
{
  auto first = ::eccdb::detail::DatabaseTestAccess::borrow(design);
  auto first_net = first.createNet(DesignNet{.name = "first"});
  auto first_pin = first.createIoPin(DesignIoPin{.name = "first_pin"});

  DesignDatabase other_database{tech, library.libraryRegistry()};
  auto second = ::eccdb::detail::DatabaseTestAccess::borrow(other_database);
  auto second_net = second.createNet(DesignNet{.name = "second"});

  EXPECT_EQ(first_net.getId().packed(), second_net.getId().packed());
  EXPECT_THROW(first_pin.connect(second_net), std::invalid_argument);
  EXPECT_FALSE(first_pin.getNet());
}

}  // namespace
}  // namespace eccdb
