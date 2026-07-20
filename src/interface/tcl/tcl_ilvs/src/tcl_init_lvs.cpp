#include "LVSInterface.hpp"
#include "tcl_ilvs.h"

namespace tcl {

TclInitLVS::TclInitLVS(const char* cmd_name) : TclCmd(cmd_name)
{
}

unsigned TclInitLVS::exec()
{
  if (!check()) {
    return 0;
  }
  LVSI.initLVS({});
  return 1;
}

}  // namespace tcl
