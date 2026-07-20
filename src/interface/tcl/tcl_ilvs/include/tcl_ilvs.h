#pragma once

#include "tcl_util.h"

namespace tcl {

class TclInitLVS : public TclCmd
{
 public:
  explicit TclInitLVS(const char* cmd_name);
  ~TclInitLVS() override = default;

  unsigned check() override { return 1; };
  unsigned exec() override;
};

class TclRunLVS : public TclCmd
{
 public:
  explicit TclRunLVS(const char* cmd_name);
  ~TclRunLVS() override = default;

  unsigned check() override { return 1; };
  unsigned exec() override;
};

class TclDestroyLVS : public TclCmd
{
 public:
  explicit TclDestroyLVS(const char* cmd_name);
  ~TclDestroyLVS() override = default;

  unsigned check() override { return 1; };
  unsigned exec() override;
};

}  // namespace tcl
