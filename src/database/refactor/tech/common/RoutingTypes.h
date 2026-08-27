#pragma once

#include <cstdint>

namespace idb::refactor {

// Routing wire direction used by LEF layer clauses. This describes the routing
// wire orientation, not the direction of a via object.
enum class RoutingDirection : uint8_t
{
  kUnknown,
  kHorizontal,
  kVertical
};

}  // namespace idb::refactor
