#pragma once

#include <entt/entt.hpp>

#include "library/common/LibraryTypes.h"

namespace idb::refactor {

// One registry owns all complete entities for one loaded cell library.
class LibraryRegistry
{
 public:
  using entity_type = LibraryEntity;
  using registry_type = entt::basic_registry<entity_type>;

  [[nodiscard]] registry_type& registry() noexcept { return _registry; }
  [[nodiscard]] const registry_type& registry() const noexcept { return _registry; }

 private:
  registry_type _registry;
};

}  // namespace idb::refactor
