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
#include <string>

#if 1  // 前向声明

namespace idb {
class IdbSpecialWire;
enum class IdbConnectDirection : uint8_t;
enum class IdbOrient : uint8_t;
}

#endif

namespace ifp {

class PGSegment;
enum class IOPinDirection;
enum class PlacementOrientation;

#define FPI (ifp::FPInterface::getInst())

class FPInterface
{
 public:
  static FPInterface& getInst();
  static void destroyInst();

#if 1  // 外部调用FP的API

#if 1  // iFP
  void initFP(std::map<std::string, std::any> config_map);
  void runFP();
  void destroyFP();
#endif

#endif

#if 1  // FP调用外部的API

#if 1  // TopData

#if 1  // input
  void input(std::map<std::string, std::any>& config_map);
  void wrapConfig(std::map<std::string, std::any>& config_map);
  void wrapDatabase();
  void wrapDBInfo();
  void wrapMicronDBU();
  void wrapManufactureGrid();
  void wrapCellArea();
  void wrapSiteMap();
  void wrapCellMasterMap();
  void wrapRoutingLayerList();
  void wrapInstanceList();
  PlacementOrientation wrapPlacementOrientation(idb::IdbOrient idb_orient);
  void wrapNetList();
  void wrapIOPinList();
#endif

#if 1  // output
  void output();
  void outputFloorplan();
  void outputDie();
  void outputCore();
  void outputRowList();
  idb::IdbOrient unwrapPlacementOrientation(PlacementOrientation orient);
  void outputTrackList();
  void outputPGNetList();
  idb::IdbConnectDirection unwrapIOPinDirection(IOPinDirection io_pin_direction);
  void outputIOPinList();
  void outputIOInstancePlacement();
  void outputMacroPlacement();
  void outputNewInstanceList();
  void outputPGSegmentList();
  void outputPGVia(idb::IdbSpecialWire* idb_special_wire, PGSegment& pg_segment);
#endif

#endif

#endif

 private:
  static FPInterface* _fp_interface_instance;

  FPInterface() = default;
  FPInterface(const FPInterface& other) = delete;
  FPInterface(FPInterface&& other) = delete;
  ~FPInterface() = default;
  FPInterface& operator=(const FPInterface& other) = delete;
  FPInterface& operator=(FPInterface&& other) = delete;
  // function
};

}  // namespace ifp
