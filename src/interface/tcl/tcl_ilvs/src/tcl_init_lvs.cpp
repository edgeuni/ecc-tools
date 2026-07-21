#include "LVSInterface.hpp"
#include "tcl_ilvs.h"
#include "tcl_util.h"

namespace tcl {

TclInitLVS::TclInitLVS(const char* cmd_name) : TclCmd(cmd_name)
{
  // std::string netlist_path;  // required
  _config_list.push_back(std::make_pair("-netlist", ValueType::kString));
  // std::string def_path;      // required
  _config_list.push_back(std::make_pair("-def", ValueType::kString));
  // std::string top_module;    // optional
  _config_list.push_back(std::make_pair("-top_module", ValueType::kString));
  // std::string report_directory_path;  // optional
  _config_list.push_back(std::make_pair("-report_directory_path", ValueType::kString));

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
