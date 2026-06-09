#include "ifill_rule_parser.h"

#include <cmath>
#include <fstream>
#include <stdexcept>

#include "json.hpp"

namespace ifill {
namespace {

using Json = nlohmann::json;

int32_t toDbu(double value, int32_t dbu_per_micron)
{
  return static_cast<int32_t>(std::llround(value * dbu_per_micron));
}

std::vector<double> readNumberList(const Json& json)
{
  std::vector<double> values;
  if (json.is_array()) {
    for (const auto& value : json) {
      values.push_back(value.get<double>());
    }
  } else {
    values.push_back(json.get<double>());
  }
  return values;
}

std::vector<std::string> readLayerNames(const Json& layer_group)
{
  std::vector<std::string> names;
  if (layer_group.contains("names")) {
    for (const auto& name : layer_group.at("names")) {
      names.push_back(name.get<std::string>());
    }
  } else if (layer_group.contains("name")) {
    names.push_back(layer_group.at("name").get<std::string>());
  }
  return names;
}

std::vector<FillShape> readShapes(const Json& shape_config, int32_t dbu_per_micron)
{
  const auto widths = readNumberList(shape_config.at("width"));
  const auto heights = readNumberList(shape_config.at("height"));
  if (widths.size() != heights.size()) {
    throw std::runtime_error("ifill width and height lists must have the same length");
  }

  std::vector<FillShape> shapes;
  shapes.reserve(widths.size());
  for (size_t i = 0; i < widths.size(); ++i) {
    shapes.push_back({toDbu(widths[i], dbu_per_micron), toDbu(heights[i], dbu_per_micron)});
  }
  return shapes;
}

}  // namespace

std::vector<LayerFillRule> loadLayerFillRules(const std::string& rule_file, int32_t dbu_per_micron)
{
  std::ifstream in(rule_file);
  if (!in.is_open()) {
    throw std::runtime_error("failed to open ifill rule file: " + rule_file);
  }

  Json root;
  in >> root;

  std::vector<LayerFillRule> rules;
  for (const auto& [_, layer_group] : root.at("layers").items()) {
    const auto layer_names = readLayerNames(layer_group);
    const auto& non_opc = layer_group.at("non-opc");

    LayerFillRule base_rule;
    base_rule.shapes = readShapes(non_opc, dbu_per_micron);
    base_rule.space_to_fill = toDbu(non_opc.at("space_to_fill").get<double>(), dbu_per_micron);
    base_rule.space_to_non_fill = toDbu(non_opc.at("space_to_non_fill").get<double>(), dbu_per_micron);

    for (const auto& layer_name : layer_names) {
      LayerFillRule rule = base_rule;
      rule.layer_name = layer_name;
      rules.push_back(std::move(rule));
    }
  }

  return rules;
}

}  // namespace ifill
