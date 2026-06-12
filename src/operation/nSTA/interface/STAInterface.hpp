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

namespace nsta {

#define STAI (nsta::STAInterface::getInst())

class STAInterface
{
 public:
  static STAInterface& getInst();
  static void destroyInst();

#if 1  // 外部调用STA的API

#if 1  // nSTA
  void initSTA();
  void runSTA();
  void destroySTA();
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
};

}  // namespace nsta
