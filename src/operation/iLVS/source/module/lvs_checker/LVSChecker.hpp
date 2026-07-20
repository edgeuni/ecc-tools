// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#pragma once

#include "LVSDatabase.hpp"

namespace ilvs {

class LVSChecker
{
 public:
  static LVSCheckResult check(const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist);
};

}  // namespace ilvs
