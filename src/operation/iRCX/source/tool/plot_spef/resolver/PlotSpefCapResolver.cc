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
#include "resolver/PlotSpefCapResolver.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Geometry.hh"
#include "StringUtils.hh"

namespace ircx::plot_spef {
namespace {

constexpr double kScoreEpsilon = 1e-12;

struct EdgeCandidate
{
  EdgeRef ref;
  const Resistor* edge = nullptr;
};

using IncidentEdges = std::unordered_map<std::string, std::vector<EdgeCandidate>>;

enum class CapResolveMode
{
  kStarRc,
  kIrcx
};

auto detectResolveMode(const Model& model) -> CapResolveMode
{
  const std::string program = string::to_lower(model.program_name);
  const std::string vendor = string::to_lower(model.vendor_name);
  if (program.find("ircx") != std::string::npos || vendor.find("ecos") != std::string::npos) {
    return CapResolveMode::kIrcx;
  }
  if (program.find("starrc") != std::string::npos || vendor.find("synopsys") != std::string::npos) {
    return CapResolveMode::kStarRc;
  }
  return CapResolveMode::kStarRc;
}

auto edgeTieValue(const EdgeCandidate& candidate) -> std::size_t
{
  return edgeRefTieValue(candidate.ref);
}

class IncidentEdgeIndex
{
 public:
  explicit IncidentEdgeIndex(const Model& model)
  {
    for (std::size_t net_index = 0; net_index < model.nets.size(); ++net_index) {
      const auto& net = model.nets[net_index];
      for (std::size_t resistor_index = 0; resistor_index < net.resistors.size(); ++resistor_index) {
        const auto& resistor = net.resistors[resistor_index];
        EdgeCandidate candidate;
        candidate.ref = {.net_index = net_index, .resistor_index = resistor_index, .valid = true};
        candidate.edge = &resistor;
        incident_edges_[resistor.node1].push_back(candidate);
        incident_edges_[resistor.node2].push_back(candidate);
      }
    }
  }

  auto candidatesFor(const std::string& node_name) const -> const std::vector<EdgeCandidate>&
  {
    const auto it = incident_edges_.find(node_name);
    return it == incident_edges_.end() ? empty_candidates_ : it->second;
  }

 private:
  IncidentEdges incident_edges_;
  std::vector<EdgeCandidate> empty_candidates_;
};

class IrcxResolver
{
 public:
  IrcxResolver(const Model& model, const IncidentEdgeIndex& incident_edges) : model_(model), incident_edges_(incident_edges) {}

  auto resolve(Model& model) const -> void
  {
    std::unordered_map<std::string, double> coupling_votes;
    for (auto& net : model.nets) {
      for (auto& cap : net.coupling_caps) {
        cap.edge1 = chooseEdgeForNode(cap.node1);
        cap.edge2 = chooseEdgeForNode(cap.node2);
        if (cap.edge1.valid) {
          coupling_votes[nodeEdgeVoteKey(cap.node1, cap.edge1)] += cap.value;
        }
        if (cap.edge2.valid) {
          coupling_votes[nodeEdgeVoteKey(cap.node2, cap.edge2)] += cap.value;
        }
      }
    }

    for (auto& net : model.nets) {
      for (auto& cap : net.ground_caps) {
        const auto& candidates = incident_edges_.candidatesFor(cap.node1);
        if (candidates.empty()) {
          continue;
        }

        EdgeCandidate best_vote;
        double best_vote_value = 0.0;
        for (const auto& candidate : candidates) {
          const auto vote_it = coupling_votes.find(nodeEdgeVoteKey(cap.node1, candidate.ref));
          const double vote = vote_it == coupling_votes.end() ? 0.0 : vote_it->second;
          if (vote > best_vote_value) {
            best_vote = candidate;
            best_vote_value = vote;
          }
        }
        cap.edge1 = best_vote_value > 0.0 ? best_vote.ref : chooseEdgeForNode(cap.node1);
      }
    }
  }

 private:
  auto findNode(const std::string& name) const -> const Node*
  {
    const auto it = model_.nodes_by_name.find(name);
    return it == model_.nodes_by_name.end() ? nullptr : it->second;
  }

  static auto pointToBoxDistance2(const Node* node, const Resistor& edge) -> double
  {
    if (node == nullptr || !node->has_point) {
      return 0.0;
    }
    if (!edge.has_box) {
      return std::numeric_limits<double>::max() / 4.0;
    }
    return geom::point_to_rect_distance2(node->x, node->y, edge.llx, edge.lly, edge.urx, edge.ury);
  }

