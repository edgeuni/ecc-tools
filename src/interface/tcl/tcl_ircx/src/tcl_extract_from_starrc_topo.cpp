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
 * @file tcl_extract_from_starrc_topo.cpp
 * @brief Tcl command for RC extraction using StarRC SPEF topology.
 */
#include <utility>

#include "RCXAPI.hh"
#include "config/ExtractFromStarrcTopoConfig.hh"
#include "log/Log.hh"
#include "tcl_ircx.h"

namespace tcl {
namespace {

constexpr const char* kSpefArg = "spef";

auto getStringValue(TclOption* option) -> const char*
{
  if (option == nullptr || !option->is_set_val()) {
    return nullptr;
  }
  return option->getStringVal();
}

auto isOptionSet(TclOption* option) -> bool
{
  return option != nullptr && option->is_set_val();
}

}  // namespace

TclExtractFromStarrcTopo::TclExtractFromStarrcTopo(const char* cmd_name) : TclCmd(cmd_name)
{
  addOption(new TclStringOption(kSpefArg, 1, nullptr));
  addOption(new TclSwitchOption("-non_strict"));
}

unsigned TclExtractFromStarrcTopo::check()
{
  if (getStringValue(getOptionOrArg(kSpefArg)) == nullptr) {
    LOG_ERROR << "extract_from_starrc_topo requires a SPEF argument.";
    return 0;
  }
  return 1;
}

unsigned TclExtractFromStarrcTopo::exec()
{
  if (!check()) {
    return 0;
  }

  ircx::extract_from_starrc_topo::Config config;
  config.spef_file = getStringValue(getOptionOrArg(kSpefArg));
  config.strict = !isOptionSet(getOptionOrArg("-non_strict"));

  return RCX_API_INST.extract_from_starrc_topo(std::move(config)) ? 1U : 0U;
}

}  // namespace tcl
