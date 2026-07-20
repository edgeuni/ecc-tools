// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// ***************************************************************************************
#include "NetlistExtractor.hpp"

#include <algorithm>

#include "IdbDesign.h"
#include "IdbInstance.h"
#include "IdbNet.h"
#include "IdbPins.h"

namespace ilvs {

LVSNetlist NetlistExtractor::extract(idb::IdbDesign* design)
{
  LVSNetlist netlist;
  if (design == nullptr || design->get_net_list() == nullptr) {
    return netlist;
  }

  netlist.design_name = design->get_design_name();
  for (idb::IdbNet* idb_net : design->get_net_list()->get_net_list()) {
    if (idb_net == nullptr) {
      continue;
    }
    LVSNet net;
    net.name = idb_net->get_net_name();
    net.wire_segment_num = idb_net->get_segment_wire_num();
    net.via_num = idb_net->get_via_num();

    auto add_terminal_list = [&net](idb::IdbPins* pin_list) {
      if (pin_list == nullptr) {
        return;
      }
      for (idb::IdbPin* pin : pin_list->get_pin_list()) {
        if (pin != nullptr) {
          net.terminal_list.push_back(getTerminalName(pin));
        }
      }
    };
    add_terminal_list(idb_net->get_io_pins());
    add_terminal_list(idb_net->get_instance_pin_list());
    std::sort(net.terminal_list.begin(), net.terminal_list.end());
    net.terminal_list.erase(std::unique(net.terminal_list.begin(), net.terminal_list.end()), net.terminal_list.end());
    netlist.net_map.emplace(net.name, std::move(net));
  }
  return netlist;
}

std::string NetlistExtractor::getTerminalName(idb::IdbPin* pin)
{
  if (pin->is_io_pin()) {
    return "PIN/" + pin->get_pin_name();
  }
  idb::IdbInstance* instance = pin->get_instance();
  if (instance == nullptr) {
    return "PIN/" + pin->get_pin_name();
  }
  return instance->get_name() + "/" + pin->get_pin_name();
}

}  // namespace ilvs
