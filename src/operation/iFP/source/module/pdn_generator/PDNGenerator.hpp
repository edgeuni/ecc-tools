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

#include "FPHeader.hpp"
#include "Instance.hpp"
#include "IOPin.hpp"
#include "PGModel.hpp"
#include "PGNet.hpp"
#include "PGSegment.hpp"
#include "PlanarRect.hpp"
#include "RoutingLayer.hpp"

namespace ifp {

#define FPPG (ifp::PDNGenerator::getInst())

class PDNGenerator
{
 public:
  static void initInst();
  static PDNGenerator& getInst();
  static void destroyInst();
  // function

#if 1  // generate

  void generate();
  void generatePDN(PGModel& pg_model);

#endif

#if 1  // build net

  void buildIOPin();
  PGNet& getPGNet(std::string net_name, PGNetType net_type);
  IOPin& getIOPin(std::string pin_name);
  void buildGlobalConnect();

#endif

#if 1  // build port

  void buildPDNPort();
  Instance* findInstance(std::string instance_name);

#endif

#if 1  // build grid

  void buildGrid();
  RoutingLayer* findRoutingLayer(std::string layer_name);
  int32_t transMicronToDBU(double value);
  void addLineSegmentWithBlockage(std::string net_name, std::string layer_name, PGSegmentType segment_type, int32_t width,
                                  int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y);
  void addLineSegment(std::string net_name, std::string layer_name, PGSegmentType segment_type, int32_t width, int32_t start_x,
                      int32_t start_y, int32_t end_x, int32_t end_y);

#endif

#if 1  // build stripe

  void buildStripe();

#endif

#if 1  // build via

  void buildLayerConnect(PGModel& pg_model);
  PlanarRect getOverlapRect(PlanarRect first_rect, PlanarRect second_rect);
  void addViaSegment(PGModel& pg_model, std::string net_name, std::string bottom_layer_name, std::string top_layer_name,
                     std::string cut_layer_name, int32_t x, int32_t y, int32_t width, int32_t height);

#endif

#if 1  // build macro connection

  void buildMacroConnect(PGModel& pg_model);
  std::vector<std::string> getNameList(std::string name_list);
  std::string getPGNetName(PGNetType net_type, std::vector<std::string>& pin_name_list);

#endif

#if 1  // build IO connection

  void buildIOPinConnect();
  std::vector<std::pair<int32_t, int32_t>> getCoordList(std::vector<std::string>& value_list, int32_t begin_idx);
  std::string getIOPinNameByCoord(int32_t x, int32_t y, std::string layer_name);
  std::string getPGNetNameByIOPin(std::string pin_name);
  void addPolyline(std::vector<std::pair<int32_t, int32_t>>& coord_list, std::string net_name, std::string layer_name, int32_t width);

#endif

#if 1  // build segment

  void buildStripeConnect();
  void buildSegmentStripe();
  void buildSegmentVia(PGModel& pg_model);

#endif

 private:
  // self
  static PDNGenerator* _pg_instance;

  PDNGenerator() = default;
  PDNGenerator(const PDNGenerator& other) = delete;
  PDNGenerator(PDNGenerator&& other) = delete;
  ~PDNGenerator() = default;
  PDNGenerator& operator=(const PDNGenerator& other) = delete;
  PDNGenerator& operator=(PDNGenerator&& other) = delete;
  // function
};

}  // namespace ifp
