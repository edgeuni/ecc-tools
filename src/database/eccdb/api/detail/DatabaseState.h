// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include <memory>
#include <vector>

#include "eccdb/Config.h"
#include "eccdb/Database.h"

namespace eccdb {

class DesignDatabase;
class LibraryDatabase;
class TechDatabase;

namespace detail {

class DatabaseState
{
 public:
  explicit DatabaseState(DesignDatabase& borrowed_design) noexcept;
  ~DatabaseState();
  DatabaseState(const DatabaseState&) = delete;
  DatabaseState& operator=(const DatabaseState&) = delete;

  [[nodiscard]] static std::unique_ptr<DatabaseState> open(const Config& config);

  [[nodiscard]] DesignDatabase& design() const noexcept { return *_design; }
  [[nodiscard]] TechDatabase* technology() const noexcept { return _technology.get(); }
  [[nodiscard]] LibraryDatabase* library() const noexcept { return _library.get(); }
  [[nodiscard]] const std::vector<ImportDiagnostic>& diagnostics() const noexcept { return _diagnostics; }

  void writeDef(const std::filesystem::path& file) const;
  void writeBinary(const BinaryFiles& files) const;

 private:
  DatabaseState() = default;

  std::unique_ptr<TechDatabase> _technology;
  std::unique_ptr<LibraryDatabase> _library;
  std::unique_ptr<DesignDatabase> _owned_design;
  DesignDatabase* _design = nullptr;
  std::vector<ImportDiagnostic> _diagnostics;
};

}  // namespace detail
}  // namespace eccdb
