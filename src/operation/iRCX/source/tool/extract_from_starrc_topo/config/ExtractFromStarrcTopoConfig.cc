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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
/**
 * @file ExtractFromStarrcTopoConfig.cc
 * @brief extract_from_starrc_topo implementation detail.
 */
#include "config/ExtractFromStarrcTopoConfig.hh"

#include "PathUtils.hh"

namespace ircx::extract_from_starrc_topo {

auto ConfigValidator::validate(const Config& config) const -> bool
{
  return path::fileExists(config.spef_file, "extract_from_starrc_topo SPEF file");
}

}  // namespace ircx::extract_from_starrc_topo
