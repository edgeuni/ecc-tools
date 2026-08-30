#pragma once

#include <filesystem>
#include <memory>

namespace idb::eccdb {

class DesignDatabase;
class LibraryDatabase;
class TechDatabase;

class BinaryDatabaseImporter
{
 public:
  [[nodiscard]] static std::unique_ptr<TechDatabase> loadTech(const std::filesystem::path& path);
  [[nodiscard]] static std::unique_ptr<LibraryDatabase> loadLibrary(const std::filesystem::path& path,
                                                                    const TechDatabase& technology);
  [[nodiscard]] static std::unique_ptr<DesignDatabase> loadDesign(const std::filesystem::path& path, const TechDatabase& technology,
                                                                  const LibraryDatabase& library);
};

}  // namespace idb::eccdb
