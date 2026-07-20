// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of the License at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "ProcessEffectType.hpp"
#include "ProcessTable1D.hpp"
#include "ProcessViaEtchTable.hpp"

namespace ircx {

class ProcessVia
{
 public:
  ProcessVia() = default;
  ~ProcessVia() = default;
  // getter
  std::string& get_layer_name() { return _layer_name; }
  std::string& get_from_layer_name() { return _from_layer_name; }
  std::string& get_to_layer_name() { return _to_layer_name; }
  const std::string& get_layer_name() const { return _layer_name; }
  Micron get_area() const { return _area; }
  F64 get_resistance() const { return _resistance; }
  F64 get_resistivity() const { return _resistivity; }
  bool get_has_nominal_temperature() const { return _has_nominal_temperature; }
  F64 get_nominal_temperature() const { return _nominal_temperature; }
  F64 get_temperature_coefficient1() const { return _temperature_coefficient1; }
  F64 get_temperature_coefficient2() const { return _temperature_coefficient2; }
  ProcessTable1D& get_resistance_by_area_table() { return _resistance_by_area_table; }
  ProcessTable1D& get_temperature_coefficient1_by_area_table() { return _temperature_coefficient1_by_area_table; }
  ProcessTable1D& get_temperature_coefficient2_by_area_table() { return _temperature_coefficient2_by_area_table; }
  std::vector<ProcessViaEtchTable>& get_etch_table_list() { return _etch_table_list; }
  const std::vector<ProcessViaEtchTable>& get_etch_table_list() const { return _etch_table_list; }
  // setter
  void set_layer_name(const std::string& layer_name) { _layer_name = layer_name; }
  void set_from_layer_name(const std::string& from_layer_name) { _from_layer_name = from_layer_name; }
  void set_to_layer_name(const std::string& to_layer_name) { _to_layer_name = to_layer_name; }
  void set_area(Micron area) { _area = area; }
  void set_resistance(F64 resistance) { _resistance = resistance; }
  void set_resistivity(F64 resistivity) { _resistivity = resistivity; }
  void set_nominal_temperature(F64 nominal_temperature)
  {
    _nominal_temperature = nominal_temperature;
    _has_nominal_temperature = true;
  }
  void set_temperature_coefficient1(F64 temperature_coefficient1) { _temperature_coefficient1 = temperature_coefficient1; }
  void set_temperature_coefficient2(F64 temperature_coefficient2) { _temperature_coefficient2 = temperature_coefficient2; }
  // function
  std::optional<F64> query_resistance(F64 area) const;
  void query_temperature_coefficient(F64 area, F64& temperature_coefficient1, F64& temperature_coefficient2) const;
  std::pair<Micron, Micron> query_etch(ProcessEffectType effect_type, Micron width, Micron length) const;

 private:
  bool get_effect_is_applied(ProcessEffectType table_effect_type, ProcessEffectType query_effect_type) const;

  std::string _layer_name;
  std::string _from_layer_name;
  std::string _to_layer_name;
  Micron _area = 0.0;
  F64 _resistance = 0.0;
  F64 _resistivity = 0.0;
  bool _has_nominal_temperature = false;
  F64 _nominal_temperature = 25.0;
  F64 _temperature_coefficient1 = 0.0;
  F64 _temperature_coefficient2 = 0.0;
  ProcessTable1D _resistance_by_area_table;
  ProcessTable1D _temperature_coefficient1_by_area_table;
  ProcessTable1D _temperature_coefficient2_by_area_table;
  std::vector<ProcessViaEtchTable> _etch_table_list;
};

inline std::optional<F64> ProcessVia::query_resistance(F64 area) const
{
  if (_resistance > 0.0) {
    return _resistance;
  }
  return _resistance_by_area_table.query(area);
}

inline void ProcessVia::query_temperature_coefficient(F64 area,
                                                       F64& temperature_coefficient1,
                                                       F64& temperature_coefficient2) const
{
  temperature_coefficient1 = _temperature_coefficient1;
  temperature_coefficient2 = _temperature_coefficient2;
  std::optional<F64> coefficient1 = _temperature_coefficient1_by_area_table.query(area);
  std::optional<F64> coefficient2 = _temperature_coefficient2_by_area_table.query(area);
  if (coefficient1.has_value()) {
    temperature_coefficient1 = coefficient1.value();
  }
  if (coefficient2.has_value()) {
    temperature_coefficient2 = coefficient2.value();
  }
}

inline std::pair<Micron, Micron> ProcessVia::query_etch(ProcessEffectType effect_type, Micron width, Micron length) const
{
  Micron length_etch = 0.0;
  Micron width_etch = 0.0;
  for (const ProcessViaEtchTable& etch_table : _etch_table_list) {
    if (!get_effect_is_applied(etch_table.get_effect_type(), effect_type)) {
      continue;
    }
    std::optional<F64> table_length_etch = etch_table.get_length_table().query(width, length);
    std::optional<F64> table_width_etch = etch_table.get_width_table().query(width, length);
    if (table_length_etch.has_value()) {
      length_etch += table_length_etch.value();
    }
    if (table_width_etch.has_value()) {
      width_etch += table_width_etch.value();
    }
  }
  return std::make_pair(length_etch, width_etch);
}

inline bool ProcessVia::get_effect_is_applied(ProcessEffectType table_effect_type, ProcessEffectType query_effect_type) const
{
  return table_effect_type == ProcessEffectType::kBoth || table_effect_type == query_effect_type;
}

}  // namespace ircx
