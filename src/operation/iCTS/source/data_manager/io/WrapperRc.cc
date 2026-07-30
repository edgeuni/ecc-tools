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
 * @file WrapperRc.cc
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-05-29
 * @brief Wrapper-backed wire RC queries for iCTS.
 */
#include <cmath>
#include <cstddef>
#include <optional>
#include <ostream>

#include "IdbLayer.h"
#include "IdbLayout.h"
#include "IdbUnits.h"
#include "Logger.hh"
#include "Wrapper.hh"
#include "config/Config.hh"
#include "routing/ClockRouteSegmentRc.hh"

namespace icts {
namespace {

constexpr double kOppositeSwitchingCouplingFactor = 2.0;

auto requireLayout(idb::IdbLayout* layout, const char* metric) -> idb::IdbLayout*
{
  if (layout == nullptr) {
    CTSLOG.error(Loc::current(), "Wrapper: iDB layout is unavailable for required wire ", metric, " query.");
  }
  if (layout->get_units() == nullptr) {
    CTSLOG.error(Loc::current(), "Wrapper: iDB units are unavailable for required wire ", metric, " query.");
  }
  if (layout->get_layers() == nullptr) {
    CTSLOG.error(Loc::current(), "Wrapper: iDB layers are unavailable for required wire ", metric, " query.");
  }
  return layout;
}

auto requireWireRcQuery(const char* metric, int routing_layer, double length_um) -> void
{
  if (routing_layer <= 0) {
    CTSLOG.error(Loc::current(), "Wrapper: required wire ", metric, " query needs a positive routing layer.");
  }
  if (!std::isfinite(length_um) || length_um < 0.0) {
    CTSLOG.error(Loc::current(), "Wrapper: required wire ", metric, " query needs a finite non-negative length, got ", length_um, " um.");
  }
}

auto requireRoutingLayer(const Config& config) -> int
{
  const auto& routing_layers = config.get_routing_layers();
  if (routing_layers.empty() || routing_layers.front() == 0U) {
    CTSLOG.error(Loc::current(), "Wrapper: routing layer is not configured for clock route segment RC.");
  }
  return static_cast<int>(routing_layers.front());
}

auto resolveWireWidth(const Config& config) -> std::optional<double>
{
  const double wire_width_um = config.get_wire_width();
  return wire_width_um > 0.0 ? std::optional<double>{wire_width_um} : std::nullopt;
}

auto queryRoutingLayer(idb::IdbLayout* layout, int routing_layer, const char* metric) -> idb::IdbLayerRouting*
{
  layout = requireLayout(layout, metric);
  auto& routing_layers = layout->get_layers()->get_routing_layers();
  const int routing_layer_id = routing_layer - 1;
  if (routing_layer_id < 0) {
    CTSLOG.error(Loc::current(), "Wrapper: routing layer ", routing_layer, " is out of range for required wire ", metric, " query.");
  }
  const auto layer_index = static_cast<std::size_t>(routing_layer_id);
  if (layer_index >= routing_layers.size()) {
    CTSLOG.error(Loc::current(), "Wrapper: routing layer ", routing_layer, " is out of range for required wire ", metric, " query.");
  }
  auto* layer = dynamic_cast<idb::IdbLayerRouting*>(routing_layers.at(layer_index));
  if (layer == nullptr) {
    CTSLOG.error(Loc::current(), "Wrapper: routing layer ", routing_layer, " is not a routing layer.");
  }
  return layer;
}

auto resolveWidthUm(idb::IdbLayout* layout, idb::IdbLayerRouting* layer, std::optional<double> wire_width_um) -> double
{
  if (wire_width_um.has_value()) {
    return *wire_width_um;
  }
  layout = requireLayout(layout, "width");
  const auto dbu_per_um = layout->get_units()->get_micron_dbu();
  if (dbu_per_um <= 0) {
    CTSLOG.error(Loc::current(), "Wrapper: DBU-per-micron is invalid when resolving routing layer width.");
  }
  return static_cast<double>(layer->get_width()) / static_cast<double>(dbu_per_um);
}

auto buildWireCapacitanceProfile(idb::IdbLayerRouting* layer, double length_um, double width_um) -> Wrapper::WireCapacitanceProfile
{
  const double area_cap_pf = layer->get_capacitance() * length_um * width_um;
  const double edge_cap_pf = layer->get_edge_capacitance() * 2.0 * (length_um + width_um);
  const double ground_cap_pf = area_cap_pf;
  const double coupling_cap_pf = edge_cap_pf;
  return Wrapper::WireCapacitanceProfile{
      .area_cap_pf = area_cap_pf,
      .edge_cap_pf = edge_cap_pf,
      .ground_cap_pf = ground_cap_pf,
      .coupling_cap_pf = coupling_cap_pf,
      .timing_coupling_factor = kOppositeSwitchingCouplingFactor,
      .total_cap_pf = ground_cap_pf + coupling_cap_pf,
      .timing_effective_cap_pf = ground_cap_pf + kOppositeSwitchingCouplingFactor * coupling_cap_pf,
  };
}

}  // namespace

auto Wrapper::queryWireResistance(int routing_layer, double length_um, std::optional<double> wire_width_um) const -> double
{
  if (!is_layout_ready()) {
    CTSLOG.warn(Loc::current(), "Wrapper: iDB layout is not ready for wire resistance query.");
    return 0.0;
  }
  if (routing_layer <= 0 || !std::isfinite(length_um) || length_um < 0.0) {
    CTSLOG.warn(Loc::current(), "Wrapper: invalid wire resistance query: layer=", routing_layer, ", length=", length_um, " um.");
    return 0.0;
  }
  auto* layer = queryRoutingLayer(_idb_layout, routing_layer, "resistance");
  const double width_um = resolveWidthUm(_idb_layout, layer, wire_width_um);
  return layer->get_resistance() * length_um / width_um;
}

auto Wrapper::queryWireCapacitance(int routing_layer, double length_um, std::optional<double> wire_width_um) const -> double
{
  if (!is_layout_ready()) {
    CTSLOG.warn(Loc::current(), "Wrapper: iDB layout is not ready for wire capacitance query.");
    return 0.0;
  }
  if (routing_layer <= 0 || !std::isfinite(length_um) || length_um < 0.0) {
    CTSLOG.warn(Loc::current(), "Wrapper: invalid wire capacitance query: layer=", routing_layer, ", length=", length_um, " um.");
    return 0.0;
  }
  auto* layer = queryRoutingLayer(_idb_layout, routing_layer, "capacitance");
  const double width_um = resolveWidthUm(_idb_layout, layer, wire_width_um);
  return buildWireCapacitanceProfile(layer, length_um, width_um).total_cap_pf;
}

auto Wrapper::queryRequiredWireResistance(int routing_layer, double length_um, std::optional<double> wire_width_um) const -> double
{
  requireWireRcQuery("resistance", routing_layer, length_um);
  auto* layer = queryRoutingLayer(_idb_layout, routing_layer, "resistance");
  const double width_um = resolveWidthUm(_idb_layout, layer, wire_width_um);
  return layer->get_resistance() * length_um / width_um;
}

auto Wrapper::queryRequiredWireCapacitance(int routing_layer, double length_um, std::optional<double> wire_width_um) const -> double
{
  requireWireRcQuery("capacitance", routing_layer, length_um);
  auto* layer = queryRoutingLayer(_idb_layout, routing_layer, "capacitance");
  const double width_um = resolveWidthUm(_idb_layout, layer, wire_width_um);
  return buildWireCapacitanceProfile(layer, length_um, width_um).total_cap_pf;
}

auto Wrapper::queryRequiredWireCapacitanceProfile(int routing_layer, double length_um, std::optional<double> wire_width_um) const
    -> WireCapacitanceProfile
{
  requireWireRcQuery("capacitance", routing_layer, length_um);
  auto* layer = queryRoutingLayer(_idb_layout, routing_layer, "capacitance");
  const double width_um = resolveWidthUm(_idb_layout, layer, wire_width_um);
  return buildWireCapacitanceProfile(layer, length_um, width_um);
}

auto Wrapper::queryRequiredClockTimingWireCapacitanceProfile(int routing_layer, double length_um, std::optional<double> wire_width_um) const
    -> WireCapacitanceProfile
{
  return queryRequiredWireCapacitanceProfile(routing_layer, length_um, wire_width_um);
}

auto Wrapper::queryConfiguredClockRouteSegmentRc(const Config& config) const -> ClockRouteSegmentRc
{
  const auto dbu_per_um = queryDbUnit();
  if (dbu_per_um <= 0) {
    CTSLOG.error(Loc::current(), "Wrapper: DBU-per-micron is unavailable for configured clock route segment RC.");
  }
  const auto routing_layer = requireRoutingLayer(config);
  const auto wire_width_um = resolveWireWidth(config);
  return ClockRouteSegmentRc{
      .dbu_per_um = dbu_per_um,
      .resistance_per_um_ohm = queryRequiredWireResistance(routing_layer, 1.0, wire_width_um),
      .capacitance_per_um_pf = queryRequiredWireCapacitance(routing_layer, 1.0, wire_width_um),
  };
}

}  // namespace icts
