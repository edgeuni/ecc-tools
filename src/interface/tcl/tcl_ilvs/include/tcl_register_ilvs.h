#pragma once

#include "tcl_ilvs.h"

using namespace ieda;

namespace tcl {

int registerCmdLVS()
{
  // lvs
  registerTclCmd(TclInitLVS, "init_lvs");
  registerTclCmd(TclRunLVS, "run_lvs");
  registerTclCmd(TclDestroyLVS, "destroy_lvs");
  return EXIT_SUCCESS;
}

}  // namespace tcl
