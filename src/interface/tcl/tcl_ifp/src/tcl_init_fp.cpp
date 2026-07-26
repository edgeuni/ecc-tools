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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "FPInterface.hpp"
#include "tcl_fp.h"
#include "tcl_util.h"

namespace tcl {

// public

TclInitFP::TclInitFP(const char* cmd_name) : TclCmd(cmd_name)
{
  _config_list.push_back(std::make_pair("-temp_directory_path", ValueType::kString));
  _config_list.push_back(std::make_pair("-thread_number", ValueType::kInt));

#if 1  // Floorplan

  // -layout {site_name xy_ratio core_util margin_left margin_right margin_top margin_bottom}
  _config_list.push_back(std::make_pair("-layout", ValueType::kStringList));

#endif

#if 1  // IO pin

  // -io_pin {{io_layer1 io_layer2 ...} io_pin_width_micron io_pin_depth_micron}
  _config_list.push_back(std::make_pair("-io_pin", ValueType::kStringListList));

#endif

#if 1  // PG connect

  // -pg_connect {{{pg_net_name connect_pin_name...} ...} is_power}
  _config_list.push_back(std::make_pair("-pg_connect", ValueType::kStringListListList));

#endif

#if 1  // PDN mesh

  // -pdn_mesh {{rail_layer rail_width_micron} {stripe_layer stripe_width_micron pitch_micron offset_micron} ...}
  _config_list.push_back(std::make_pair("-pdn_mesh", ValueType::kStringListList));

#endif

#if 1  // Phy cell

  // -phy_insert {{tapcell_name tap_distance_micron} endcap_name}
  _config_list.push_back(std::make_pair("-phy_insert", ValueType::kStringListList));

#endif

  TclUtil::addOption(this, _config_list);
}

unsigned TclInitFP::exec()
{
  if (!check()) {
    return 0;
  }
  std::map<std::string, std::any> config_map = TclUtil::getConfigMap(this, _config_list);
  FPI.initFP(config_map);
  return 1;
}

// private

}  // namespace tcl
