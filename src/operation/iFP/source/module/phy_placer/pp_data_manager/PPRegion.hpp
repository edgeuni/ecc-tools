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
// THIS SOFTWARE IS PROVIDED ON AN \"AS IS\" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "FPHeader.hpp"

namespace ifp {

class PPRegion
{
 public:
  PPRegion() = default;
  ~PPRegion() = default;
  // getter
  int32_t get_row_idx() const { return _row_idx; }
  int32_t get_start_coord() const { return _start_coord; }
  int32_t get_end_coord() const { return _end_coord; }
  int32_t get_y_coord() const { return _y_coord; }
  std::string& get_orient_name() { return _orient_name; }
  // const getter
  const std::string& get_orient_name() const { return _orient_name; }

  // setter
  void set_row_idx(int32_t row_idx) { _row_idx = row_idx; }
  void set_start_coord(int32_t start_coord) { _start_coord = start_coord; }
  void set_end_coord(int32_t end_coord) { _end_coord = end_coord; }
  void set_y_coord(int32_t y_coord) { _y_coord = y_coord; }
  void set_orient_name(std::string orient_name) { _orient_name = orient_name; }
  // function

 private:
  int32_t _row_idx = -1;
  int32_t _start_coord = -1;
  int32_t _end_coord = -1;
  int32_t _y_coord = -1;
  std::string _orient_name;
};

}  // namespace ifp
