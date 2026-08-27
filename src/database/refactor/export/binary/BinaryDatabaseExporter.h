#pragma once

#include <filesystem>

namespace idb::refactor {

class DesignDatabase;
class LibraryDatabase;
class TechDatabase;

class BinaryDatabaseExporter
{
 public:
  static void saveTech(const std::filesystem::path& path, const TechDatabase& database);
  static void saveLibrary(const std::filesystem::path& path, const LibraryDatabase& library);
  static void saveDesign(const std::filesystem::path& path, const DesignDatabase& design);
};

}  // namespace idb::refactor
