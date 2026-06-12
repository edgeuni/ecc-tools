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
 * @file SinkLoadRegion.cc
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-04-24
 * @brief Sink-load-region legality and boundary-cap coverage checks for H-tree candidates.
 */

#include "synthesis/htree/region/SinkLoadRegion.hh"

#include <glog/logging.h>
#include <stdlib.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "BufferingPattern.hh"
#include "ClockRouteSegmentRc.hh"
#include "Clustering.hh"
#include "FastClustering.hh"
#include "HTreeTopologyChar.hh"
#include "HTreeTopologyPattern.hh"
#include "Log.hh"
#include "PatternId.hh"
#include "Pin.hh"
#include "Point.hh"
#include "TopologyConfig.hh"
#include "Tree.hh"
#include "io/Wrapper.hh"
#include "synthesis/htree/constraint/Constraint.hh"
#include "synthesis/htree/segment_pruning/SegmentPatternLibrary.hh"
#include "synthesis/htree/segment_pruning/TopologyPatternLibrary.hh"

namespace icts::htree {
namespace {

auto CalcMedianCapPf(std::vector<double> caps_pf) -> double
{
  if (caps_pf.empty()) {
    return 0.0;
  }

  std::ranges::sort(caps_pf);
  const std::size_t middle = caps_pf.size() / 2U;
  if ((caps_pf.size() & 1U) == 1U) {
    return caps_pf.at(middle);
  }
  return (caps_pf.at(middle - 1U) + caps_pf.at(middle)) * 0.5;
}

auto BuildCapDistributionStats(const std::vector<double>& caps_pf) -> CapDistributionStats
{
  CapDistributionStats stats;
  if (caps_pf.empty()) {
    return stats;
  }

  stats.group_count = caps_pf.size();
  stats.cap_min_pf = *std::ranges::min_element(caps_pf);
  stats.cap_max_pf = *std::ranges::max_element(caps_pf);

  double total_cap_pf = 0.0;
  for (const double cap_pf : caps_pf) {
    total_cap_pf += cap_pf;
  }
  stats.cap_mean_pf = total_cap_pf / static_cast<double>(caps_pf.size());
  stats.cap_median_pf = CalcMedianCapPf(caps_pf);
  return stats;
}

auto collectSinkPinCapPfByPin(Wrapper& wrapper, const std::vector<SinkLoadRegionBoundaryGroup>& groups)
    -> std::unordered_map<const Pin*, double>
{
  std::unordered_map<const Pin*, double> sink_pin_cap_pf_by_pin;
  for (const auto& group : groups) {
    if (group.loads == nullptr) {
      continue;
    }
    sink_pin_cap_pf_by_pin.reserve(sink_pin_cap_pf_by_pin.size() + group.loads->size());
    for (const auto* pin : *group.loads) {
      if (pin == nullptr) {
        continue;
      }
      sink_pin_cap_pf_by_pin[pin] = std::max(0.0, wrapper.queryPinCapacitance(pin));
    }
  }
  return sink_pin_cap_pf_by_pin;
}

auto BuildLeafElectricalConfig(const SinkLoadRegionLegalityInput& input, const std::vector<SinkLoadRegionBoundaryGroup>& groups)
    -> ClusterConfig
{
  const double max_cap = input.has_max_cap ? input.max_cap_pf : std::numeric_limits<double>::infinity();
  LOG_FATAL_IF(input.wrapper == nullptr) << "HTree: Wrapper is unavailable for sink-load-region legality.";
  auto config = FastClustering::buildElectricalBaseConfig(input.max_fanout, max_cap);
  config.clock_route_segment_rc = input.clock_route_segment_rc;
  config.sink_pin_cap_pf_by_pin = collectSinkPinCapPfByPin(*input.wrapper, groups);
  config.enable_exact_cap = true;
  config.always_build_exact_cap = true;
  config.scoring_strategy = ClusterScoringStrategy::kTotalWirelength;
  return config;
}

auto ResolveBottomMostBufferedLevel(const HTreeTopologyPattern& topology_pattern, const BufferPatternLibrary& segment_pattern_library)
    -> int
{
  const auto& level_segment_pattern_ids = topology_pattern.get_level_segment_pattern_ids();
  for (int level = static_cast<int>(level_segment_pattern_ids.size()) - 1; level >= 0; --level) {
    const auto segment_pattern_id = level_segment_pattern_ids.at(static_cast<std::size_t>(level));
    const auto* segment_pattern = segment_pattern_library.find(segment_pattern_id);
    LOG_FATAL_IF(segment_pattern == nullptr) << "HTree: missing segment pattern metadata while resolving sink-load-region boundary.";
    if (!segment_pattern->get_buffer_positions().empty()) {
      return level;
    }
  }
  return -1;
}

auto ResolveSinkLoadRegionLegalitySignature(const HTreeTopologyPattern& topology_pattern,
                                            const BufferPatternLibrary& segment_pattern_library) -> SinkLoadRegionLegalitySignature
{
  const int bottom_most_buffered_level = ResolveBottomMostBufferedLevel(topology_pattern, segment_pattern_library);
  SinkLoadRegionLegalitySignature signature;
  signature.bottom_most_buffered_level = bottom_most_buffered_level;
  if (bottom_most_buffered_level >= 0) {
    signature.segment_pattern_id
        = topology_pattern.get_level_segment_pattern_ids().at(static_cast<std::size_t>(bottom_most_buffered_level));
  }
  return signature;
}

auto BuildSinkLoadRegionFeasibilityReason(std::size_t node_id, const Point<int>& anchor, const std::string& detail) -> std::string
{
  std::ostringstream stream;
  stream << "htree_load_group_node_" << node_id << " anchor=(" << anchor.get_x() << "," << anchor.get_y() << ") " << detail;
  return stream.str();
}

auto InterpolateBoundaryAnchor(const Point<int>& source, const Point<int>& sink, double normalized_position) -> Point<int>
{
  const double clamped_position = std::clamp(normalized_position, 0.0, 1.0);
  const int dx = sink.get_x() - source.get_x();
  const int dy = sink.get_y() - source.get_y();
  const int total_distance = std::abs(dx) + std::abs(dy);
  if (total_distance == 0) {
    return source;
  }

  const int target_distance = static_cast<int>(std::lround(clamped_position * static_cast<double>(total_distance)));
  const int x_step = std::min(std::abs(dx), target_distance);
  const int y_step = std::max(0, target_distance - x_step);
  const int x = source.get_x() + ((dx >= 0) ? x_step : -x_step);
  const int y = source.get_y() + ((dy >= 0) ? y_step : -y_step);
  return Point<int>(x, y);
}

struct SinkLoadRegionBoundaryCollection
{
  std::vector<SinkLoadRegionBoundaryGroup> groups;
  SinkLoadRegionViolation violation = SinkLoadRegionViolation::kNone;
  std::string failure_reason;
};

auto MakeBoundaryCollectionFailure(std::vector<SinkLoadRegionBoundaryGroup> groups, SinkLoadRegionViolation violation, std::string reason)
    -> SinkLoadRegionBoundaryCollection
{
  return SinkLoadRegionBoundaryCollection{
      .groups = std::move(groups),
      .violation = violation,
      .failure_reason = std::move(reason),
  };
}

auto CollectSinkLoadRegionBoundaryGroups(const Tree& topology, const SinkLoadRegionLegalitySignature& signature,
                                         const BufferPatternLibrary& segment_pattern_library) -> SinkLoadRegionBoundaryCollection
{
  std::vector<SinkLoadRegionBoundaryGroup> groups;

  const auto* root_node = topology.get_node(topology.get_root());
  if (root_node == nullptr) {
    return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kMissingTopologyRoot, "missing_topology_root");
  }

