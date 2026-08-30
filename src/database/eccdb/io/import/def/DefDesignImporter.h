#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "design/DesignDatabase.h"

namespace idb::eccdb {

struct DefDesignImportDiagnostic
{
  std::string statement;
  std::size_t occurrence_count = 0;
};

// One-shot importer from SI2 DEF callbacks into one empty DesignDatabase.
// Callbacks create EnTT objects immediately. Only unresolved cross-section
// references are retained until the referenced object is available.
class DefDesignImporter
{
 public:
  explicit DefDesignImporter(DesignDatabase& design) : _design(design) {}

  void import(const std::filesystem::path& file);

  [[nodiscard]] const std::vector<DefDesignImportDiagnostic>& diagnostics() const noexcept { return _diagnostics; }

 private:
  DesignDatabase& _design;
  std::vector<DefDesignImportDiagnostic> _diagnostics;
  bool _used = false;
};

}  // namespace idb::eccdb
