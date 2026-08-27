#pragma once

#include <filesystem>
#include <iosfwd>

namespace idb::refactor {

class LibraryDatabase;
class TechDatabase;

// Deterministic LEF writer for one LibraryDatabase. Technology references are
// resolved through the supplied TechDatabase and are emitted by name.
class LefLibraryExporter
{
 public:
  static void write(std::ostream& output, const TechDatabase& technology, const LibraryDatabase& library);
  static void write(const std::filesystem::path& path, const TechDatabase& technology, const LibraryDatabase& library);
};

}  // namespace idb::refactor
