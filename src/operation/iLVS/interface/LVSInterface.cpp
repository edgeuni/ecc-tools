#include "LVSInterface.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ilvs {

LVSInterface& LVSInterface::getInst()
{
  if (_lvs_interface_instance == nullptr) {
    _lvs_interface_instance = new LVSInterface();
  }
  return *_lvs_interface_instance;
}

void LVSInterface::destroyInst()
{
  if (_lvs_interface_instance != nullptr) {
    delete _lvs_interface_instance;
    _lvs_interface_instance = nullptr;
  }
}

#if 1  // 外部调用LVS的API

#if 1  // iLVS

void LVSInterface::initLVS(const std::map<std::string, std::any>& config_map)
{
  Logger::initInst();
  LVSLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  LVSLOG.info(Loc::current(), "iLVS initialization.");
  LVSLOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  DataManager::initInst();
  std::map<std::string, std::any> mutable_config_map = config_map;
  LVSDM.input(mutable_config_map);

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::runLVS()
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void LVSInterface::destroyLVS()
{
  Monitor monitor;
  LVSLOG.info(Loc::current(), "Starting...");

  LVSDM.output();
  DataManager::destroyInst();

  LVSLOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  Logger::destroyInst();
}

// private

LVSInterface* LVSInterface::_lvs_interface_instance = nullptr;

#endif

#endif

}  // namespace ilvs
