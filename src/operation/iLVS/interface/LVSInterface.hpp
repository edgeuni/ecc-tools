#pragma once

#include <any>
#include <map>
#include <string>

namespace ilvs {

#define LVSI (ilvs::LVSInterface::getInst())

class LVSInterface
{
 public:
  static LVSInterface& getInst();
  static void destroyInst();

#if 1  // 外部调用LVS的API

#if 1  // iLVS
  void initLVS(const std::map<std::string, std::any>& config_map);
  void runLVS();
  void destroyLVS();
#endif

#endif

 private:
  static LVSInterface* _lvs_interface_instance;

  LVSInterface() = default;
  LVSInterface(const LVSInterface& other) = delete;
  LVSInterface(LVSInterface&& other) = delete;
  ~LVSInterface() = default;
  LVSInterface& operator=(const LVSInterface& other) = delete;
  LVSInterface& operator=(LVSInterface&& other) = delete;
  // function
};

}  // namespace ilvs
