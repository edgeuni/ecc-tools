// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#pragma once

#include "LVSDatabase.hpp"

namespace idb {
class IdbDesign;
class IdbPin;
}  // namespace idb

namespace ilvs {

class NetlistExtractor
{
 public:
  static LVSNetlist extract(idb::IdbDesign* design);

 private:
  static std::string getTerminalName(idb::IdbPin* pin);
};

}  // namespace ilvs
