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

namespace ista {
class Database;
enum class PinDirection;
class Net;
}  // namespace ista

#endif

namespace ista {

#define STAI (ista::STAInterface::getInst())


class STAInterface
{
 public:
  static STAInterface& getInst();
  static void destroyInst();

#if 1  // 外部调用STA的API

#if 1  // iSTA
  void initSTA(std::map<std::string, std::any> config_map);
  void runSTA();
  void destroySTA();
#endif

#endif

#if 1  // STA调用外部的API

#if 1  // TopData

#if 1  // input
  void input(std::map<std::string, std::any>& config_map);
  void wrapConfig(std::map<std::string, std::any>& config_map);
  void wrapDatabase(idb::IdbDesign* idb_design, Database& database);
  void wrapInstanceList(idb::IdbDesign* idb_design, Database& database);
  void wrapInstancePin(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin, Database& database);
  std::string wrapInstancePinName(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin) const;
  PinDirection wrapPinDirection(const idb::IdbConnectDirection& idb_direction) const;
  void wrapUniqueName(std::vector<std::string>& list, const std::string& value);
  void wrapPortList(idb::IdbDesign* idb_design, Database& database);
  void wrapPortPin(idb::IdbPin* idb_pin, Database& database);
  std::string wrapPinName(idb::IdbPin* idb_pin) const;
  void wrapNetList(idb::IdbDesign* idb_design, Database& database);
  void wrapNet(idb::IdbNet* idb_net, Database& database);
  bool wrapSignalNet(idb::IdbConnectType connect_type);
  void wrapNetPinList(idb::IdbNet* idb_net, Database& database, Net& net);
  void wrapNetPinList(idb::IdbPins* io_pin_list, idb::IdbPins* instance_pin_list, Database& database, Net& net);
  void wrapNetPin(idb::IdbPin* idb_pin, Database& database, Net& net);
  void wrapSpecialNet(idb::IdbSpecialNet* idb_net, Database& database);
  void wrapNetPinList(idb::IdbSpecialNet* idb_net, Database& database, Net& net);
#endif

#if 1  // output
  void output();
#endif

#endif

#endif

 private:
  static STAInterface* _sta_interface_instance;

  STAInterface() = default;
  STAInterface(const STAInterface& other) = delete;
  STAInterface(STAInterface&& other) = delete;
  ~STAInterface() = default;
  STAInterface& operator=(const STAInterface& other) = delete;
  STAInterface& operator=(STAInterface&& other) = delete;
  // function
};

}  // namespace ista
