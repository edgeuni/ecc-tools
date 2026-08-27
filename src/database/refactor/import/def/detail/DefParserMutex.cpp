#include "import/def/detail/DefParserMutex.h"

namespace idb::refactor::def_detail {

std::mutex& parserMutex()
{
  static std::mutex mutex;
  return mutex;
}

}  // namespace idb::refactor::def_detail