  if (signature.bottom_most_buffered_level < 0) {
    if (root_node->get_loads().empty()) {
      return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kEmptyLoadGroup, "empty_root_load_group");
    }
    groups.push_back(SinkLoadRegionBoundaryGroup{
        .node_id = root_node->get_id(),
        .anchor = root_node->get_position(),
        .loads = &root_node->get_loads(),
    });
    return SinkLoadRegionBoundaryCollection{
        .groups = std::move(groups),
        .violation = SinkLoadRegionViolation::kNone,
        .failure_reason = {},
    };
  }

  const auto topology_levels = topology.levels();
  const std::size_t boundary_level = static_cast<std::size_t>(signature.bottom_most_buffered_level) + 1U;
  if (boundary_level >= topology_levels.size()) {
    return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kMissingTopologyLevel,
                                         "missing_sink_load_region_boundary_level");
  }

  const auto* segment_pattern = segment_pattern_library.find(signature.segment_pattern_id);
  if (segment_pattern == nullptr) {
    return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kMissingSegmentPattern,
                                         "missing_boundary_segment_pattern");
  }
  if (segment_pattern->get_buffer_positions().empty()) {
    return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kMissingBufferPosition,
                                         "missing_boundary_buffer_position");
  }

  const double last_buffer_position = segment_pattern->get_buffer_positions().back();
  groups.reserve(topology_levels.at(boundary_level).size());
  for (const auto node_id : topology_levels.at(boundary_level)) {
    const auto* node = topology.get_node(node_id);
    if (node == nullptr) {
      return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kMissingTopologyNode,
                                           "missing_boundary_topology_node");
    }
    if (node->get_loads().empty()) {
      continue;
    }

    const auto* parent_node = topology.get_node(node->get_parent());
    if (parent_node == nullptr) {
      return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kMissingTopologyNode,
                                           "missing_boundary_parent_node");
    }

    groups.push_back(SinkLoadRegionBoundaryGroup{
        .node_id = node_id,
        .anchor = InterpolateBoundaryAnchor(parent_node->get_position(), node->get_position(), last_buffer_position),
        .loads = &node->get_loads(),
    });
  }

  if (groups.empty()) {
    return MakeBoundaryCollectionFailure(std::move(groups), SinkLoadRegionViolation::kEmptyLoadGroup, "empty_sink_load_region_groups");
  }
  return SinkLoadRegionBoundaryCollection{
      .groups = std::move(groups),
      .violation = SinkLoadRegionViolation::kNone,
      .failure_reason = {},
  };
}

