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

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Net.hpp"
#include "RCXHeader.hpp"
#include "TBNetTopology.hpp"
#include "TBModel.hpp"

namespace ircx {

#define RCXTB (ircx::TopologyBuilder::getInst())

class TopologyBuilder
{
 public:
  static void initInst();
  static TopologyBuilder& getInst();
  static void destroyInst();
  // function
  void build();

 private:
  // self
  static TopologyBuilder* _tb_instance;

  TopologyBuilder() = default;
  TopologyBuilder(const TopologyBuilder& other) = delete;
  TopologyBuilder(TopologyBuilder&& other) = delete;
  ~TopologyBuilder() = default;
  TopologyBuilder& operator=(const TopologyBuilder& other) = delete;
  TopologyBuilder& operator=(TopologyBuilder&& other) = delete;
  // function
  TBModel initTBModel();
  void buildTBModel(TBModel& tb_model);
  void buildAll();
  TBNetTopology buildNet(Net& net);
  void buildSpecial();
};

}  // namespace ircx