  auto chooseEdgeForNode(const std::string& node_name) const -> EdgeRef
  {
    const auto& candidates = incident_edges_.candidatesFor(node_name);
    if (candidates.empty()) {
      return {};
    }
    if (candidates.size() == 1) {
      return candidates.front().ref;
    }

    const Node* node = findNode(node_name);
    EdgeCandidate best;
    double best_score = -std::numeric_limits<double>::infinity();
    for (const auto& candidate : candidates) {
      const auto& edge = *candidate.edge;
      double score = 0.0;
      score += isWireResistor(edge) ? 10000.0 : -10000.0;
      if (node != nullptr && edge.has_layer && node->layer == edge.layer) {
        score += 2000.0;
      }
      if (edge.has_box) {
        score -= 0.001 * pointToBoxDistance2(node, edge);
      }
      score += 0.01 * resistorLength(edge);
      if (best.edge == nullptr || score > best_score
          || (std::abs(score - best_score) < kScoreEpsilon && edgeTieValue(candidate) < edgeTieValue(best))) {
        best = candidate;
        best_score = score;
      }
    }
    return best.edge == nullptr ? EdgeRef{} : best.ref;
  }

  const Model& model_;
  const IncidentEdgeIndex& incident_edges_;
};

class StarRcResolver
{
 public:
  StarRcResolver(int dbu, const IncidentEdgeIndex& incident_edges) : dbu_(dbu), incident_edges_(incident_edges) {}

  auto resolve(Model& model) const -> void
  {
    std::unordered_map<std::string, double> coupling_votes;
    for (auto& net : model.nets) {
      for (auto& cap : net.coupling_caps) {
        const auto [edge1, edge2] = chooseCouplingEdgePair(incident_edges_.candidatesFor(cap.node1), incident_edges_.candidatesFor(cap.node2));
        cap.edge1 = edge1;
        cap.edge2 = edge2;
        if (edge1.valid) {
          coupling_votes[nodeEdgeVoteKey(cap.node1, edge1)] += cap.value;
        }
        if (edge2.valid) {
          coupling_votes[nodeEdgeVoteKey(cap.node2, edge2)] += cap.value;
        }
      }
    }

    for (auto& net : model.nets) {
      for (auto& cap : net.ground_caps) {
        cap.edge1 = chooseGroundEdge(cap.node1, incident_edges_.candidatesFor(cap.node1), coupling_votes);
      }
    }
  }

 private:
  auto relationBetween(const Resistor& edge_a, const Resistor& edge_b) const -> geom::RectRelation<double>
  {
    if (!edge_a.has_box || !edge_b.has_box || dbu_ <= 0) {
      return {};
    }

    const double scale = static_cast<double>(dbu_);
    const double a_llx = static_cast<double>(edge_a.llx) / scale;
    const double a_lly = static_cast<double>(edge_a.lly) / scale;
    const double a_urx = static_cast<double>(edge_a.urx) / scale;
    const double a_ury = static_cast<double>(edge_a.ury) / scale;
    const double b_llx = static_cast<double>(edge_b.llx) / scale;
    const double b_lly = static_cast<double>(edge_b.lly) / scale;
    const double b_urx = static_cast<double>(edge_b.urx) / scale;
    const double b_ury = static_cast<double>(edge_b.ury) / scale;

    return geom::rect_relation(a_llx, a_lly, a_urx, a_ury, b_llx, b_lly, b_urx, b_ury);
  }

  auto pairGeometryScore(const Resistor& edge_a, const Resistor& edge_b) const -> double
  {
    const auto relation = relationBetween(edge_a, edge_b);
    double score = 0.0;
    score += isWireResistor(edge_a) ? 10000.0 : -10000.0;
    score += isWireResistor(edge_b) ? 10000.0 : -10000.0;

    int layer_delta = 99;
    if (edge_a.has_layer && edge_b.has_layer) {
      layer_delta = std::abs(edge_a.layer - edge_b.layer);
      score += 1000.0 / (1.0 + static_cast<double>(layer_delta));
    }

    if (edge_a.has_direction && edge_b.has_direction && edge_a.direction == edge_b.direction) {
      score += 800.0;
      if (edge_a.direction == 0) {
        score += 3000.0 * relation.overlap_x;
        score -= 500.0 * relation.gap_y;
      } else if (edge_a.direction == 1) {
        score += 3000.0 * relation.overlap_y;
        score -= 500.0 * relation.gap_x;
      }
    } else {
      score += 1000.0 * std::min(relation.overlap_x, relation.overlap_y);
      score -= 100.0 * (relation.gap_x + relation.gap_y);
    }

    if (layer_delta > 0) {
      score += 1200.0 * relation.overlap_x * relation.overlap_y;
      score += 50.0 * (relation.overlap_x + relation.overlap_y);
    }
    score += 0.01 * (resistorLength(edge_a) + resistorLength(edge_b));
    return score;
  }

