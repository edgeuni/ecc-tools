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
  // std::string temp_directory_path;  // required
  _config_list.push_back(std::make_pair("-temp_directory_path", ValueType::kString));
  // int32_t thread_number;            // optional
  _config_list.push_back(std::make_pair("-thread_number", ValueType::kInt));

#if 1  // DieBuilder

  _config_list.push_back(std::make_pair("-core_util", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-cell_area", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-x_margin", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-y_margin", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-xy_ratio", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-die_area", ValueType::kDoubleList));
  _config_list.push_back(std::make_pair("-core_area", ValueType::kDoubleList));
  _config_list.push_back(std::make_pair("-core_site", ValueType::kString));
  _config_list.push_back(std::make_pair("-io_site", ValueType::kString));
  _config_list.push_back(std::make_pair("-corner_site", ValueType::kString));
  _config_list.push_back(std::make_pair("-track_list", ValueType::kStringListList));

#endif

#if 1  // IOPlacer

  _config_list.push_back(std::make_pair("-io_pin_layer", ValueType::kString));
  _config_list.push_back(std::make_pair("-io_pin_width", ValueType::kInt));
  _config_list.push_back(std::make_pair("-io_pin_height", ValueType::kInt));
  _config_list.push_back(std::make_pair("-io_pin_side_list", ValueType::kStringList));
  _config_list.push_back(std::make_pair("-io_port_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-io_pad_master_list", ValueType::kStringList));
  _config_list.push_back(std::make_pair("-io_corner_master_list", ValueType::kStringList));
  _config_list.push_back(std::make_pair("-io_filler_name_list", ValueType::kStringList));
  _config_list.push_back(std::make_pair("-io_filler_prefix", ValueType::kString));

#endif

#if 1  // MacroPlacer

  _config_list.push_back(std::make_pair("-macro_halo_micron", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_dead_space_ratio", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_weight_wl", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_weight_ol", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_weight_ob", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_weight_periphery", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_weight_blockage", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_weight_io", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_max_iters", ValueType::kInt));
  _config_list.push_back(std::make_pair("-macro_cool_rate", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-macro_init_temperature", ValueType::kDouble));
  _config_list.push_back(std::make_pair("-placement_blockage_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-placement_halo_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-routing_blockage_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-routing_halo_list", ValueType::kStringListList));

#endif

#if 1  // PDNGenerator

  _config_list.push_back(std::make_pair("-pdn_io_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_global_connect_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_port_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_grid_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_stripe_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_layer_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_macro_connect_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_io_pin_connect_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_stripe_connect_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_segment_stripe_list", ValueType::kStringListList));
  _config_list.push_back(std::make_pair("-pdn_segment_via_list", ValueType::kStringListList));

#endif

#if 1  // PhyPlacer

  _config_list.push_back(std::make_pair("-tapcell_name", ValueType::kString));
  _config_list.push_back(std::make_pair("-endcap_name", ValueType::kString));
  _config_list.push_back(std::make_pair("-tap_distance", ValueType::kDouble));

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
