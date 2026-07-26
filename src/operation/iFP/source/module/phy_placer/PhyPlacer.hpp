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
#include "PPModel.hpp"
#include "Row.hpp"

namespace ifp {

#define FPPP (ifp::PhyPlacer::getInst())

class PhyPlacer
{
 public:
  static void initInst();
  static PhyPlacer& getInst();
  static void destroyInst();
  // function
  void place();

 private:
  // self
  static PhyPlacer* _pp_instance;

  PhyPlacer() = default;
  PhyPlacer(const PhyPlacer& other) = delete;
  PhyPlacer(PhyPlacer&& other) = delete;
  ~PhyPlacer() = default;
  PhyPlacer& operator=(const PhyPlacer& other) = delete;
  PhyPlacer& operator=(PhyPlacer&& other) = delete;
  // function

#if 1  // place phy cell
  void placePhyCell(PPModel& pp_model);
  void adjustTapDistance(int32_t& inst_space);
  int32_t buildPPRegionList(PPModel& pp_model);
  void buildPPRegionInRow(PPModel& pp_model, Row& row, int32_t row_idx);
  int32_t insertPhyCell(PPModel& pp_model, int32_t inst_space, std::string tapcell_name, std::string endcap_name);
  void addPhyCell(std::string instance_name, std::string cell_master_name, int32_t x_coord, int32_t y_coord, PlacementOrientation orient);
  int32_t getCellMasterWidthByOrient(CellMaster& cell_master, PlacementOrientation orient);
#endif
};

}  // namespace ifp
