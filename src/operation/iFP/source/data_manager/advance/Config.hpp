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

#include "FPHeader.hpp"

namespace ifp {

class Config
{
 public:
  Config() = default;
  ~Config() = default;
  /////////////////////////////////////////////
  // **********        FP         ********** //
  std::string temp_directory_path;  // required
  int32_t thread_number;            // optional
  // **********    DieBuilder     ******** //
  double core_util = -1.0;          // optional
  double cell_area = -1.0;          // optional
  double x_margin = 10.0;           // optional
  double y_margin = 10.0;           // optional
  double xy_ratio = 1.0;            // optional
  std::vector<double> die_area;     // optional
  std::vector<double> core_area;    // optional
  std::string core_site;            // optional
  std::string io_site;              // optional
  std::string corner_site;          // optional
  std::vector<std::vector<std::string>> track_list;  // optional
  // **********     IOPlacer      ******** //
  std::string io_pin_layer;                   // optional
  int32_t io_pin_width = -1;                  // optional
  int32_t io_pin_height = -1;                 // optional
  std::vector<std::string> io_pin_side_list;  // optional
  std::vector<std::vector<std::string>> io_port_list;  // optional
  std::vector<std::string> io_pad_master_list;     // optional
  std::vector<std::string> io_corner_master_list;  // optional
  std::vector<std::string> io_filler_name_list;    // optional
  std::string io_filler_prefix;                     // optional
  // **********    MacroPlacer    ******** //
  double macro_halo_micron = -1.0;              // optional
  double macro_dead_space_ratio = -1.0;         // optional
  double macro_weight_wl = -1.0;                // optional
  double macro_weight_ol = -1.0;                // optional
  double macro_weight_ob = -1.0;                // optional
  double macro_weight_periphery = -1.0;         // optional
  double macro_weight_blockage = -1.0;          // optional
  double macro_weight_io = -1.0;                // optional
  int32_t macro_max_iters = -1;                 // optional
  double macro_cool_rate = -1.0;                // optional
  double macro_init_temperature = -1.0;         // optional
  std::vector<std::vector<std::string>> placement_blockage_list;  // optional
  std::vector<std::vector<std::string>> placement_halo_list;       // optional
  std::vector<std::vector<std::string>> routing_blockage_list;     // optional
  std::vector<std::vector<std::string>> routing_halo_list;         // optional
  // **********   PDNGenerator    ******** //
  std::vector<std::vector<std::string>> pdn_io_list;                 // optional
  std::vector<std::vector<std::string>> pdn_global_connect_list;     // optional
  std::vector<std::vector<std::string>> pdn_port_list;               // optional
  std::vector<std::vector<std::string>> pdn_grid_list;               // optional
  std::vector<std::vector<std::string>> pdn_stripe_list;             // optional
  std::vector<std::vector<std::string>> pdn_layer_list;              // optional
  std::vector<std::vector<std::string>> pdn_macro_connect_list;      // optional
  std::vector<std::vector<std::string>> pdn_io_pin_connect_list;     // optional
  std::vector<std::vector<std::string>> pdn_stripe_connect_list;     // optional
  std::vector<std::vector<std::string>> pdn_segment_stripe_list;     // optional
  std::vector<std::vector<std::string>> pdn_segment_via_list;        // optional
  // **********    PhyPlacer     ******** //
  std::string tapcell_name;  // optional
  std::string endcap_name;   // optional
  double tap_distance = -1.0;  // optional
  /////////////////////////////////////////////
  // **********        FP         ********** //
  std::string log_file_path;  // building
  // **********    DataManager    ********** //
  std::string dm_temp_directory_path;  // building
  // **********    DieBuilder     ******** //
  std::string db_temp_directory_path;  // building
  // **********     IOPlacer      ******** //
  std::string iop_temp_directory_path;  // building
  // **********    MacroPlacer    ******** //
  std::string mp_temp_directory_path;  // building
  // **********   PDNGenerator    ******** //
  std::string pg_temp_directory_path;  // building
  // **********    PhyPlacer      ******** //
  std::string pp_temp_directory_path;  // building
  /////////////////////////////////////////////
};

}  // namespace ifp