auto EvaluateSinkLoadRegionLegality(const Tree& topology, const SinkLoadRegionLegalitySignature& signature,
                                    const BufferPatternLibrary& segment_pattern_library,
                                    const SinkLoadRegionLegalityContext& legality_context) -> SinkLoadRegionLegalitySummary
{
  SinkLoadRegionLegalitySummary result;
  result.bottom_most_buffered_level = signature.bottom_most_buffered_level;
  result.segment_pattern_id = signature.segment_pattern_id;

  const auto collection = CollectSinkLoadRegionBoundaryGroups(topology, signature, segment_pattern_library);
  if (collection.violation != SinkLoadRegionViolation::kNone) {
    result.failure_reason = collection.failure_reason;
    result.violation = collection.violation;
    return result;
  }

  const auto electrical_config = BuildLeafElectricalConfig(legality_context.input, collection.groups);
  const std::size_t max_fanout = legality_context.input.max_fanout;
  std::vector<SinkLoadRegionSplitPlan> group_split_plans(collection.groups.size());
  for (std::size_t group_index = 0; group_index < collection.groups.size(); ++group_index) {
    const auto& group = collection.groups.at(group_index);
    const auto* loads = group.loads;
    if (loads == nullptr || loads->empty()) {
      result.violation = SinkLoadRegionViolation::kEmptyLoadGroup;
      result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, "empty_group_loads");
      return result;
    }
    if (max_fanout > 0U && loads->size() > max_fanout) {
      auto split_plan = SplitSinkLoadRegionGroup(*loads, max_fanout);
      if (!split_plan.feasible) {
        std::ostringstream detail;
        detail << "fanout_violation load_count=" << loads->size() << ", max_fanout=" << max_fanout << ", split=infeasible";
        result.violation = SinkLoadRegionViolation::kFanout;
        result.monotone_hard_fail = true;
        result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
        return result;
      }
      group_split_plans.at(group_index) = std::move(split_plan);
      continue;
    }

    const auto lower_bound = Clustering::evaluateClusterElectrical(*loads, group.anchor, electrical_config, false);
    if (!lower_bound.legal) {
      if (lower_bound.violation == ClusterElectricalViolation::kFanout) {
        std::ostringstream detail;
        detail << "fanout_violation load_count=" << loads->size() << ", max_fanout=" << max_fanout;
        result.violation = SinkLoadRegionViolation::kFanout;
        result.monotone_hard_fail = true;
        result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
      } else if (lower_bound.violation == ClusterElectricalViolation::kCapacitance) {
        std::ostringstream detail;
        detail << "pin_cap_lower_bound_violation total_cap_pf=" << lower_bound.summary.total_cap_pf;
        if (legality_context.input.has_max_cap) {
          detail << ", max_cap_pf=" << legality_context.input.max_cap_pf;
        }
        result.violation = SinkLoadRegionViolation::kPinCapLowerBound;
        result.monotone_hard_fail = true;
        result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
      } else {
        result.violation = SinkLoadRegionViolation::kEmptyLoadGroup;
        result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, "lower_bound_evaluation_failed");
      }
      return result;
    }
  }

  std::vector<double> total_caps_pf;
  total_caps_pf.reserve(collection.groups.size());
  std::size_t split_group_count = 0U;
  std::size_t split_extra_buffer_count = 0U;
  const auto& route_segment_rc = legality_context.input.clock_route_segment_rc;
  for (std::size_t group_index = 0; group_index < collection.groups.size(); ++group_index) {
    const auto& group = collection.groups.at(group_index);
    const auto* loads = group.loads;
    LOG_FATAL_IF(loads == nullptr || loads->empty()) << "HTree: sink-load-region boundary group lost its load set.";

    const auto& split_plan = group_split_plans.at(group_index);
    if (!split_plan.feasible) {
      const auto exact = Clustering::evaluateClusterElectrical(*loads, group.anchor, electrical_config, true);
      if (!exact.legal) {
        if (exact.violation == ClusterElectricalViolation::kRoutingFailed) {
          result.violation = SinkLoadRegionViolation::kRoutingFailed;
          result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, "routing_failed");
        } else if (exact.violation == ClusterElectricalViolation::kCapacitance) {
          std::ostringstream detail;
          detail << "cap_violation total_cap_pf=" << exact.summary.total_cap_pf;
          if (legality_context.input.has_max_cap) {
            detail << ", max_cap_pf=" << legality_context.input.max_cap_pf;
          }
          result.violation = SinkLoadRegionViolation::kCapacitance;
          result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
        } else if (exact.violation == ClusterElectricalViolation::kFanout) {
          std::ostringstream detail;
          detail << "fanout_violation load_count=" << loads->size() << ", max_fanout=" << max_fanout;
          result.violation = SinkLoadRegionViolation::kFanout;
          result.monotone_hard_fail = true;
          result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
        } else {
          result.violation = SinkLoadRegionViolation::kEmptyLoadGroup;
          result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, "exact_evaluation_failed");
        }
        return result;
      }
      total_caps_pf.push_back(exact.summary.total_cap_pf);
      continue;
    }

    // Split-remediated group: every subgroup must be exact-legal from its own
    // sub-buffer site, and the upstream buffer now drives the sub-buffer pins.
    for (std::size_t subgroup_index = 0; subgroup_index < split_plan.subgroups.size(); ++subgroup_index) {
      const auto exact = Clustering::evaluateClusterElectrical(split_plan.subgroups.at(subgroup_index),
                                                               split_plan.centers.at(subgroup_index), electrical_config, true);
      if (!exact.legal) {
        std::ostringstream detail;
        detail << "split_subgroup_violation subgroup=" << subgroup_index
               << ", load_count=" << split_plan.subgroups.at(subgroup_index).size() << ", total_cap_pf=" << exact.summary.total_cap_pf;
        result.violation = exact.violation == ClusterElectricalViolation::kRoutingFailed ? SinkLoadRegionViolation::kRoutingFailed
                                                                                         : SinkLoadRegionViolation::kCapacitance;
        result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
        return result;
      }
    }

    double upstream_cap_pf
        = static_cast<double>(split_plan.subgroups.size()) * std::max(0.0, legality_context.input.split_buffer_input_cap_pf);
    if (route_segment_rc.dbu_per_um > 0) {
      for (const auto& center : split_plan.centers) {
        const auto distance_dbu = std::llabs(static_cast<long long>(center.get_x()) - static_cast<long long>(group.anchor.get_x()))
                                  + std::llabs(static_cast<long long>(center.get_y()) - static_cast<long long>(group.anchor.get_y()));
        upstream_cap_pf += static_cast<double>(distance_dbu) / static_cast<double>(route_segment_rc.dbu_per_um)
                           * route_segment_rc.capacitance_per_um_pf;
      }
    }
    if (legality_context.input.has_max_cap && upstream_cap_pf > legality_context.input.max_cap_pf) {
      std::ostringstream detail;
      detail << "split_upstream_cap_violation total_cap_pf=" << upstream_cap_pf << ", max_cap_pf=" << legality_context.input.max_cap_pf;
      result.violation = SinkLoadRegionViolation::kCapacitance;
      result.failure_reason = BuildSinkLoadRegionFeasibilityReason(group.node_id, group.anchor, detail.str());
      return result;
    }
    total_caps_pf.push_back(upstream_cap_pf);
    ++split_group_count;
    split_extra_buffer_count += split_plan.subgroups.size();
  }

  result.split_group_count = split_group_count;
  result.split_extra_buffer_count = split_extra_buffer_count;
  result.cap_distribution = BuildCapDistributionStats(total_caps_pf);
  result.required_leaf_load_cap_pf = result.cap_distribution.cap_max_pf;
  result.required_leaf_load_cap_covering_idx = CoveringBoundaryIndex(result.required_leaf_load_cap_pf, legality_context.cap_lattice);
  result.violation = SinkLoadRegionViolation::kNone;
  result.legal = true;
  return result;
}

}  // namespace

