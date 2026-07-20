#include "DataManager.hpp"

namespace ilvs {

DataManager& DataManager::getInst()
{
  static DataManager instance;
  return instance;
}

void DataManager::init(const std::map<std::string, std::any>& config_map)
{
  (void) config_map;
  _is_initialized = true;
}

bool DataManager::isInitialized() const
{
  return _is_initialized;
}

void DataManager::destroy()
{
  _is_initialized = false;
}

}  // namespace ilvs
