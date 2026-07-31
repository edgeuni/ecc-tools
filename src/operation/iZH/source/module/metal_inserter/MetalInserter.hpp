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

#include "Logger.hpp"
#include "MIModel.hpp"
#include "Monitor.hpp"

namespace izh {

#define ZHMI (izh::MetalInserter::getInst())

class MetalInserter
{
 public:
  static void initInst();
  static MetalInserter& getInst();
  static void destroyInst();
  // function
  void insert(std::map<std::string, std::any> config_map);

 private:
  // self
  static MetalInserter* _mi_instance;

  MetalInserter() = default;
  MetalInserter(const MetalInserter& other) = delete;
  MetalInserter(MetalInserter&& other) = delete;
  ~MetalInserter() = default;
  MetalInserter& operator=(const MetalInserter& other) = delete;
  MetalInserter& operator=(MetalInserter&& other) = delete;
  // function
  MIModel initMIModel(std::map<std::string, std::any>& config_map);
};

}  // namespace izh
