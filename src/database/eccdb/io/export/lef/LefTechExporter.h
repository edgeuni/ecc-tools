#pragma once

#include <filesystem>
#include <iosfwd>

namespace eccdb {

class TechDatabase;

// Deterministic LEF writer for the technology data represented by
// TechDatabase. It writes a canonical supported subset, rather than preserving
// source whitespace or cross-category statement order.
class LefTechExporter
{
 public:
  static void write(std::ostream& output, const TechDatabase& database);
  static void write(const std::filesystem::path& path, const TechDatabase& database);
};

}  // namespace eccdb
