#pragma once

#include <string>

#include "LVSDatabase.hpp"

namespace ilvs {

class LVSReporter
{
 public:
  static void report(const LVSCheckResult& check_result, const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist,
                     const std::string& report_directory_path);
};

}  // namespace ilvs
