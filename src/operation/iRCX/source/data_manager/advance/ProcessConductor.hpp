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
#include "ProcessEtchTable.hpp"
#include "ProcessTable1D.hpp"
#include "ProcessTable2D.hpp"

namespace ircx {

class ProcessConductor
{
 public:
  ProcessConductor() = default;
  ~ProcessConductor() = default;
  // getter
  std::string& get_layer_name() { return _layer_name; }
  const std::string& get_layer_name() const { return _layer_name; }
  Micron get_thickness() const { return _thickness; }
  F64 get_sheet_resistance() const { return _sheet_resistance; }
  F64 get_resistivity() const { return _resistivity; }
  bool get_has_nominal_temperature() const { return _has_nominal_temperature; }
  F64 get_nominal_temperature() const { return _nominal_temperature; }
  F64 get_temperature_coefficient1() const { return _temperature_coefficient1; }
  F64 get_temperature_coefficient2() const { return _temperature_coefficient2; }
  Micron get_etch() const { return _etch; }
  Micron get_resistive_only_etch() const { return _resistive_only_etch; }
  Micron get_capacitive_only_etch() const { return _capacitive_only_etch; }
  ProcessTable1D& get_sheet_resistance_by_width_table() { return _sheet_resistance_by_width_table; }
  ProcessTable1D& get_temperature_coefficient1_by_width_table() { return _temperature_coefficient1_by_width_table; }
  ProcessTable1D& get_temperature_coefficient2_by_width_table() { return _temperature_coefficient2_by_width_table; }
  ProcessTable2D& get_sheet_resistance_by_width_spacing_table() { return _sheet_resistance_by_width_spacing_table; }
  ProcessTable2D& get_resistivity_by_width_thickness_table() { return _resistivity_by_width_thickness_table; }
  ProcessTable2D& get_resistivity_by_width_spacing_table() { return _resistivity_by_width_spacing_table; }
  std::vector<ProcessEtchTable>& get_etch_table_list() { return _etch_table_list; }
  std::vector<ProcessEtchTable>& get_thickness_change_table_list() { return _thickness_change_table_list; }
  const std::vector<ProcessEtchTable>& get_etch_table_list() const { return _etch_table_list; }
  const std::vector<ProcessEtchTable>& get_thickness_change_table_list() const { return _thickness_change_table_list; }
  // setter
  void set_layer_name(const std::string& layer_name) { _layer_name = layer_name; }
  void set_thickness(Micron thickness) { _thickness = thickness; }
  void set_sheet_resistance(F64 sheet_resistance) { _sheet_resistance = sheet_resistance; }
  void set_resistivity(F64 resistivity) { _resistivity = resistivity; }
  void set_nominal_temperature(F64 nominal_temperature)
  {
    _nominal_temperature = nominal_temperature;
    _has_nominal_temperature = true;
  }
  void set_temperature_coefficient1(F64 temperature_coefficient1) { _temperature_coefficient1 = temperature_coefficient1; }
  void set_temperature_coefficient2(F64 temperature_coefficient2) { _temperature_coefficient2 = temperature_coefficient2; }
  void set_etch(Micron etch) { _etch = etch; }
  void set_resistive_only_etch(Micron resistive_only_etch) { _resistive_only_etch = resistive_only_etch; }
  void set_capacitive_only_etch(Micron capacitive_only_etch) { _capacitive_only_etch = capacitive_only_etch; }
  // function
  std::optional<F64> query_sheet_resistance(Micron width, Micron lower_spacing, Micron upper_spacing) const;
  std::optional<F64> query_resistivity(Micron width, Micron thickness, Micron lower_spacing, Micron upper_spacing) const;
  void query_temperature_coefficient(Micron width, F64& temperature_coefficient1, F64& temperature_coefficient2) const;
  Micron query_etch(ProcessEffectType effect_type, Micron width, Micron spacing) const;
  Micron query_thickness_change(ProcessEffectType effect_type, Micron width, Micron spacing) const;

 private:
  bool get_effect_is_applied(ProcessEffectType table_effect_type, ProcessEffectType query_effect_type) const;

