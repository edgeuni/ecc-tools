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

#include "RCXHeader.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class SPEFNameMap
{
 public:
  SPEFNameMap() = default;
  ~SPEFNameMap() = default;
  // getter
  std::unordered_map<std::string, size_t>& get_net_name_to_id_map() { return _net_name_to_id_map; }
  std::unordered_map<std::string, size_t>& get_port_name_to_id_map() { return _port_name_to_id_map; }
  std::unordered_map<std::string, size_t>& get_instance_name_to_id_map() { return _instance_name_to_id_map; }
  std::map<size_t, std::string>& get_id_to_net_name_map() { return _id_to_net_name_map; }
  std::map<size_t, std::string>& get_id_to_port_name_map() { return _id_to_port_name_map; }
  std::map<size_t, std::string>& get_id_to_instance_name_map() { return _id_to_instance_name_map; }
  size_t get_next_id() const { return _next_id; }
  // setter
  void set_next_id(size_t next_id) { _next_id = next_id; }
  // function

 private:
  std::unordered_map<std::string, size_t> _net_name_to_id_map;
  std::unordered_map<std::string, size_t> _port_name_to_id_map;
  std::unordered_map<std::string, size_t> _instance_name_to_id_map;
  std::map<size_t, std::string> _id_to_net_name_map;
  std::map<size_t, std::string> _id_to_port_name_map;
  std::map<size_t, std::string> _id_to_instance_name_map;
  size_t _next_id = 1;
};

}  // namespace ircx