auto SplitSinkLoadRegionGroup(const std::vector<Pin*>& loads, std::size_t max_fanout) -> SinkLoadRegionSplitPlan
{
  SinkLoadRegionSplitPlan plan;
  if (max_fanout == 0U || loads.size() <= max_fanout || loads.size() > max_fanout * max_fanout) {
    return plan;
  }

  std::vector<Pin*> ordered_loads;
  ordered_loads.reserve(loads.size());
  for (auto* pin : loads) {
    if (pin != nullptr) {
      ordered_loads.push_back(pin);
    }
  }
  if (ordered_loads.size() <= max_fanout) {
    return plan;
  }

  // Canonical order keeps legality evaluation and embedding materialization on
  // identical subgroups without plumbing verdicts between the two phases.
  const auto canonical_less = [](const Pin* lhs, const Pin* rhs) -> bool {
    const auto& lhs_location = lhs->get_location();
    const auto& rhs_location = rhs->get_location();
    if (lhs_location.get_x() != rhs_location.get_x()) {
      return lhs_location.get_x() < rhs_location.get_x();
    }
    if (lhs_location.get_y() != rhs_location.get_y()) {
      return lhs_location.get_y() < rhs_location.get_y();
    }
    return lhs->get_name() < rhs->get_name();
  };
  std::ranges::sort(ordered_loads, canonical_less);

  std::vector<std::vector<Pin*>> pending;
  pending.push_back(std::move(ordered_loads));
  while (!pending.empty()) {
    auto group = std::move(pending.back());
    pending.pop_back();
    if (group.size() <= max_fanout) {
      plan.subgroups.push_back(std::move(group));
      continue;
    }

    int min_x = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    int min_y = std::numeric_limits<int>::max();
    int max_y = std::numeric_limits<int>::min();
    for (const auto* pin : group) {
      const auto& location = pin->get_location();
      min_x = std::min(min_x, location.get_x());
      max_x = std::max(max_x, location.get_x());
      min_y = std::min(min_y, location.get_y());
      max_y = std::max(max_y, location.get_y());
    }
    const bool split_on_x = (static_cast<long long>(max_x) - min_x) >= (static_cast<long long>(max_y) - min_y);
    std::ranges::sort(group, [split_on_x, &canonical_less](const Pin* lhs, const Pin* rhs) -> bool {
      const auto lhs_axis = split_on_x ? lhs->get_location().get_x() : lhs->get_location().get_y();
      const auto rhs_axis = split_on_x ? rhs->get_location().get_x() : rhs->get_location().get_y();
      if (lhs_axis != rhs_axis) {
        return lhs_axis < rhs_axis;
      }
      return canonical_less(lhs, rhs);
    });

    const auto split_point = static_cast<std::ptrdiff_t>(group.size() / 2U);
    pending.emplace_back(group.begin(), group.begin() + split_point);
    pending.emplace_back(group.begin() + split_point, group.end());
  }

  if (plan.subgroups.size() > max_fanout) {
    plan.subgroups.clear();
    return plan;
  }

  plan.centers.reserve(plan.subgroups.size());
  for (const auto& subgroup : plan.subgroups) {
    long long sum_x = 0;
    long long sum_y = 0;
    for (const auto* pin : subgroup) {
      sum_x += pin->get_location().get_x();
      sum_y += pin->get_location().get_y();
    }
    const auto count = static_cast<double>(subgroup.size());
    plan.centers.emplace_back(static_cast<int>(std::llround(static_cast<double>(sum_x) / count)),
                              static_cast<int>(std::llround(static_cast<double>(sum_y) / count)));
  }
  plan.feasible = true;
  return plan;
}

