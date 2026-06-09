#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ifill {

struct Rect
{
  int32_t lx = 0;
  int32_t ly = 0;
  int32_t ux = 0;
  int32_t uy = 0;

  bool operator==(const Rect&) const = default;
};

struct FillShape
{
  int32_t width = 0;
  int32_t height = 0;

  bool operator==(const FillShape&) const = default;
};

struct LayerFillRule
{
  std::string layer_name;
  bool horizontal = true;
  std::vector<FillShape> shapes;
  int32_t space_to_fill = 0;
  int32_t space_to_non_fill = 0;
};

struct LayerFillInput
{
  Rect bounds;
  LayerFillRule rule;
  std::vector<Rect> occupied;
};

class MetalFillGenerator
{
 public:
  std::vector<Rect> generate(const LayerFillInput& input) const;
};

}  // namespace ifill
