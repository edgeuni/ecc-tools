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
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "ZHHeader.hpp"

namespace izh {

class MIModel
{
 public:
  MIModel() = default;
  ~MIModel() = default;
  // getter
  int32_t get_inserted_metal_num() const { return _inserted_metal_num; }
  // setter
  void set_inserted_metal_num(int32_t inserted_metal_num) { _inserted_metal_num = inserted_metal_num; }
  // function
  void addInsertedMetalNum() { ++_inserted_metal_num; }

 private:
  int32_t _inserted_metal_num = 0;
};

}  // namespace izh
