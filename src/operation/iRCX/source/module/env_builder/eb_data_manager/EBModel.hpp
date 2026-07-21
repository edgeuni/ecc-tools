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

#include "EnvPixel.hpp"
#include "EnvTrack.hpp"
#include "NetEnv.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class EBModel final
{
 public:
  EBModel() = default;
  ~EBModel() = default;

  // getter
  double get_bucket_size_um() const { return bucket_size_um_; }
  size_t get_cross_layer() const { return cross_layer_; }
  std::map<size_t, EnvPixel>& get_layer_to_pixel_prefer_dir() { return layer_to_pixel_prefer_dir_; }
  std::map<size_t, EnvPixel>& get_layer_to_pixel_nonprefer_dir() { return layer_to_pixel_nonprefer_dir_; }
  std::map<size_t, EnvTrack>& get_layer_to_track_prefer_dir() { return layer_to_track_prefer_dir_; }
  std::map<size_t, EnvTrack>& get_layer_to_track_nonprefer_dir() { return layer_to_track_nonprefer_dir_; }
  std::map<size_t, int32_t>& get_layer_to_search_track_num() { return layer_to_search_track_num_; }
  // setter
  // function

  EBModel(const EBModel&) = delete;
  EBModel(EBModel&&) = default;
  EBModel& operator=(const EBModel&) = delete;
  EBModel& operator=(EBModel&&) = default;

 private:
  double bucket_size_um_ = 5.0;
  double window_size_um_ = 5.0;

  size_t cross_layer_ = 3;

  std::map<size_t, EnvPixel> layer_to_pixel_prefer_dir_;
  std::map<size_t, EnvPixel> layer_to_pixel_nonprefer_dir_;
  std::map<size_t, EnvTrack> layer_to_track_prefer_dir_;
  std::map<size_t, EnvTrack> layer_to_track_nonprefer_dir_;
  std::map<size_t, int32_t> layer_to_search_track_num_;

};

} // namespace ircx
