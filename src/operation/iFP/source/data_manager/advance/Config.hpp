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
#include "PGGlobalConnect.hpp"
#include "PGGrid.hpp"
#include "PGIOPin.hpp"
#include "PGLayerPair.hpp"
#include "PGStripe.hpp"

namespace ifp {

class Config
{
 public:
  Config() = default;
  ~Config() = default;

#if 1  // FP

  std::string temp_directory_path;
  int32_t thread_number = -1;
  std::string log_file_path;

#endif

#if 1  // DieBuilder

  std::string layout_site_name;
  double layout_xy_ratio = -1.0;
  double layout_core_util = -1.0;
  double layout_margin_left_micron = -1.0;
  double layout_margin_right_micron = -1.0;
  double layout_margin_top_micron = -1.0;
  double layout_margin_bottom_micron = -1.0;

#endif

#if 1  // IOPlacer

  std::vector<std::string> io_pin_layer_name_list;
  double io_pin_width_micron = -1.0;
  double io_pin_depth_micron = -1.0;

#endif

#if 1  // PDNGenerator

  std::vector<PGIOPin> pg_io_pin_list;
  std::vector<PGGlobalConnect> pg_global_connect_list;
  std::vector<PGGrid> pg_grid_list;
  std::vector<PGStripe> pg_stripe_list;
  std::vector<PGLayerPair> pg_layer_pair_list;

#endif

#if 1  // PhyPlacer

  std::string tapcell_name;
  double tap_distance_micron = -1.0;
  std::string endcap_name;

#endif

#if 1  // DataManager

  std::string dm_temp_directory_path;

#endif

#if 1  // DieBuilder

  std::string db_temp_directory_path;

#endif

#if 1  // IOPlacer

  std::string ip_temp_directory_path;

#endif

#if 1  // MacroPlacer

  std::string mp_temp_directory_path;

#endif

#if 1  // PDNGenerator

  std::string pg_temp_directory_path;

#endif

#if 1  // PhyPlacer

  std::string pp_temp_directory_path;

#endif
};

}  // namespace ifp
