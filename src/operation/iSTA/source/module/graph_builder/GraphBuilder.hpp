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

#define STAGB (ista::GraphBuilder::getInst())

class GraphBuilder
{
 public:
  static void initInst();
  static GraphBuilder& getInst();
  static void destroyInst();
  // function
  bool build();

 private:
  // self
  static GraphBuilder* _gb_instance;

  GraphBuilder() = default;
  GraphBuilder(const GraphBuilder& other) = delete;
  GraphBuilder(GraphBuilder&& other) = delete;
  ~GraphBuilder() = default;
  GraphBuilder& operator=(const GraphBuilder& other) = delete;
  GraphBuilder& operator=(GraphBuilder&& other) = delete;
  // function
  void buildTimingPointList(Database& database);
  void buildCellArcs(Database& database);
  std::vector<std::string> collectInputPins(Database& database, Instance& instance);
  bool isInputLike(PinDirection direction);
  std::vector<std::string> collectOutputPins(Database& database, Instance& instance);
  bool isOutputLike(PinDirection direction);
  void buildNetArcs(Database& database);
  void addArc(Database& database, const std::string& source_pin, const std::string& sink_pin, ArcType type,
              const std::string& owner_name);
  void buildStartEndPointList(Database& database);
  bool isStartPoint(Database& database, const std::string& pin_name, Pin& pin);
  bool hasIncomingArc(Database& database, const std::string& pin_name);
  bool isStartPort(Pin& pin);
  bool isEndPoint(Database& database, const std::string& pin_name, Pin& pin);
  bool hasOutgoingArc(Database& database, const std::string& pin_name);
  bool isEndPort(Pin& pin);
  void appendUnique(std::vector<std::string>& list, const std::string& value);
};

}  // namespace ista
