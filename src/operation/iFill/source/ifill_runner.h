#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ifill {

struct IFillRunOptions
{
  std::string rule_file;
  std::vector<int32_t> area;
  bool reset_fill = false;
};

class IFillRunner
{
 public:
  int32_t run(const IFillRunOptions& options) const;
};

}  // namespace ifill
