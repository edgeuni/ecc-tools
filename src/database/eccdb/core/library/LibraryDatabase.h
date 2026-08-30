#pragma once

#include "geometry/GeometryPool.h"
#include "library/LibraryRegistry.h"
#include "library/cell_master/storage/CellMasterStorage.h"
#include "library/master_port/storage/MasterPortStorage.h"
#include "library/master_term/storage/MasterTermStorage.h"
#include "library/site/storage/SiteStorage.h"

namespace idb::eccdb {

class TechRegistry;

struct LibraryDatabaseOptions
{
  GeometryPoolOptions geometry;
};

// A LibraryDatabase is one loaded cell-library scope. It is a composition root
// for its registry-backed object storages, not itself an EnTT entity.
class LibraryDatabase
{
 public:
  // Technology outlives the library because PORT and OBS geometry reference
  // Tech layers and ViaMasters from a different registry domain.
  explicit LibraryDatabase(const TechRegistry& tech_registry, LibraryDatabaseOptions options = {});
  LibraryDatabase(const LibraryDatabase&) = delete;
  LibraryDatabase& operator=(const LibraryDatabase&) = delete;
  LibraryDatabase(LibraryDatabase&&) = delete;
  LibraryDatabase& operator=(LibraryDatabase&&) = delete;

  [[nodiscard]] LibraryRegistry& libraryRegistry() noexcept { return _registry; }
  [[nodiscard]] const LibraryRegistry& libraryRegistry() const noexcept { return _registry; }
  [[nodiscard]] GeometryPool& geometryPool() noexcept { return _geometry; }
  [[nodiscard]] const GeometryPool& geometryPool() const noexcept { return _geometry; }

  [[nodiscard]] LibrarySiteStorage& siteStorage() noexcept { return _sites; }
  [[nodiscard]] const LibrarySiteStorage& siteStorage() const noexcept { return _sites; }
  [[nodiscard]] LibraryCellMasterStorage& cellMasterStorage() noexcept { return _cell_masters; }
  [[nodiscard]] const LibraryCellMasterStorage& cellMasterStorage() const noexcept { return _cell_masters; }
  [[nodiscard]] LibraryMasterTermStorage& masterTermStorage() noexcept { return _master_terms; }
  [[nodiscard]] const LibraryMasterTermStorage& masterTermStorage() const noexcept { return _master_terms; }
  [[nodiscard]] LibraryMasterPortStorage& masterPortStorage() noexcept { return _master_ports; }
  [[nodiscard]] const LibraryMasterPortStorage& masterPortStorage() const noexcept { return _master_ports; }

 private:
  const TechRegistry& _tech_registry;
  LibraryRegistry _registry;
  GeometryPool _geometry;
  LibrarySiteStorage _sites;
  LibraryCellMasterStorage _cell_masters;
  LibraryMasterTermStorage _master_terms;
  LibraryMasterPortStorage _master_ports;
};

}  // namespace idb::eccdb