  std::string _layer_name;
  Micron _thickness = 0.0;
  F64 _sheet_resistance = 0.0;
  F64 _resistivity = 0.0;
  bool _has_nominal_temperature = false;
  F64 _nominal_temperature = 25.0;
  F64 _temperature_coefficient1 = 0.0;
  F64 _temperature_coefficient2 = 0.0;
  Micron _etch = 0.0;
  Micron _resistive_only_etch = 0.0;
  Micron _capacitive_only_etch = 0.0;
  ProcessTable1D _sheet_resistance_by_width_table;
  ProcessTable1D _temperature_coefficient1_by_width_table;
  ProcessTable1D _temperature_coefficient2_by_width_table;
  ProcessTable2D _sheet_resistance_by_width_spacing_table;
  ProcessTable2D _resistivity_by_width_thickness_table;
  ProcessTable2D _resistivity_by_width_spacing_table;
  std::vector<ProcessEtchTable> _etch_table_list;
  std::vector<ProcessEtchTable> _thickness_change_table_list;
};

inline std::optional<F64> ProcessConductor::query_sheet_resistance(Micron width, Micron lower_spacing, Micron upper_spacing) const
{
  Micron spacing = std::min(lower_spacing, upper_spacing);
  std::optional<F64> sheet_resistance = _sheet_resistance_by_width_spacing_table.query(width, spacing);
  if (sheet_resistance.has_value()) {
    return sheet_resistance;
  }
  sheet_resistance = _sheet_resistance_by_width_table.query(width);
  if (sheet_resistance.has_value()) {
    return sheet_resistance;
  }
  if (_sheet_resistance > 0.0) {
    return _sheet_resistance;
  }
  return std::nullopt;
}

inline std::optional<F64> ProcessConductor::query_resistivity(Micron width, Micron thickness, Micron lower_spacing, Micron upper_spacing) const
{
  std::optional<F64> resistivity = _resistivity_by_width_thickness_table.query(thickness, width);
  if (resistivity.has_value()) {
    return resistivity;
  }
  Micron spacing = std::min(lower_spacing, upper_spacing);
  resistivity = _resistivity_by_width_spacing_table.query(width, spacing);
  if (resistivity.has_value()) {
    return resistivity;
  }
  if (_resistivity > 0.0) {
    return _resistivity;
  }
  return std::nullopt;
}

inline void ProcessConductor::query_temperature_coefficient(Micron width,
                                                             F64& temperature_coefficient1,
                                                             F64& temperature_coefficient2) const
{
  temperature_coefficient1 = _temperature_coefficient1;
  temperature_coefficient2 = _temperature_coefficient2;
  std::optional<F64> coefficient1 = _temperature_coefficient1_by_width_table.query(width);
  std::optional<F64> coefficient2 = _temperature_coefficient2_by_width_table.query(width);
  if (coefficient1.has_value()) {
    temperature_coefficient1 = coefficient1.value();
  }
  if (coefficient2.has_value()) {
    temperature_coefficient2 = coefficient2.value();
  }
}

inline Micron ProcessConductor::query_etch(ProcessEffectType effect_type, Micron width, Micron spacing) const
{
  Micron etch = _etch;
  if (effect_type == ProcessEffectType::kResistance) {
    etch += _resistive_only_etch;
  } else if (effect_type == ProcessEffectType::kCapacitance) {
    etch += _capacitive_only_etch;
  }
  for (const ProcessEtchTable& etch_table : _etch_table_list) {
    if (!get_effect_is_applied(etch_table.get_effect_type(), effect_type)) {
      continue;
    }
    std::optional<F64> table_etch = etch_table.get_table().query(width, spacing);
    if (table_etch.has_value()) {
      etch += table_etch.value();
    }
  }
  return etch;
}

inline Micron ProcessConductor::query_thickness_change(ProcessEffectType effect_type, Micron width, Micron spacing) const
{
  Micron thickness_change = 0.0;
  for (const ProcessEtchTable& thickness_change_table : _thickness_change_table_list) {
    if (!get_effect_is_applied(thickness_change_table.get_effect_type(), effect_type)) {
      continue;
    }
    std::optional<F64> table_thickness_change = thickness_change_table.get_table().query(width, spacing);
    if (table_thickness_change.has_value()) {
      thickness_change += table_thickness_change.value();
    }
  }
  return thickness_change;
}

inline bool ProcessConductor::get_effect_is_applied(ProcessEffectType table_effect_type, ProcessEffectType query_effect_type) const
{
  return table_effect_type == ProcessEffectType::kBoth || table_effect_type == query_effect_type;
}

}  // namespace ircx
