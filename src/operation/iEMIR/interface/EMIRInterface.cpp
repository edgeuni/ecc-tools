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
#include "EMIRInterface.hpp"

namespace iemir {

EMIRInterface* EMIRInterface::_emir_interface_instance = nullptr;

// public

EMIRInterface& EMIRInterface::getInst()
{
  if (_emir_interface_instance == nullptr) {
    _emir_interface_instance = new EMIRInterface();
  }
  return *_emir_interface_instance;
}

void EMIRInterface::destroyInst()
{
  if (_emir_interface_instance != nullptr) {
    delete _emir_interface_instance;
    _emir_interface_instance = nullptr;
  }
}

#if 1  // 外部调用EMIR的API

#if 1  // iEMIR

void EMIRInterface::initEMIR() {}

void EMIRInterface::runEMIR() {}

void EMIRInterface::destroyEMIR() {}

#endif

#endif

}  // namespace iemir
