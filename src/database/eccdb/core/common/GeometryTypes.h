#pragma once

#include <algorithm>
#include <cstdint>

namespace idb::eccdb {

enum class GeometryLayerKind : uint8_t
{
  kNone,
  kRouting,
  kCut,
  kOther
};

struct Point
{
  int32_t x = 0;
  int32_t y = 0;

  template <typename Visitor>
  void visitFields(Visitor& v)
  {
    v("x", x);
    v("y", y);
  }

  template <typename Visitor>
  void visitFields(Visitor& v) const
  {
    v("x", x);
    v("y", y);
  }
};

[[nodiscard]] inline bool operator==(Point lhs, Point rhs) noexcept
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

[[nodiscard]] inline bool operator!=(Point lhs, Point rhs) noexcept
{
  return !(lhs == rhs);
}

// Axis-aligned rectangle in database units.
struct Rect
{
  int32_t ll_x = 0;
  int32_t ll_y = 0;
  int32_t ur_x = 0;
  int32_t ur_y = 0;

  [[nodiscard]] bool isValid() const noexcept { return ll_x <= ur_x && ll_y <= ur_y; }
  [[nodiscard]] bool hasArea() const noexcept { return ll_x < ur_x && ll_y < ur_y; }
  [[nodiscard]] int64_t width() const noexcept { return static_cast<int64_t>(ur_x) - ll_x; }
  [[nodiscard]] int64_t height() const noexcept { return static_cast<int64_t>(ur_y) - ll_y; }
  [[nodiscard]] int64_t area() const noexcept { return hasArea() ? width() * height() : 0; }

  [[nodiscard]] Point center() const noexcept
  {
    return Point{ll_x + static_cast<int32_t>(width() / 2), ll_y + static_cast<int32_t>(height() / 2)};
  }

  [[nodiscard]] Rect normalized() const noexcept
  {
    return Rect{.ll_x = std::min(ll_x, ur_x), .ll_y = std::min(ll_y, ur_y), .ur_x = std::max(ll_x, ur_x), .ur_y = std::max(ll_y, ur_y)};
  }

  [[nodiscard]] Rect offset(int32_t dx, int32_t dy) const noexcept
  {
    return Rect{.ll_x = ll_x + dx, .ll_y = ll_y + dy, .ur_x = ur_x + dx, .ur_y = ur_y + dy};
  }

  [[nodiscard]] Rect enlarged(int32_t delta) const noexcept { return enlarged(delta, delta, delta, delta); }

  [[nodiscard]] Rect enlarged(int32_t left, int32_t bottom, int32_t right, int32_t top) const noexcept
  {
    return Rect{.ll_x = ll_x - left, .ll_y = ll_y - bottom, .ur_x = ur_x + right, .ur_y = ur_y + top};
  }

  [[nodiscard]] bool contains(Point point, bool boundary = true) const noexcept
  {
    if (!isValid()) {
      return false;
    }
    if (boundary) {
      return point.x >= ll_x && point.x <= ur_x && point.y >= ll_y && point.y <= ur_y;
    }
    return point.x > ll_x && point.x < ur_x && point.y > ll_y && point.y < ur_y;
  }

  [[nodiscard]] bool contains(Rect rect, bool boundary = true) const noexcept
  {
    if (!isValid() || !rect.isValid()) {
      return false;
    }
    if (boundary) {
      return rect.ll_x >= ll_x && rect.ur_x <= ur_x && rect.ll_y >= ll_y && rect.ur_y <= ur_y;
    }
    return rect.ll_x > ll_x && rect.ur_x < ur_x && rect.ll_y > ll_y && rect.ur_y < ur_y;
  }

  [[nodiscard]] bool intersects(Rect rect, bool boundary = true) const noexcept
  {
    if (!isValid() || !rect.isValid()) {
      return false;
    }
    if (boundary) {
      return !(ur_x < rect.ll_x || rect.ur_x < ll_x || ur_y < rect.ll_y || rect.ur_y < ll_y);
    }
    return !(ur_x <= rect.ll_x || rect.ur_x <= ll_x || ur_y <= rect.ll_y || rect.ur_y <= ll_y);
  }

  [[nodiscard]] Rect united(Rect rect) const noexcept
  {
    if (!isValid()) {
      return rect;
    }
    if (!rect.isValid()) {
      return *this;
    }
    return Rect{.ll_x = std::min(ll_x, rect.ll_x),
                .ll_y = std::min(ll_y, rect.ll_y),
                .ur_x = std::max(ur_x, rect.ur_x),
                .ur_y = std::max(ur_y, rect.ur_y)};
  }

  template <typename Visitor>
  void visitFields(Visitor& v)
  {
    v("ll_x", ll_x);
    v("ll_y", ll_y);
    v("ur_x", ur_x);
    v("ur_y", ur_y);
  }

  template <typename Visitor>
  void visitFields(Visitor& v) const
  {
    v("ll_x", ll_x);
    v("ll_y", ll_y);
    v("ur_x", ur_x);
    v("ur_y", ur_y);
  }
};

[[nodiscard]] inline bool operator==(Rect lhs, Rect rhs) noexcept
{
  return lhs.ll_x == rhs.ll_x && lhs.ll_y == rhs.ll_y && lhs.ur_x == rhs.ur_x && lhs.ur_y == rhs.ur_y;
}

[[nodiscard]] inline bool operator!=(Rect lhs, Rect rhs) noexcept
{
  return !(lhs == rhs);
}

}  // namespace idb::eccdb
