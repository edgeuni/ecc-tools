#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

#include "ifill_core.h"
#include "ifill_rule_parser.h"

namespace {

void testGenerateRegularFillTiles()
{
  ifill::LayerFillInput input;
  input.bounds = {0, 0, 8, 5};
  input.rule.layer_name = "M2";
  input.rule.horizontal = true;
  input.rule.shapes = {{2, 2}};
  input.rule.space_to_fill = 1;
  input.rule.space_to_non_fill = 1;

  const auto fills = ifill::MetalFillGenerator().generate(input);

  assert((fills == std::vector<ifill::Rect>{{0, 0, 2, 2}, {0, 3, 2, 5}, {3, 0, 5, 2}, {3, 3, 5, 5}, {6, 0, 8, 2}, {6, 3, 8, 5}}));
}

void testKeepsSpacingFromOccupiedShapes()
{
  ifill::LayerFillInput input;
  input.bounds = {0, 0, 8, 2};
  input.rule.layer_name = "M2";
  input.rule.horizontal = true;
  input.rule.shapes = {{2, 2}};
  input.rule.space_to_fill = 1;
  input.rule.space_to_non_fill = 1;
  input.occupied = {{3, 0, 5, 2}};

  const auto fills = ifill::MetalFillGenerator().generate(input);

  assert((fills == std::vector<ifill::Rect>{{0, 0, 2, 2}, {6, 0, 8, 2}}));
}

void testOrientsLongSideToPreferredDirection()
{
  ifill::LayerFillInput input;
  input.bounds = {0, 0, 4, 2};
  input.rule.layer_name = "M1";
  input.rule.horizontal = true;
  input.rule.shapes = {{2, 4}};

  const auto fills = ifill::MetalFillGenerator().generate(input);

  assert((fills == std::vector<ifill::Rect>{{0, 0, 4, 2}}));
}

void testParsesOpenRoadStyleRuleFile()
{
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "ifill_rule_test.json";
  std::ofstream out(path);
  out << R"json({
    "layers": {
      "Mx": {
        "names": ["M1", "M2"],
        "non-opc": {
          "width": [2.0, 1.0],
          "height": [1.0, 1.0],
          "space_to_fill": 0.3,
          "space_to_non_fill": 0.7
        }
      }
    }
  })json";
  out.close();

  const auto rules = ifill::loadLayerFillRules(path.string(), 1000);

  assert(rules.size() == 2);
  assert(rules[0].layer_name == "M1");
  assert((rules[0].shapes == std::vector<ifill::FillShape>{{2000, 1000}, {1000, 1000}}));
  assert(rules[0].space_to_fill == 300);
  assert(rules[0].space_to_non_fill == 700);
  assert(rules[1].layer_name == "M2");
}

}  // namespace

int main()
{
  testGenerateRegularFillTiles();
  testKeepsSpacingFromOccupiedShapes();
  testOrientsLongSideToPreferredDirection();
  testParsesOpenRoadStyleRuleFile();
  return 0;
}
