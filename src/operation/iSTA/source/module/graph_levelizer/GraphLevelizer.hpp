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
// WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "Database.hpp"

namespace ista {

#define STAGL (ista::GraphLevelizer::getInst())

class GraphLevelizer
{
 public:
  static void initInst();
  static GraphLevelizer& getInst();
  static void destroyInst();
  // function
  bool build();

 private:
  // self
  static GraphLevelizer* _gl_instance;

  GraphLevelizer() = default;
  GraphLevelizer(const GraphLevelizer& other) = delete;
  GraphLevelizer(GraphLevelizer&& other) = delete;
  ~GraphLevelizer() = default;
  GraphLevelizer& operator=(const GraphLevelizer& other) = delete;
  GraphLevelizer& operator=(GraphLevelizer&& other) = delete;
  // function
  void buildTimingOrder(Database& database);
  std::map<std::string, std::size_t> makeIndegreeMap(Database& database);
  void pushRootPinList(Database& database, std::map<std::string, std::size_t>& indegree_map, std::queue<std::string>& pin_queue);
  void updateSinkLevel(Database& database, Arc& arc);
  void updateSinkIndegree(Arc& arc, std::map<std::string, std::size_t>& indegree_map, std::queue<std::string>& pin_queue);
  void printLoopInfo(Database& database);
};

}  // namespace ista
