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
#include "IOPadCoord.hpp"

namespace ifp {

class IOPModel
{
 public:
  IOPModel() = default;
  ~IOPModel() = default;
  // getter
  int32_t get_io_filler_idx() const { return _io_filler_idx; }
  std::vector<IOPadCoord>& get_io_pad_coord_list() { return _io_pad_coord_list; }

  // const getter
  const std::vector<IOPadCoord>& get_io_pad_coord_list() const { return _io_pad_coord_list; }

  // setter
  void set_io_filler_idx(int32_t io_filler_idx) { _io_filler_idx = io_filler_idx; }
  void set_io_pad_coord_list(const std::vector<IOPadCoord>& io_pad_coord_list) { _io_pad_coord_list = io_pad_coord_list; }

  // function

 private:
  int32_t _io_filler_idx = -1;
  std::vector<IOPadCoord> _io_pad_coord_list;
};

}  // namespace ifp
