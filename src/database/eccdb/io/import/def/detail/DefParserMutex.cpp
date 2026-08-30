#include "import/def/detail/DefParserMutex.h"

namespace idb::eccdb::def_detail {

std::mutex& parserMutex()
{
  static std::mutex mutex;
  return mutex;
}

}  // namespace idb::eccdb::def_detail
