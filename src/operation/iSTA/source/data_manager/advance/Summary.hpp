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

#include "STAHeader.hpp"

namespace ista {

class GBSummary
{
 public:
  GBSummary() = default;
  ~GBSummary() = default;
};

class GPSummary
{
 public:
  GPSummary() = default;
  ~GPSummary() = default;
};

class TASummary
{
 public:
  TASummary() = default;
  ~TASummary() = default;
};

class Summary
{
 public:
  Summary() = default;
  ~Summary() = default;
  GBSummary gb_summary;
  GPSummary gp_summary;
  TASummary ta_summary;
};

}  // namespace ista
