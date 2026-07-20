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
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "RCXType.hpp"

namespace ircx {

class ProcessTable2D
{
 public:
  ProcessTable2D() = default;
  ~ProcessTable2D() = default;
  // getter
  std::vector<F64>& get_row_list() { return _row_list; }
  std::vector<F64>& get_column_list() { return _column_list; }
  std::vector<F64>& get_value_list() { return _value_list; }
  const std::vector<F64>& get_row_list() const { return _row_list; }
  const std::vector<F64>& get_column_list() const { return _column_list; }
  const std::vector<F64>& get_value_list() const { return _value_list; }
  // setter
  void set_row_list(const std::vector<F64>& row_list) { _row_list = row_list; }
  void set_column_list(const std::vector<F64>& column_list) { _column_list = column_list; }
  void set_value_list(const std::vector<F64>& value_list) { _value_list = value_list; }
  // function
  bool get_is_empty() const { return _row_list.empty() || _column_list.empty() || _value_list.empty(); }
  std::optional<F64> query(F64 row, F64 column) const;

 private:
  std::pair<size_t, size_t> get_bounding_index_pair(const std::vector<F64>& axis, F64 value) const;
  std::optional<F64> get_value(size_t row_idx, size_t column_idx) const;

  std::vector<F64> _row_list;
  std::vector<F64> _column_list;
  std::vector<F64> _value_list;
};

inline std::optional<F64> ProcessTable2D::query(F64 row, F64 column) const
{
  if (get_is_empty()) {
    return std::nullopt;
  }

  std::pair<size_t, size_t> row_index_pair = get_bounding_index_pair(_row_list, row);
  std::pair<size_t, size_t> column_index_pair = get_bounding_index_pair(_column_list, column);
  std::optional<F64> low_low_value = get_value(row_index_pair.first, column_index_pair.first);
  if (!low_low_value.has_value()) {
    return std::nullopt;
  }
  if (row_index_pair.first == row_index_pair.second && column_index_pair.first == column_index_pair.second) {
    return low_low_value;
  }

  F64 row_ratio = 0.0;
  if (row_index_pair.first != row_index_pair.second) {
    F64 row_delta = _row_list[row_index_pair.second] - _row_list[row_index_pair.first];
    if (row_delta != 0.0) {
      row_ratio = (row - _row_list[row_index_pair.first]) / row_delta;
    }
  }
  F64 column_ratio = 0.0;
  if (column_index_pair.first != column_index_pair.second) {
    F64 column_delta = _column_list[column_index_pair.second] - _column_list[column_index_pair.first];
    if (column_delta != 0.0) {
      column_ratio = (column - _column_list[column_index_pair.first]) / column_delta;
    }
  }

  if (row_index_pair.first == row_index_pair.second) {
    std::optional<F64> low_high_value = get_value(row_index_pair.first, column_index_pair.second);
    if (!low_high_value.has_value()) {
      return std::nullopt;
    }
    return std::lerp(low_low_value.value(), low_high_value.value(), column_ratio);
  }
  if (column_index_pair.first == column_index_pair.second) {
    std::optional<F64> high_low_value = get_value(row_index_pair.second, column_index_pair.first);
    if (!high_low_value.has_value()) {
      return std::nullopt;
    }
    return std::lerp(low_low_value.value(), high_low_value.value(), row_ratio);
  }

  std::optional<F64> low_high_value = get_value(row_index_pair.first, column_index_pair.second);
  std::optional<F64> high_low_value = get_value(row_index_pair.second, column_index_pair.first);
  std::optional<F64> high_high_value = get_value(row_index_pair.second, column_index_pair.second);
  if (!low_high_value.has_value() || !high_low_value.has_value() || !high_high_value.has_value()) {
    return std::nullopt;
  }
  F64 low_value = std::lerp(low_low_value.value(), low_high_value.value(), column_ratio);
  F64 high_value = std::lerp(high_low_value.value(), high_high_value.value(), column_ratio);
  return std::lerp(low_value, high_value, row_ratio);
}

inline std::pair<size_t, size_t> ProcessTable2D::get_bounding_index_pair(const std::vector<F64>& axis, F64 value) const
{
  if (value <= axis.front()) {
    return std::make_pair(0, 0);
  }
  if (value >= axis.back()) {
    size_t last_idx = axis.size() - 1;
    return std::make_pair(last_idx, last_idx);
  }

  std::vector<F64>::const_iterator high_iter = std::lower_bound(axis.begin(), axis.end(), value);
  size_t high_idx = static_cast<size_t>(std::distance(axis.begin(), high_iter));
  if (*high_iter == value) {
    return std::make_pair(high_idx, high_idx);
  }
  return std::make_pair(high_idx - 1, high_idx);
}

inline std::optional<F64> ProcessTable2D::get_value(size_t row_idx, size_t column_idx) const
{
  if (row_idx >= _row_list.size() || column_idx >= _column_list.size()) {
    return std::nullopt;
  }
  size_t value_idx = row_idx * _column_list.size() + column_idx;
  if (value_idx >= _value_list.size()) {
    return std::nullopt;
  }
  return _value_list[value_idx];
}

}  // namespace ircx
