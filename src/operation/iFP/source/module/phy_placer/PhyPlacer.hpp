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

namespace ifp {

#define FPP (ifp::PhyPlacer::getInst())

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
};

}  // namespace ifp
