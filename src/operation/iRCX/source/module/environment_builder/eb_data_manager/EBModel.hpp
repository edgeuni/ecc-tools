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
/**
 * @file EBModel.hh
 * @brief iRCX module implementation detail.
 */
#pragma once

#include "EnvironmentPixel.hpp"
#include "EnvironmentTrack.hpp"
#include "LayoutData.hpp"
#include "NetEnvironment.hpp"
#include "RCXHeader.hpp"
#include "TopoPool.hpp"

namespace ircx {

class EBModel final
{
 public:
  EBModel() = default;
  ~EBModel() = default;

  // getter
  std::string& get_temp_directory_path() { return _temp_directory_path; }
  LayoutData* get_layout_data() { return layout_data_; }
  TopoPool* get_topo_pool() { return topo_pool_; }
  F32 get_bucket_size_um() const { return bucket_size_um_; }
  Size get_cross_layer() const { return cross_layer_; }
  std::map<Size, EnvironmentPixel>& get_layer_to_pixel_prefer_dir() { return layer_to_pixel_prefer_dir_; }
  std::map<Size, EnvironmentPixel>& get_layer_to_pixel_nonprefer_dir() { return layer_to_pixel_nonprefer_dir_; }
  std::map<Size, EnvironmentTrack>& get_layer_to_track_prefer_dir() { return layer_to_track_prefer_dir_; }
  std::map<Size, EnvironmentTrack>& get_layer_to_track_nonprefer_dir() { return layer_to_track_nonprefer_dir_; }
  std::map<Size, Dbu>& get_layer_to_search_track_num() { return layer_to_search_track_num_; }
  // setter
  void set_temp_directory_path(const std::string& temp_directory_path) { _temp_directory_path = temp_directory_path; }
  void set_layout_data(LayoutData* v) { layout_data_ = v; }
  void set_topo_pool(TopoPool* v) { topo_pool_ = v; }
  // function

  EBModel(const EBModel&) = delete;
  EBModel(EBModel&&) = default;
  auto operator=(const EBModel&) -> EBModel& = delete;
  auto operator=(EBModel&&) -> EBModel& = default;

 private:
  std::string _temp_directory_path;
  LayoutData* layout_data_{nullptr};
  TopoPool* topo_pool_{nullptr};

  F32 bucket_size_um_{5.0F};
  F32 window_size_um_{5.0F};

  Size cross_layer_{3};

  std::map<Size, EnvironmentPixel> layer_to_pixel_prefer_dir_;
  std::map<Size, EnvironmentPixel> layer_to_pixel_nonprefer_dir_;
  std::map<Size, EnvironmentTrack> layer_to_track_prefer_dir_;
  std::map<Size, EnvironmentTrack> layer_to_track_nonprefer_dir_;
  std::map<Size, Dbu> layer_to_search_track_num_;

};

} // namespace ircx
