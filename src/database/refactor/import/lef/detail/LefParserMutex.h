#pragma once

#include <mutex>

namespace idb::refactor::lef_detail {

// SI2 keeps parser callbacks and session state globally. Every direct LEF
// importer must hold this mutex for the complete sequence of input files.
[[nodiscard]] std::mutex& parserMutex();

}  // namespace idb::refactor::lef_detail
