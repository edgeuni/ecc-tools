#include "import/lef/detail/LefParserMutex.h"

namespace idb::eccdb::lef_detail {

std::mutex& parserMutex()
{
  static std::mutex mutex;
  return mutex;
}

}  // namespace idb::eccdb::lef_detail
