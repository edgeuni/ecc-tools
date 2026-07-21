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

#include "RCXHeader.hpp"

namespace ircx {

class ProcessTable1D
{
 public:
  ProcessTable1D() = default;
  ~ProcessTable1D() = default;
  // getter
  std::vector<std::pair<double, double>>& get_entry_list() { return _entry_list; }
  const std::vector<std::pair<double, double>>& get_entry_list() const { return _entry_list; }
  // setter
  void set_entry_list(const std::vector<std::pair<double, double>>& entry_list)
  {
    _entry_list = entry_list;
    sort_entry_list();
  }
  // function
  void add_entry(double key, double value)
  {
    _entry_list.emplace_back(key, value);
    sort_entry_list();
  }
  bool get_is_empty() const { return _entry_list.empty(); }
  std::optional<double> query(double key) const;

 private:
  static bool isEntryLessThanKey(const std::pair<double, double>& entry, double key);
  static bool isEntryLess(const std::pair<double, double>& first_entry, const std::pair<double, double>& second_entry);
  void sort_entry_list();

  std::vector<std::pair<double, double>> _entry_list;
};

inline std::optional<double> ProcessTable1D::query(double key) const
{
  if (_entry_list.empty()) {
    return std::nullopt;
  }
  if (_entry_list.size() == 1 || key <= _entry_list.front().first) {
    return _entry_list.front().second;
  }
  if (key >= _entry_list.back().first) {
    return _entry_list.back().second;
  }

  std::vector<std::pair<double, double>>::const_iterator high_iter
      = std::lower_bound(_entry_list.begin(), _entry_list.end(), key, isEntryLessThanKey);
  if (high_iter == _entry_list.end()) {
    return _entry_list.back().second;
  }
  if (high_iter->first == key) {
    return high_iter->second;
  }

  std::vector<std::pair<double, double>>::const_iterator low_iter = std::prev(high_iter);
  double key_delta = high_iter->first - low_iter->first;
  if (key_delta == 0.0) {
    return high_iter->second;
  }
  double ratio = (key - low_iter->first) / key_delta;
  return std::lerp(low_iter->second, high_iter->second, ratio);
}

inline bool ProcessTable1D::isEntryLessThanKey(const std::pair<double, double>& entry, double key)
{
  return entry.first < key;
}

inline bool ProcessTable1D::isEntryLess(const std::pair<double, double>& first_entry, const std::pair<double, double>& second_entry)
{
  return first_entry.first < second_entry.first;
}

inline void ProcessTable1D::sort_entry_list()
{
  std::sort(_entry_list.begin(), _entry_list.end(), isEntryLess);
}

}  // namespace ircx
