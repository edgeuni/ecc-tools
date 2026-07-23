// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#pragma once

#include <cstdint>
#include <string>

#include "LVSDatabase.hpp"

namespace ilvs {

enum class LVSSnapshotType : uint32_t
{
  kLogical = 1,
  kPhysical = 2,
};

class LVSSnapshotIO
{
 public:
  static bool write(const LVSNetlist& netlist, LVSSnapshotType snapshot_type, const std::string& file_path, std::string& error_message);
  static bool read(const std::string& file_path, LVSSnapshotType expected_snapshot_type, LVSNetlist& netlist, std::string& error_message);
};

}  // namespace ilvs
