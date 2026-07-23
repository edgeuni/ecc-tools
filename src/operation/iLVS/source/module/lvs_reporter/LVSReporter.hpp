#pragma once

#include <string>
#include <vector>

#include "LVSDatabase.hpp"
#include "LVSHeader.hpp"

namespace ilvs {

class LVSReporter
{
 public:
  static std::vector<fort::char_table> getSummaryTableList(const LVSCheckResult& check_result, const LVSNetlist& expected_netlist,
                                                            const LVSNetlist& physical_netlist);

  static void report(const LVSCheckResult& check_result, const LVSNetlist& expected_netlist, const LVSNetlist& physical_netlist,
                     const std::string& report_directory_path);
};

}  // namespace ilvs
