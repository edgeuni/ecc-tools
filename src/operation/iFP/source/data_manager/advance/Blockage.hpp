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

#include "PlanarRect.hpp"

namespace ifp {

class Blockage : public PlanarRect
{
 public:
  Blockage() = default;
  ~Blockage() = default;
  // getter
  std::vector<std::string>& get_layer_name_list() { return _layer_name_list; }
  bool get_except_pg_net() const { return _except_pg_net; }

  // const getter
  const std::vector<std::string>& get_layer_name_list() const { return _layer_name_list; }

  // setter
  void set_layer_name_list(std::vector<std::string> layer_name_list) { _layer_name_list = layer_name_list; }
  void set_except_pg_net(bool except_pg_net) { _except_pg_net = except_pg_net; }

  // function

 private:
  std::vector<std::string> _layer_name_list;
  bool _except_pg_net = false;
};

}  // namespace ifp
