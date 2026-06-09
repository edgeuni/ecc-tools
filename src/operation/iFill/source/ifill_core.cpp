#include "ifill_core.h"

#include <algorithm>

namespace ifill {
namespace {

bool isValid(const Rect& rect)
{
  return rect.lx < rect.ux && rect.ly < rect.uy;
}

Rect expand(const Rect& rect, int32_t amount)
{
  return {rect.lx - amount, rect.ly - amount, rect.ux + amount, rect.uy + amount};
}

bool intersects(const Rect& lhs, const Rect& rhs)
{
  return lhs.lx < rhs.ux && rhs.lx < lhs.ux && lhs.ly < rhs.uy && rhs.ly < lhs.uy;
}

bool isBlockedByOccupied(const Rect& candidate, const std::vector<Rect>& occupied, int32_t spacing)
{
  const Rect blocked_area = expand(candidate, spacing);
  return std::ranges::any_of(occupied, [&](const Rect& rect) { return intersects(blocked_area, rect); });
}

bool isBlockedByFill(const Rect& candidate, const std::vector<Rect>& fills, int32_t spacing)
{
  return std::ranges::any_of(fills, [&](const Rect& rect) { return intersects(candidate, expand(rect, spacing)); });
}

FillShape orientShape(FillShape shape, bool horizontal)
{
  if ((horizontal && shape.width < shape.height) || (!horizontal && shape.height < shape.width)) {
    std::swap(shape.width, shape.height);
  }
  return shape;
}

}  // namespace

std::vector<Rect> MetalFillGenerator::generate(const LayerFillInput& input) const
{
  std::vector<Rect> fills;
  if (!isValid(input.bounds)) {
    return fills;
  }

  for (FillShape raw_shape : input.rule.shapes) {
    FillShape shape = orientShape(raw_shape, input.rule.horizontal);
    if (shape.width <= 0 || shape.height <= 0) {
      continue;
    }

    const int32_t x_step = shape.width + input.rule.space_to_fill;
    const int32_t y_step = shape.height + input.rule.space_to_fill;
    if (x_step <= 0 || y_step <= 0) {
      continue;
    }

    for (int32_t x = input.bounds.lx; x + shape.width <= input.bounds.ux; x += x_step) {
      for (int32_t y = input.bounds.ly; y + shape.height <= input.bounds.uy; y += y_step) {
        Rect candidate{x, y, x + shape.width, y + shape.height};
        if (isBlockedByOccupied(candidate, input.occupied, input.rule.space_to_non_fill)) {
          continue;
        }
        if (isBlockedByFill(candidate, fills, input.rule.space_to_fill)) {
          continue;
        }
        fills.push_back(candidate);
      }
    }
  }

  return fills;
}

}  // namespace ifill
