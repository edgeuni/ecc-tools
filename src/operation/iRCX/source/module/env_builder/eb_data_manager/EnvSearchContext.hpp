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

#include "EnvOverlapWidenContext.hpp"
#include "RCXHeader.hpp"

namespace ircx {

class EnvSearchContext
{
 public:
  EnvSearchContext(int32_t coordinate, int32_t base_track_index, int32_t query_start_coordinate, int32_t query_end_coordinate, int32_t step,
                   const EnvOverlapWidenFunc& widen_func)
      : _coordinate(coordinate),
        _base_track_index(base_track_index),
        _query_start_coordinate(query_start_coordinate),
        _query_end_coordinate(query_end_coordinate),
        _step(step),
        _widen_func(widen_func)
  {
  }
  ~EnvSearchContext() = default;
  // getter
  int32_t get_coordinate() const { return _coordinate; }
  int32_t get_base_track_index() const { return _base_track_index; }
  int32_t get_query_start_coordinate() const { return _query_start_coordinate; }
  int32_t get_query_end_coordinate() const { return _query_end_coordinate; }
  int32_t get_step() const { return _step; }
  const EnvOverlapWidenFunc& get_widen_func() const { return _widen_func; }
  // setter
  // function

 private:
  int32_t _coordinate = 0;
  int32_t _base_track_index = 0;
  int32_t _query_start_coordinate = 0;
  int32_t _query_end_coordinate = 0;
  int32_t _step = 0;
  const EnvOverlapWidenFunc& _widen_func;
};

}  // namespace ircx
