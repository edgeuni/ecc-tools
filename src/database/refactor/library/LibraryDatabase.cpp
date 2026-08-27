#include "library/LibraryDatabase.h"

namespace idb::refactor {

LibraryDatabase::LibraryDatabase(const TechRegistry& tech_registry, LibraryDatabaseOptions options)
    : _tech_registry(tech_registry),
      _geometry(options.geometry),
      _sites(_registry),
      _cell_masters(_registry, _tech_registry, _geometry),
      _master_terms(_registry),
      _master_ports(_registry, _tech_registry, _geometry)
{
}

}  // namespace idb::refactor
