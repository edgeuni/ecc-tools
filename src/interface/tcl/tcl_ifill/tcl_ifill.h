#pragma once

#include "tcl_util.h"

namespace tcl {

class TclRunIFill : public TclCmd
{
 public:
  explicit TclRunIFill(const char* cmd_name);
  ~TclRunIFill() override = default;

  unsigned check() override;
  unsigned exec() override;
};

}  // namespace tcl
