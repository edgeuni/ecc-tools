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

#include "Database.hpp"

namespace spef {
struct ConnEntry;
struct Net;
struct ResCap;
}  // namespace spef

namespace idb {
class LibArc;
class LibArcSet;
class LibCell;
class LibLibrary;
class LibPort;
}  // namespace idb

namespace ista {

#define STADL (ista::DesignLoader::getInst())

class DesignLoader
{
 public:
  static void initInst();
  static DesignLoader& getInst();
  static void destroyInst();
  // function
  bool build();

 private:
  // self
  static DesignLoader* _dl_instance;

  DesignLoader() = default;
  DesignLoader(const DesignLoader& other) = delete;
  DesignLoader(DesignLoader&& other) = delete;
  ~DesignLoader() = default;
  DesignLoader& operator=(const DesignLoader& other) = delete;
  DesignLoader& operator=(DesignLoader&& other) = delete;
  // function
  void buildDesign(Database& database);
  void buildTimingLibrary(Database& database);
  void buildTimingCellMap(Database& database, std::vector<std::unique_ptr<idb::LibLibrary>>& lib_list);
  void makeTimingCell(Database& database, idb::LibCell* lib_cell);
  void makeTimingCellPort(TimingCell& timing_cell, idb::LibPort* lib_port);
  void makeTimingCellArc(TimingCell& timing_cell, idb::LibArcSet* lib_arc_set);
  TimingCellArc makeDelayArc(idb::LibArc* lib_arc);
  TimingCheckArc makeSetupArc(idb::LibArc* lib_arc);
  void updateTimingCell(TimingCell& timing_cell);
  void buildInstanceList(Database& database);
  void makeInstanceList(Database& database);
  void buildInstanceTimingInfo(Database& database);
  void makeInstanceTimingInfo(Database& database, Instance& instance);
  TimingCellArc* findClockToQArc(TimingCell& timing_cell);
  std::string getInstancePinName(Instance& instance, std::string& port_name);
  std::string findOutputPinName(Instance& instance, TimingCell& timing_cell);
  bool isInstancePin(Pin& pin);
  void makeUniqueName(std::vector<std::string>& list, const std::string& value);
  void buildNetList(Database& database);
  void makeNetList(Database& database);
  void makeNet(Database& database, const std::string& net_name, Net& net);
  bool isDriverPin(Pin& pin);
  bool isOutputLikeDirection(PinDirection direction);
  void buildParasiticLibrary(Database& database);
  void buildParasiticNetMap(Database& database, spef::Net& spef_net);
  void makeParasiticConnection(ParasiticNet& parasitic_net, spef::ConnEntry& spef_conn);
  void makeParasiticCapacitance(ParasiticNet& parasitic_net, spef::ResCap& spef_cap);
  void makeParasiticResistance(ParasiticNet& parasitic_net, spef::ResCap& spef_res);
  ParasiticNode& getParasiticNode(ParasiticNet& parasitic_net, const std::string& node_name);
  void readSdc(Database& database);
  std::vector<std::vector<std::string>> readCommandList(std::string& sdc_file_path);
  std::vector<std::string> tokenizeSdc(std::string& content);
  std::string removeComment(std::string& line);
  void parseCommand(Database& database, std::vector<std::string>& token_list);
  void parseCreateClock(Database& database, std::vector<std::string>& token_list);
  void parseSetInputDelay(Database& database, std::vector<std::string>& token_list);
  void parseSetOutputDelay(Database& database, std::vector<std::string>& token_list);
  void parseSetInputTransition(Database& database, std::vector<std::string>& token_list);
  void parseSetLoad(Database& database, std::vector<std::string>& token_list);
  double getCommandDoubleValue(std::vector<std::string>& token_list);
  std::string getOptionValue(std::vector<std::string>& token_list, const std::string& option);
  double getOptionDoubleValue(std::vector<std::string>& token_list, const std::string& option, double default_value);
  bool hasOption(std::vector<std::string>& token_list, const std::string& option);
  std::string getClockName(std::vector<std::string>& token_list);
  std::string getCollectionName(std::vector<std::string>& token_list, std::size_t collection_idx);
  std::vector<std::string> getObjectList(std::vector<std::string>& token_list);
  void pushObjectName(std::vector<std::string>& object_list, std::string object_name);
  bool isCollectionCommand(std::string& token);
  bool isClockCollectionCommand(std::string& token);
  bool isCommandOptionValue(std::vector<std::string>& token_list, std::size_t token_idx);
  std::vector<std::string> resolveObjectList(Database& database, std::vector<std::string>& object_list);
  void updateClock(Database& database, TimingClock& timing_clock);
  TimingPortConstraint& getPortConstraint(Database& database, const std::string& port_name);
};

}  // namespace ista
