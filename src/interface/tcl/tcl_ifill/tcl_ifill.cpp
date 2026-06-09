#include "tcl_ifill.h"

#include <iostream>
#include <stdexcept>

#include "ifill_api.h"

namespace tcl {

TclRunIFill::TclRunIFill(const char* cmd_name) : TclCmd(cmd_name)
{
  addOption(new TclStringOption("-rules", 0));
  addOption(new TclIntListOption("-area", 1));
  addOption(new TclIntOption("-reset_fill", 1, 0));
}

unsigned TclRunIFill::check()
{
  auto* rules = getOptionOrArg("-rules");
  LOG_FATAL_IF(!rules || !rules->is_set_val() || rules->getStringVal() == nullptr) << "run_ifill requires -rules <json>";
  return 1;
}

unsigned TclRunIFill::exec()
{
  if (!check()) {
    return 0;
  }

  auto* rules = getOptionOrArg("-rules");
  auto* area = getOptionOrArg("-area");
  auto* reset_fill = getOptionOrArg("-reset_fill");

  std::vector<int32_t> area_values;
  if (area != nullptr && area->is_set_val()) {
    for (int value : area->getIntList()) {
      area_values.push_back(value);
    }
  }
  const bool reset = reset_fill != nullptr && reset_fill->is_set_val() && reset_fill->getIntVal() != 0;

  try {
    const int32_t count = ifillApiInst->runMetalFill(rules->getStringVal(), area_values, reset);
    std::cout << "iFill inserted " << count << " metal fill rectangles." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "iFill failed: " << e.what() << std::endl;
    return 0;
  }

  return 1;
}

}  // namespace tcl