  auto betterScoredPair(double score, const EdgeCandidate& edge_a, const EdgeCandidate& edge_b, double best_score,
                        const EdgeCandidate& best_a, const EdgeCandidate& best_b) const -> bool
  {
    if (score > best_score + kScoreEpsilon) {
      return true;
    }
    if (std::abs(score - best_score) > kScoreEpsilon) {
      return false;
    }
    const auto tie_a = edgeTieValue(edge_a);
    const auto tie_b = edgeTieValue(edge_b);
    const auto best_tie_a = edgeTieValue(best_a);
    const auto best_tie_b = edgeTieValue(best_b);
    return tie_a > best_tie_a || (tie_a == best_tie_a && tie_b > best_tie_b);
  }

  auto chooseCouplingEdgePair(const std::vector<EdgeCandidate>& candidates_a, const std::vector<EdgeCandidate>& candidates_b) const
      -> std::pair<EdgeRef, EdgeRef>
  {
    if (candidates_a.empty() || candidates_b.empty()) {
      return {};
    }

    double best_score = -std::numeric_limits<double>::infinity();
    EdgeCandidate best_a;
    EdgeCandidate best_b;
    for (const auto& edge_a : candidates_a) {
      for (const auto& edge_b : candidates_b) {
        const double score = pairGeometryScore(*edge_a.edge, *edge_b.edge);
        if (best_a.edge == nullptr || betterScoredPair(score, edge_a, edge_b, best_score, best_a, best_b)) {
          best_score = score;
          best_a = edge_a;
          best_b = edge_b;
        }
      }
    }

    return {best_a.ref, best_b.ref};
  }

  static auto betterGroundCandidate(const EdgeCandidate& candidate, const EdgeCandidate& best) -> bool
  {
    if (best.edge == nullptr) {
      return true;
    }
    const double candidate_length = resistorLength(*candidate.edge);
    const double best_length = resistorLength(*best.edge);
    if (candidate_length > best_length + kScoreEpsilon) {
      return true;
    }
    if (std::abs(candidate_length - best_length) > kScoreEpsilon) {
      return false;
    }
    return edgeTieValue(candidate) < edgeTieValue(best);
  }

  static auto chooseLongest(const std::vector<EdgeCandidate>& candidates, bool wire_only) -> EdgeRef
  {
    EdgeCandidate best;
    for (const auto& candidate : candidates) {
      if (wire_only && !isWireResistor(*candidate.edge)) {
        continue;
      }
      if (betterGroundCandidate(candidate, best)) {
        best = candidate;
      }
    }
    return best.edge == nullptr ? EdgeRef{} : best.ref;
  }

  static auto chooseGroundEdge(const std::string& node, const std::vector<EdgeCandidate>& candidates,
                               const std::unordered_map<std::string, double>& coupling_votes) -> EdgeRef
  {
    if (candidates.empty()) {
      return {};
    }

    EdgeCandidate best_vote_candidate;
    double best_vote = 0.0;
    for (const auto& candidate : candidates) {
      const auto vote_it = coupling_votes.find(nodeEdgeVoteKey(node, candidate.ref));
      const double vote = vote_it == coupling_votes.end() ? 0.0 : vote_it->second;
      if (vote > best_vote || (vote == best_vote && best_vote > 0.0 && betterGroundCandidate(candidate, best_vote_candidate))) {
        best_vote = vote;
        best_vote_candidate = candidate;
      }
    }
    if (best_vote > 0.0 && best_vote_candidate.edge != nullptr) {
      return best_vote_candidate.ref;
    }

    std::size_t wire_count = 0;
    for (const auto& candidate : candidates) {
      if (isWireResistor(*candidate.edge)) {
        ++wire_count;
      }
    }
    if (wire_count > 0) {
      return chooseLongest(candidates, true);
    }
    return chooseLongest(candidates, false);
  }

  int dbu_ = 1000;
  const IncidentEdgeIndex& incident_edges_;
};

}  // namespace

auto resolveCapacitorEdges(Model& model, const Config& config) -> void
{
  if (!config.plotCouplingCap() && !config.plotGroundCap()) {
    return;
  }

  const IncidentEdgeIndex incident_edges(model);
  switch (detectResolveMode(model)) {
    case CapResolveMode::kIrcx:
      IrcxResolver(model, incident_edges).resolve(model);
      break;
    case CapResolveMode::kStarRc:
      StarRcResolver(config.dbu, incident_edges).resolve(model);
      break;
  }
}

}  // namespace ircx::plot_spef
