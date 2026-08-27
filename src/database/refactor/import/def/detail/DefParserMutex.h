#pragma once

#include <mutex>

namespace idb::refactor::def_detail {

// SI2 DEF callbacks and parser session state are process-global.
[[nodiscard]] std::mutex& parserMutex();

}  // namespace idb::refactor::def_detail
