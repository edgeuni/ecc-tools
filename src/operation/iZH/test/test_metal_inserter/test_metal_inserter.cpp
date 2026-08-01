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
#include "MIModel.hpp"
#include "MetalInserter.hpp"

int main()
{
  izh::MIModel mi_model;
  if (mi_model.get_inserted_metal_num() != 0) {
    return 1;
  }
  mi_model.set_inserted_metal_num(2);
  mi_model.addInsertedMetalNum();
  if (mi_model.get_inserted_metal_num() != 3) {
    return 1;
  }

  izh::MetalInserter::initInst();
  izh::MetalInserter* mi_instance = &ZHMI;
  izh::MetalInserter::initInst();
  if (mi_instance != &ZHMI) {
    return 1;
  }
  izh::MetalInserter::destroyInst();
  return 0;
}
