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

namespace iemir {

#define EMIRI (iemir::EMIRInterface::getInst())

class EMIRInterface
{
 public:
  static EMIRInterface& getInst();
  static void destroyInst();

#if 1  // 外部调用EMIR的API

#if 1  // iEMIR
  void initEMIR();
  void runEMIR();
  void destroyEMIR();
#endif

#endif

 private:
  static EMIRInterface* _emir_interface_instance;

  EMIRInterface() = default;
  EMIRInterface(const EMIRInterface& other) = delete;
  EMIRInterface(EMIRInterface&& other) = delete;
  ~EMIRInterface() = default;
  EMIRInterface& operator=(const EMIRInterface& other) = delete;
  EMIRInterface& operator=(EMIRInterface&& other) = delete;
  // function
};

}  // namespace iemir
