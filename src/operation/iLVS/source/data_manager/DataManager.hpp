#pragma once

#include <any>
#include <map>
#include <string>

namespace ilvs {

#define LVSDM (ilvs::DataManager::getInst())

class DataManager
{
 public:
  static DataManager& getInst();

  void init(const std::map<std::string, std::any>& config_map);
  bool isInitialized() const;
  void destroy();

 private:
  DataManager() = default;

  bool _is_initialized = false;
};

}  // namespace ilvs
