#include "ifill_api.h"

#include "ifill_runner.h"

namespace ifill {

IFillApi* IFillApi::_instance = nullptr;

int32_t IFillApi::runMetalFill(const std::string& rule_file, const std::vector<int32_t>& area, bool reset_fill)
{
  IFillRunOptions options;
  options.rule_file = rule_file;
  options.area = area;
  options.reset_fill = reset_fill;

  return IFillRunner().run(options);
}

}  // namespace ifill
