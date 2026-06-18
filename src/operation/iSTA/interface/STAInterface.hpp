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

#include <any>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#if 1  // 前向声明

namespace idb {
class IdbDesign;
class IdbInstance;
class IdbNet;
class IdbPin;
class IdbPins;
class IdbSpecialNet;
enum class IdbConnectType : uint8_t;
enum class IdbConnectDirection : uint8_t;
}  // namespace idb

#endif

namespace ista {

#define STAI (ista::STAInterface::getInst())

class TimingModel;
enum class PinDirection;
struct Net;
struct Pin;

class STAInterface
{
 public:
  static STAInterface& getInst();
  static void destroyInst();

#if 1  // 外部调用STA的API

#if 1  // iSTA
  void initSTA();
  void runSTA();
  void destroySTA();
#endif

#endif

#if 1  // STA调用外部的API

#if 1  // TopData

#if 1  // input
  bool inputIDB(idb::IdbDesign* idb_design);
  void inputInstances(idb::IdbDesign* idb_design, TimingModel& timing_model);
  void addInstancePin(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin, TimingModel& timing_model);
  std::string makeInstancePinName(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin) const;
  PinDirection convertDirection(const idb::IdbConnectDirection& idb_direction) const;
  void appendUnique(std::vector<std::string>& list, const std::string& value);
  void inputPorts(idb::IdbDesign* idb_design, TimingModel& timing_model);
  void addPortPin(idb::IdbPin* idb_pin, TimingModel& timing_model);
  std::string makePinName(idb::IdbPin* idb_pin) const;
  void inputNets(idb::IdbDesign* idb_design, TimingModel& timing_model);
  void inputNet(idb::IdbNet* idb_net, TimingModel& timing_model);
  bool isSignalNet(idb::IdbConnectType connect_type);
  void collectNetPins(idb::IdbNet* idb_net, const std::string& net_name, TimingModel& timing_model, Net& net);
  void collectNetPins(idb::IdbPins* io_pin_list, idb::IdbPins* instance_pin_list, const std::string& net_name, TimingModel& timing_model,
                      Net& net);
  void collectNetPin(idb::IdbPin* idb_pin, const std::string& net_name, TimingModel& timing_model, Net& net);
  bool shouldDriveNet(const Pin& pin);
  bool isOutputLike(PinDirection direction);
  bool shouldLoadNet(const Pin& pin);
  bool isInputLike(PinDirection direction);
  void inputSpecialNet(idb::IdbSpecialNet* idb_net, TimingModel& timing_model);
  void collectNetPins(idb::IdbSpecialNet* idb_net, const std::string& net_name, TimingModel& timing_model, Net& net);
#endif

#if 1  // output
#endif

#endif

#endif

 private:
  static STAInterface* _sta_interface_instance;

  std::map<std::string, std::any> config_map;

  STAInterface() = default;
  STAInterface(const STAInterface& other) = delete;
  STAInterface(STAInterface&& other) = delete;
  ~STAInterface() = default;
  STAInterface& operator=(const STAInterface& other) = delete;
  STAInterface& operator=(STAInterface&& other) = delete;
  // function
};

}  // namespace ista
