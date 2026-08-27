#pragma once

#include <span>
#include <string_view>

#include "design/common/DesignGeometry.h"

namespace idb::refactor {

class TechRegistry;

[[nodiscard]] Rect validateDesignOrthogonalBoundary(std::span<const Point> boundary, std::string_view object_name);
void validateDesignShapeSet(const DesignShapeSet& shapes, bool require_nonempty = true);
void validateDesignLayerGeometry(const TechRegistry& technology, std::span<const DesignLayerGeometry> geometry,
                                 bool require_nonempty = true);

}  // namespace idb::refactor
