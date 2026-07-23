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

TclWriteLVSNetlist::TclWriteLVSNetlist(const char* cmd_name) : TclCmd(cmd_name)
{
  addOption(new TclStringOption(kPath, 1, nullptr));
}

unsigned TclWriteLVSNetlist::check()
{
  TclOption* path_option = getOptionOrArg(kPath);
  if (path_option == nullptr || path_option->getStringVal() == nullptr) {
    std::cerr << "Please specify the iLVS netlist snapshot path by: write_lvs_netlist -path <file>." << std::endl;
    return 0;
  }
  return 1;
}

unsigned TclWriteLVSNetlist::exec()
{
  if (!check()) {
    return 0;
  }
  LVSI.writeLVSNetlist(getOptionOrArg(kPath)->getStringVal());
  return 1;
}

}  // namespace tcl
