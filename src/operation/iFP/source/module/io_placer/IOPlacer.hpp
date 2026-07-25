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

#include "CellMaster.hpp"
#include "Core.hpp"
#include "IOEdgeType.hpp"
#include "IOInterval.hpp"
#include "IOPin.hpp"
#include "IOPModel.hpp"
#include "IOPadCoord.hpp"
#include "Instance.hpp"
#include "RoutingLayer.hpp"

namespace ifp {

#define FPIOP (ifp::IOPlacer::getInst())

class IOPlacer
{
 public:
  static void initInst();
  static IOPlacer& getInst();
  static void destroyInst();
  // function
  void place();

#if 1  // place IO pin
  void placeIOPin();
  void autoPlacePins(std::string layer_name, int32_t width, int32_t height, std::vector<std::string> side_list);
  bool hasSide(std::vector<std::string>& side_list, std::string side_name);
  std::string getRoutingLayerNameByIdx(int32_t layer_idx);
  int32_t getTrackPitch(std::string layer_name);
  void placeIOPinsOnEdge(IOEdgeType edge_type, std::vector<IOPin>& io_pin_list, int32_t& io_pin_idx, int32_t edge_pin_num,
                         std::string horizontal_layer_name, std::string vertical_layer_name, int32_t width, int32_t height,
                         int32_t horizontal_pitch, int32_t vertical_pitch, int32_t manufacture_grid);
  int32_t getAlongCoord(int32_t range_low, int32_t range_high, int32_t die_low, int32_t die_high, int32_t pin_span,
                        int32_t access_pitch, int32_t side_pin_num, int32_t pin_idx, int32_t manufacture_grid);
  int32_t alignDown(int32_t value, int32_t manufacture_grid);
  int32_t alignUp(int32_t value, int32_t manufacture_grid);
  int32_t alignNearest(int32_t value, int32_t manufacture_grid);
  void addIOPinPort(IOPin& io_pin, int32_t x, int32_t y, int32_t rect_width, int32_t rect_height, int32_t manufacture_grid,
                    std::string layer_name);
  void syncPinLocation(IOPin& io_pin, IOPort& io_port, int32_t x, int32_t y);
  void updateNetIOPin(IOPin& io_pin);
#endif

#if 1  // place IO port
  void placeIOPortList();
  void placePort(std::string pin_name, int32_t x_offset, int32_t y_offset, int32_t rect_width, int32_t rect_height,
                 std::string layer_name);
#endif

#if 1  // place IO pad
  void placeIOPad(IOPModel& iop_model);
  void autoPlacePad(IOPModel& iop_model, std::vector<std::string> pad_master_list, std::vector<std::string> corner_master_list);
  std::vector<int32_t> getIOPadIdxList(std::vector<std::string> pad_master_list);
  void setPadCoordList(IOPModel& iop_model, std::vector<std::string> corner_master_list);
  std::string getOrientByEdge(IOEdgeType edge_type);
  void placePad(std::vector<int32_t>& pad_idx_list, int32_t& pad_idx, IOPadCoord& pad_coord, int32_t step);
#endif

#if 1  // place IO filler
  void placeIOFiller(IOPModel& iop_model);
  void autoPlaceFiller(IOPModel& iop_model, std::vector<std::string> filler_name_list, std::string prefix);
  void placeFiller(IOPModel& iop_model, std::vector<std::string>& filler_name_list, std::string prefix, IOPadCoord& pad_coord);
  bool isSameEdgeAndOrient(IOEdgeType edge_type, std::string orient_name);
  void fillInterval(IOPModel& iop_model, IOInterval& interval, std::vector<std::string>& filler_name_list, std::string prefix,
                    IOPadCoord& pad_coord);
  std::string getFiller(int32_t length, std::vector<std::string>& filler_name_list);
  std::string buildFillerInstName(std::string prefix, IOPadCoord& pad_coord, int32_t filler_idx);
#endif

 private:
  // self
  static IOPlacer* _iop_instance;

  IOPlacer() = default;
  IOPlacer(const IOPlacer& other) = delete;
  IOPlacer(IOPlacer&& other) = delete;
  ~IOPlacer() = default;
  IOPlacer& operator=(const IOPlacer& other) = delete;
  IOPlacer& operator=(IOPlacer&& other) = delete;
  // function
};

}  // namespace ifp
