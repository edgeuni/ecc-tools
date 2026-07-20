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

  void initLVS(const std::map<std::string, std::any>& config_map);
  void runLVS();
  void destroyLVS();

 private:
  LVSInterface() = default;
};

}  // namespace ilvs
