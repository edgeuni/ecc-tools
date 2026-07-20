#include "LVSInterface.hpp"
#include "tcl_ilvs.h"
#include "tcl_util.h"

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
