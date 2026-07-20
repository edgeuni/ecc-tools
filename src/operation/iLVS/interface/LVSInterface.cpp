#include "LVSInterface.hpp"

#include "DataManager.hpp"

namespace ilvs {

LVSInterface& LVSInterface::getInst()
{
  static LVSInterface instance;
  return instance;
}

void LVSInterface::initLVS(const std::map<std::string, std::any>& config_map)
{
  LVSDM.init(config_map);
}

void LVSInterface::runLVS()
{
  if (!LVSDM.isInitialized()) {
    return;
  }
}

void LVSInterface::destroyLVS()
{
  LVSDM.destroy();
}

}  // namespace ilvs
