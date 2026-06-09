#pragma once

#include "UserShell.hh"
#include "tcl_ifill.h"

using namespace ieda;

namespace tcl {

int registerCmdIFill()
{
  registerTclCmd(TclRunIFill, "run_ifill");
  return EXIT_SUCCESS;
}

}  // namespace tcl
