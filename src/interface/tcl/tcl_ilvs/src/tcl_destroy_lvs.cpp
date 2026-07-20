#include "LVSInterface.hpp"
#include "tcl_ilvs.h"

namespace tcl {

TclDestroyLVS::TclDestroyLVS(const char* cmd_name) : TclCmd(cmd_name)
{
}

unsigned TclDestroyLVS::exec()
{
  if (!check()) {
    return 0;
  }
  LVSI.destroyLVS();
  return 1;
}

}  // namespace tcl
