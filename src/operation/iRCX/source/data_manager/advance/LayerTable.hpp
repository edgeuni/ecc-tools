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
#pragma once

#include "RCXHeader.hpp"

namespace ircx {

class LayerTable
{
 public:
  LayerTable() = default;
  ~LayerTable() = default;
  // getter
  std::unordered_map<int32_t, std::string>& get_design_id_to_name_map() { return _design_id_to_name_map; }
  std::unordered_map<std::string, int32_t>& get_design_name_to_id_map() { return _design_name_to_id_map; }
  std::unordered_map<int32_t, std::string>& get_process_id_to_name_map() { return _process_id_to_name_map; }
  std::unordered_map<std::string, int32_t>& get_process_name_to_id_map() { return _process_name_to_id_map; }
  std::unordered_map<std::string, std::string>& get_design_name_to_process_name_map() { return _design_name_to_process_name_map; }
  std::unordered_map<std::string, std::string>& get_process_name_to_design_name_map() { return _process_name_to_design_name_map; }
  // setter
  void set_design_id_to_name_map(const std::unordered_map<int32_t, std::string>& design_id_to_name_map)
  {
    _design_id_to_name_map = design_id_to_name_map;
  }
  void set_design_name_to_id_map(const std::unordered_map<std::string, int32_t>& design_name_to_id_map)
  {
    _design_name_to_id_map = design_name_to_id_map;
  }
  void set_process_id_to_name_map(const std::unordered_map<int32_t, std::string>& process_id_to_name_map)
  {
    _process_id_to_name_map = process_id_to_name_map;
  }
  void set_process_name_to_id_map(const std::unordered_map<std::string, int32_t>& process_name_to_id_map)
  {
    _process_name_to_id_map = process_name_to_id_map;
  }
  void set_design_name_to_process_name_map(const std::unordered_map<std::string, std::string>& design_name_to_process_name_map)
  {
    _design_name_to_process_name_map = design_name_to_process_name_map;
  }
  void set_process_name_to_design_name_map(const std::unordered_map<std::string, std::string>& process_name_to_design_name_map)
  {
    _process_name_to_design_name_map = process_name_to_design_name_map;
  }
  // function
  void register_design_layer(int32_t design_id, const std::string& design_name)
  {
    _design_id_to_name_map[design_id] = design_name;
    _design_name_to_id_map[design_name] = design_id;
  }
  void register_process_layer(int32_t process_id, const std::string& process_name)
  {
    _process_id_to_name_map[process_id] = process_name;
    _process_name_to_id_map[process_name] = process_id;
  }
  void register_mapping(const std::string& design_name, const std::string& process_name)
  {
    _design_name_to_process_name_map[design_name] = process_name;
    _process_name_to_design_name_map[process_name] = design_name;
  }
  int32_t get_design_id(const std::string& design_name) const { return _design_name_to_id_map.at(design_name); }
  std::string& get_design_name(int32_t design_id) { return _design_id_to_name_map.at(design_id); }
  int32_t get_process_id(const std::string& process_name) const { return _process_name_to_id_map.at(process_name); }
  std::string& get_process_name(int32_t process_id) { return _process_id_to_name_map.at(process_id); }
  int32_t get_process_id_by_design_id(int32_t design_id) const
  {
    return _process_name_to_id_map.at(_design_name_to_process_name_map.at(_design_id_to_name_map.at(design_id)));
  }
  int32_t get_design_id_by_process_id(int32_t process_id) const
  {
    return _design_name_to_id_map.at(_process_name_to_design_name_map.at(_process_id_to_name_map.at(process_id)));
  }

 private:
  std::unordered_map<int32_t, std::string> _design_id_to_name_map;
  std::unordered_map<std::string, int32_t> _design_name_to_id_map;
  std::unordered_map<int32_t, std::string> _process_id_to_name_map;
  std::unordered_map<std::string, int32_t> _process_name_to_id_map;
  std::unordered_map<std::string, std::string> _design_name_to_process_name_map;
  std::unordered_map<std::string, std::string> _process_name_to_design_name_map;
};

}  // namespace ircx
