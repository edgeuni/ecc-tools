#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>

#include "design/DesignDatabase.h"

namespace idb::eccdb {

// Deterministic DEF 5.8 writer for the syntax represented by Design V1.
class DefDesignExporter
{
 public:
  explicit DefDesignExporter(const DesignDatabase& design) : _design(design) {}

  void write(std::ostream& output) const;
  [[nodiscard]] std::string exportText() const;
  void write(const std::filesystem::path& file) const;

 private:
  const DesignDatabase& _design;
};

}  // namespace idb::eccdb
