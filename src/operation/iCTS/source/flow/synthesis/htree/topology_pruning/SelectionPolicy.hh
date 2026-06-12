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
 * @file SelectionPolicy.hh
 * @author Dawn Li (dawnli619215645@gmail.com)
 * @date 2026-06-13
 * @brief Delay-margin bounded selection over delay-power Pareto fronts.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace icts::htree {

/// Selects an entry index on an already-built delay-power Pareto front.
///
/// The front is consumed in its caller-established deterministic order:
/// - margin <= 0 (disabled): legacy Pareto-median behavior, returning the
///   median index ((size - 1) / 2) of the front in its current order.
/// - margin > 0: bound = front_min_delay x (1.0 + margin); among entries with
///   delay <= bound the minimum-power entry wins; power ties prefer the
///   smaller delay; remaining ties keep the first entry in front order. The
///   min-delay entry is always within the bound, so a non-empty front always
///   yields a selection.
///
/// Returns std::nullopt only for an empty front.
template <typename EntryT, typename DelayFn, typename PowerFn>
auto SelectDelayBoundedIndex(const std::vector<EntryT>& front, double margin, const DelayFn& delay_of, const PowerFn& power_of)
    -> std::optional<std::size_t>
{
  if (front.empty()) {
    return std::nullopt;
  }
  if (margin <= 0.0) {
    return (front.size() - 1U) / 2U;
  }

  double front_min_delay = delay_of(front.front());
  for (std::size_t index = 1U; index < front.size(); ++index) {
    front_min_delay = std::min(front_min_delay, delay_of(front.at(index)));
  }
  const double delay_bound = front_min_delay * (1.0 + margin);

  std::optional<std::size_t> selected_index;
  for (std::size_t index = 0U; index < front.size(); ++index) {
    const double entry_delay = delay_of(front.at(index));
    if (entry_delay > delay_bound) {
      continue;
    }
    if (!selected_index.has_value()) {
      selected_index = index;
      continue;
    }
    const double selected_power = power_of(front.at(*selected_index));
    const double entry_power = power_of(front.at(index));
    if (entry_power < selected_power || (entry_power == selected_power && entry_delay < delay_of(front.at(*selected_index)))) {
      selected_index = index;
    }
  }
  return selected_index;
}

}  // namespace icts::htree
