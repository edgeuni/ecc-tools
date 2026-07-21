#include "LVSInterface.hpp"
#include "tcl_ilvs.h"
#include "tcl_util.h"

namespace tcl {

TclInitLVS::TclInitLVS(const char* cmd_name) : TclCmd(cmd_name)
{
  // std::string temp_directory_path;  // required
  _config_list.push_back(std::make_pair("-temp_directory_path", ValueType::kString));
  // int32_t thread_number;             // optional
  _config_list.push_back(std::make_pair("-thread_number", ValueType::kInt));

  TclUtil::addOption(this, _config_list);
}

unsigned TclInitLVS::exec()
{
  if (!check()) {
    return 0;
  }
  std::map<std::string, std::any> config_map = TclUtil::getConfigMap(this, _config_list);
  LVSI.initLVS(config_map);
  return 1;
}

}  // namespace tcl