auto ResolveSinkLoadRegionLegality(const Tree& topology, PatternId topology_pattern_id, const TopologyPatternLibrary& topology_library,
                                   const BufferPatternLibrary& segment_pattern_library, SinkLoadRegionLegalityContext& legality_context)
    -> SinkLoadRegionLegalitySummary
{
  const auto topology_pattern = topology_library.materialize(topology_pattern_id);
  const auto signature = ResolveSinkLoadRegionLegalitySignature(topology_pattern, segment_pattern_library);
  if (signature.bottom_most_buffered_level <= legality_context.max_monotone_failed_level) {
    SinkLoadRegionLegalitySummary result;
    result.bottom_most_buffered_level = signature.bottom_most_buffered_level;
    result.segment_pattern_id = signature.segment_pattern_id;
    result.violation = SinkLoadRegionViolation::kFanout;
    std::ostringstream detail;
    detail << "monotone_pruned_by_bottom_most_buffered_level threshold=" << legality_context.max_monotone_failed_level
           << ", candidate_level=" << signature.bottom_most_buffered_level;
    result.failure_reason = detail.str();
    return result;
  }

  const auto cache_it = legality_context.result_by_signature.find(signature);
  if (cache_it != legality_context.result_by_signature.end()) {
    return cache_it->second;
  }

  auto evaluated = EvaluateSinkLoadRegionLegality(topology, signature, segment_pattern_library, legality_context);
  if (!evaluated.legal && evaluated.monotone_hard_fail
      && signature.bottom_most_buffered_level > legality_context.max_monotone_failed_level) {
    legality_context.max_monotone_failed_level = signature.bottom_most_buffered_level;
    legality_context.first_monotone_hard_fail_reason = evaluated.failure_reason;
    LOG_WARNING << "HTree: sink-load-region monotone threshold raised to bottom-most buffered level "
                << signature.bottom_most_buffered_level << " by hard violation: " << evaluated.failure_reason;
  }
  return legality_context.result_by_signature.emplace(signature, std::move(evaluated)).first->second;
}

auto FilterSinkLoadRegionLegalEntries(const std::vector<HTreeTopologyChar>& entries, const Tree& topology,
                                      const TopologyPatternLibrary& topology_library, const BufferPatternLibrary& segment_pattern_library,
                                      SinkLoadRegionLegalityContext& legality_context) -> SinkLoadRegionEntryFilterBuild
{
  SinkLoadRegionEntryFilterBuild result;
  result.output.entries.reserve(entries.size());
  for (const auto& entry : entries) {
    const auto legality
        = ResolveSinkLoadRegionLegality(topology, entry.get_pattern_id(), topology_library, segment_pattern_library, legality_context);
    if (!legality.legal) {
      if (result.summary.first_failure_reason.empty()) {
        result.summary.first_failure_reason = legality.failure_reason;
      }
      continue;
    }
    result.summary.max_split_group_count = std::max(result.summary.max_split_group_count, legality.split_group_count);
    result.summary.max_split_extra_buffer_count = std::max(result.summary.max_split_extra_buffer_count, legality.split_extra_buffer_count);
    result.output.entries.push_back(entry);
  }
  return result;
}

}  // namespace icts::htree
