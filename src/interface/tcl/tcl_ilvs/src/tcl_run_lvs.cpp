#include "LVSInterface.hpp"
#include "tcl_ilvs.h"
#include "tcl_util.h"

namespace tcl {

TclRunLVS::TclRunLVS(const char* cmd_name) : TclCmd(cmd_name)
{
}

unsigned TclRunLVS::exec()
{
  if (!check()) {
    return 0;
  }
  LVSI.runLVS();
  return 1;
}

}  // namespace tcl
