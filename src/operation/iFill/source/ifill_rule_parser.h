#pragma once

#include <string>
#include <vector>

#include "ifill_core.h"

namespace ifill {

std::vector<LayerFillRule> loadLayerFillRules(const std::string& rule_file, int32_t dbu_per_micron);

}  // namespace ifill
