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
#include "tcl_cts.h"

#include "CTSAPI.hh"

namespace tcl {

CmdCTSAutoRun::CmdCTSAutoRun(const char* cmd_name) : TclCmd(cmd_name)
{
  auto* file_name_option = new TclStringOption(TCL_CONFIG, 1, nullptr);
  addOption(file_name_option);
  auto* dir_name_option = new TclStringOption(TCL_WORK_DIR, 1, nullptr);
  addOption(dir_name_option);
}

unsigned CmdCTSAutoRun::check()
{
  TclOption* file_name_option = getOptionOrArg(TCL_CONFIG);
  LOG_FATAL_IF(!file_name_option);
  TclOption* dir_name_option = getOptionOrArg(TCL_WORK_DIR);
  LOG_FATAL_IF(!dir_name_option);
  return 1;
}

unsigned CmdCTSAutoRun::exec()
{
  if (!check()) {
    return 0;
  }

  TclOption* config_option = getOptionOrArg(TCL_CONFIG);
  auto config_path = config_option->getStringVal();

  TclOption* dir_option = getOptionOrArg(TCL_WORK_DIR);
  auto dir_path = dir_option->getStringVal();
  icts::CTSStatus cts_status;
  if (dir_path == nullptr) {
    cts_status = CTS_API_INST.runCTS(config_path);
  } else {
    cts_status = CTS_API_INST.runCTS(config_path, dir_path);
  }

  LOG_FATAL_IF(!cts_status.ok()) << "iCTS run failed: " << cts_status.message << std::endl;

  LOG_INFO << "iCTS run successfully." << std::endl;
  return 1;
}

/////////////////////////////////////////////////////////////

CmdCTSReport::CmdCTSReport(const char* cmd_name) : TclCmd(cmd_name)
{
  auto* option = new TclStringOption(TCL_NAME, 1, nullptr);
  addOption(option);

  auto* path = new TclStringOption(TCL_PATH, 1);
  addOption(path);
}

unsigned CmdCTSReport::check()
{
  TclOption* option = getOptionOrArg(TCL_NAME);
  LOG_FATAL_IF(!option);

  TclOption* path = getOptionOrArg(TCL_PATH);
  LOG_FATAL_IF(!path);
  return 1;
}

unsigned CmdCTSReport::exec()
{
  if (!check()) {
    return 0;
  }

  TclOption* option = getOptionOrArg(TCL_NAME);
  auto name = option->getStringVal();
  if (name != nullptr) {
    if (CTS_API_INST.report(name).ok()) {
      return 1;
    }
    LOG_FATAL << "iCTS report failed." << std::endl;
  }

  TclOption* def_path = getOptionOrArg(TCL_PATH);
  auto str_path = def_path->getStringVal();
  if (str_path != nullptr) {
    if (CTS_API_INST.report(str_path).ok()) {
      return 1;
    }
    LOG_FATAL << "iCTS report failed." << std::endl;
  }

  return 1;
}
}  // namespace tcl
