// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#include "LVSInterface.hpp"
#include "tcl_ilvs.h"
#include "tcl_util.h"

namespace tcl {

namespace {

constexpr const char* kPath = "-path";

}  // namespace

TclWriteLVSDef::TclWriteLVSDef(const char* cmd_name) : TclCmd(cmd_name)
{
  addOption(new TclStringOption(kPath, 1, nullptr));
}

unsigned TclWriteLVSDef::check()
{
  TclOption* path_option = getOptionOrArg(kPath);
  if (path_option == nullptr || path_option->getStringVal() == nullptr) {
    std::cerr << "Please specify the iLVS DEF snapshot path by: write_lvs_def -path <file>." << std::endl;
    return 0;
  }
  return 1;
}

unsigned TclWriteLVSDef::exec()
{
  if (!check()) {
    return 0;
  }
  LVSI.writeLVSDef(getOptionOrArg(kPath)->getStringVal());
  return 1;
}

}  // namespace tcl
