#include "import/lef/detail/LefParserMutex.h"

namespace idb::refactor::lef_detail {

std::mutex& parserMutex()
{
  static std::mutex mutex;
  return mutex;
}

}  // namespace idb::refactor::lef_detail
