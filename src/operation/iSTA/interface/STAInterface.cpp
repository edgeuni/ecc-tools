// ***************************************************************************************
// Copyright (c) 2023-2025 Peng Cheng Laboratory
// Copyright (c) 2023-2025 Institute of Computing Technology, Chinese Academy of Sciences
// Copyright (c) 2023-2025 Beijing Institute of Open Source Chip
//
// iEDA is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
// http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "STAInterface.hpp"

#include "DataManager.hpp"
#include "GraphBuilder.hpp"
#include "GraphPropagator.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Database.hpp"
#include "STAHeader.hpp"
#include "TimingAnalyzer.hpp"
#include "idm.h"

namespace ista {

// public

STAInterface& STAInterface::getInst()
{
  if (_sta_interface_instance == nullptr) {
    _sta_interface_instance = new STAInterface();
  }
  return *_sta_interface_instance;
}

void STAInterface::destroyInst()
{
  if (_sta_interface_instance != nullptr) {
    delete _sta_interface_instance;
    _sta_interface_instance = nullptr;
  }
}

#if 1  // 外部调用STA的API

#if 1  // iSTA

void STAInterface::initSTA()
{
  Logger::initInst();
  // clang-format off
  STALOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  STALOG.info(Loc::current(), "_____ _______________________       _______________________ ________ ________ ");
  STALOG.info(Loc::current(), "___(_)__  ___/___  __/___    |      __  ___/___  __/___    |___  __ \\___  __/");
  STALOG.info(Loc::current(), "__  / _____ \\ __  /   __  /| |      _____ \\ __  /   __  /| |__  /_/ /__  /  ");
  STALOG.info(Loc::current(), "_  /  ____/ / _  /    _  ___ |      ____/ / _  /    _  ___ |_  _, _/ _  /     ");
  STALOG.info(Loc::current(), "/_/   /____/  /_/     /_/  |_|      /____/  /_/     /_/  |_|/_/ |_|  /_/      ");
  STALOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // clang-format on
  STALOG.printLogFilePath();
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  //////////////////////////////////////////////////////
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  DataManager::initInst();
  STADM.input(config_map);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void STAInterface::runSTA()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  GraphBuilder::initInst();
  STAGB.build();
  GraphBuilder::destroyInst();

  GraphPropagator::initInst();
  STAGP.build();
  GraphPropagator::destroyInst();

  TimingAnalyzer::initInst();
  STATA.build();
  TimingAnalyzer::destroyInst();

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void STAInterface::destroySTA()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  STADM.output();
  DataManager::destroyInst();

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());

  STALOG.printLogFilePath();
  // clang-format off
  STALOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  STALOG.info(Loc::current(), "_____ _______________________       _______________________   ________________________  __  ");
  STALOG.info(Loc::current(), "___(_)__  ___/___  __/___    |      ___  ____/____  _/___  | / /____  _/__  ___/___  / / /  ");
  STALOG.info(Loc::current(), "__  / _____ \\ __  /   __  /| |      __  /_     __  /  __   |/ /  __  /  _____ \\ __  /_/ / ");
  STALOG.info(Loc::current(), "_  /  ____/ / _  /    _  ___ |      _  __/    __/ /   _  /|  /  __/ /   ____/ / _  __  /    ");
  STALOG.info(Loc::current(), "/_/   /____/  /_/     /_/  |_|      /_/       /___/   /_/ |_/   /___/   /____/  /_/ /_/     ");
  STALOG.info(Loc::current(), ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // clang-format on
  Logger::destroyInst();
}

#endif

#endif

#if 1  // STA调用外部的API

#if 1  // TopData

#if 1  // input

void STAInterface::input(std::map<std::string, std::any>& config_map)
{
  (void) config_map;
  idb::IdbDesign* idb_design = dmInst->get_idb_design();
  if (idb_design == nullptr) {
    STALOG.warn(Loc::current(), "IDB design is null, skip iSTA input.");
    return;
  }

  Database& database = STADM.getDatabase();
  wrapDatabase(idb_design, database);

  STALOG.info(Loc::current(), "Input IDB to iSTA database: instances=", database.get_instance_map().size(),
              " pins=", database.get_pin_map().size(), " nets=", database.get_net_map().size());
}

void STAInterface::wrapDatabase(idb::IdbDesign* idb_design, Database& database)
{
  database.set_design_name(idb_design->get_design_name());

  wrapInstanceList(idb_design, database);
  wrapPortList(idb_design, database);
  wrapNetList(idb_design, database);
}

void STAInterface::wrapInstanceList(idb::IdbDesign* idb_design, Database& database)
{
  if (idb_design->get_instance_list() == nullptr) {
    return;
  }

  for (idb::IdbInstance* idb_instance : idb_design->get_instance_list()->get_instance_list()) {
    if (idb_instance == nullptr) {
      continue;
    }

    Instance instance;
    instance.set_name(idb_instance->get_name());
    if (idb_instance->get_cell_master() != nullptr) {
      instance.set_cell_name(idb_instance->get_cell_master()->get_name());
    }
    database.get_instance_map()[instance.get_name()] = instance;

    if (idb_instance->get_pin_list() == nullptr) {
      continue;
    }
    for (idb::IdbPin* idb_pin : idb_instance->get_pin_list()->get_pin_list()) {
      wrapInstancePin(idb_instance, idb_pin, database);
    }
  }
}

void STAInterface::wrapInstancePin(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin, Database& database)
{
  if (idb_instance == nullptr || idb_pin == nullptr || idb_pin->get_term() == nullptr) {
    return;
  }

  const std::string full_name = wrapInstancePinName(idb_instance, idb_pin);
  Pin pin;
  pin.set_name(idb_pin->get_term_name());
  pin.set_full_name(full_name);
  pin.set_instance_name(idb_instance->get_name());
  pin.set_direction(wrapPinDirection(idb_pin->get_term()->get_direction()));

  if (auto* coordinate = idb_pin->get_average_coordinate(); coordinate != nullptr) {
    pin.set_x(coordinate->get_x());
    pin.set_y(coordinate->get_y());
  }

  database.get_pin_map()[full_name] = pin;
}

std::string STAInterface::wrapInstancePinName(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin) const
{
  if (idb_instance == nullptr || idb_pin == nullptr) {
    return "";
  }
  return idb_instance->get_name() + ":" + idb_pin->get_term_name();
}

PinDirection STAInterface::wrapPinDirection(const idb::IdbConnectDirection& idb_direction) const
{
  switch (idb_direction) {
    case idb::IdbConnectDirection::kInput:
      return PinDirection::kInput;
    case idb::IdbConnectDirection::kOutput:
    case idb::IdbConnectDirection::kOutputTriState:
      return PinDirection::kOutput;
    case idb::IdbConnectDirection::kInOut:
      return PinDirection::kInout;
    default:
      return PinDirection::kNone;
  }
}

void STAInterface::wrapUniqueName(std::vector<std::string>& list, const std::string& value)
{
  if (value.empty()) {
    return;
  }
  if (std::find(list.begin(), list.end(), value) == list.end()) {
    list.push_back(value);
  }
}

void STAInterface::wrapPortList(idb::IdbDesign* idb_design, Database& database)
{
  if (idb_design->get_io_pin_list() == nullptr) {
    return;
  }

  for (idb::IdbPin* idb_pin : idb_design->get_io_pin_list()->get_pin_list()) {
    wrapPortPin(idb_pin, database);
  }
}

void STAInterface::wrapPortPin(idb::IdbPin* idb_pin, Database& database)
{
  if (idb_pin == nullptr || idb_pin->get_term() == nullptr) {
    return;
  }

  const std::string full_name = wrapPinName(idb_pin);
  Pin pin;
  pin.set_name(idb_pin->get_pin_name());
  pin.set_full_name(full_name);
  pin.set_direction(wrapPinDirection(idb_pin->get_term()->get_direction()));
  pin.set_is_port(true);

  if (auto* coordinate = idb_pin->get_average_coordinate(); coordinate != nullptr) {
    pin.set_x(coordinate->get_x());
    pin.set_y(coordinate->get_y());
  }

  database.get_pin_map()[full_name] = pin;
}

std::string STAInterface::wrapPinName(idb::IdbPin* idb_pin) const
{
  if (idb_pin == nullptr) {
    return "";
  }
  if (!idb_pin->get_pin_name().empty()) {
    return idb_pin->get_pin_name();
  }
  return idb_pin->get_term_name();
}

void STAInterface::wrapNetList(idb::IdbDesign* idb_design, Database& database)
{
  if (idb_design->get_net_list() != nullptr) {
    for (idb::IdbNet* idb_net : idb_design->get_net_list()->get_net_list()) {
      wrapNet(idb_net, database);
    }
  }

  if (idb_design->get_special_net_list() != nullptr) {
    for (idb::IdbSpecialNet* idb_net : idb_design->get_special_net_list()->get_net_list()) {
      wrapSpecialNet(idb_net, database);
    }
  }
}

void STAInterface::wrapNet(idb::IdbNet* idb_net, Database& database)
{
  if (idb_net == nullptr || !wrapSignalNet(idb_net->get_connect_type())) {
    return;
  }

  Net net;
  net.set_name(idb_net->get_net_name());
  wrapNetPinList(idb_net, database, net);
  if (!net.get_name().empty() && net.get_pin_name_list().size() >= 2) {
    database.get_net_map()[net.get_name()] = net;
  }
}

bool STAInterface::wrapSignalNet(idb::IdbConnectType connect_type)
{
  return connect_type == idb::IdbConnectType::kNone || connect_type == idb::IdbConnectType::kSignal
         || connect_type == idb::IdbConnectType::kClock || connect_type == idb::IdbConnectType::kReset
         || connect_type == idb::IdbConnectType::kScan || connect_type == idb::IdbConnectType::kTieOff;
}

void STAInterface::wrapNetPinList(idb::IdbNet* idb_net, Database& database, Net& net)
{
  wrapNetPinList(idb_net->get_io_pins(), idb_net->get_instance_pin_list(), database, net);
}

void STAInterface::wrapNetPinList(idb::IdbPins* io_pin_list, idb::IdbPins* instance_pin_list, Database& database, Net& net)
{
  if (io_pin_list != nullptr) {
    for (idb::IdbPin* idb_pin : io_pin_list->get_pin_list()) {
      wrapNetPin(idb_pin, database, net);
    }
  }
  if (instance_pin_list != nullptr) {
    for (idb::IdbPin* idb_pin : instance_pin_list->get_pin_list()) {
      wrapNetPin(idb_pin, database, net);
    }
  }
}

void STAInterface::wrapNetPin(idb::IdbPin* idb_pin, Database& database, Net& net)
{
  if (idb_pin == nullptr) {
    return;
  }

  std::string full_name;
  if (idb_pin->is_io_pin() || idb_pin->get_instance() == nullptr) {
    full_name = idb_pin->get_pin_name();
    if (full_name.empty() && idb_pin->get_term() != nullptr) {
      full_name = idb_pin->get_term_name();
    }
  } else if (idb_pin->get_instance() != nullptr) {
    if (idb_pin->get_term() == nullptr) {
      return;
    }
    full_name = idb_pin->get_instance()->get_name() + ":" + idb_pin->get_term_name();
  }
  if (full_name.empty()) {
    return;
  }

  auto& pin_map = database.get_pin_map();
  auto pin_iter = pin_map.find(full_name);
  if (pin_iter == pin_map.end()) {
    return;
  }

  wrapUniqueName(net.get_pin_name_list(), full_name);
}

void STAInterface::wrapSpecialNet(idb::IdbSpecialNet* idb_net, Database& database)
{
  if (idb_net == nullptr || !wrapSignalNet(idb_net->get_connect_type())) {
    return;
  }

  Net net;
  net.set_name(idb_net->get_net_name());
  wrapNetPinList(idb_net, database, net);
  if (!net.get_name().empty() && net.get_pin_name_list().size() >= 2) {
    database.get_net_map()[net.get_name()] = net;
  }
}

void STAInterface::wrapNetPinList(idb::IdbSpecialNet* idb_net, Database& database, Net& net)
{
  wrapNetPinList(idb_net->get_io_pins(), idb_net->get_instance_pin_list(), database, net);
}

#endif

#if 1  // output

void STAInterface::output()
{
  const Database& database = STADM.getDatabase();
  const Summary& summary = database.get_summary();
  STALOG.info(Loc::current(), "Output iSTA summary: design=", database.get_design_name(),
              " instances=", summary.get_instance_num(), " ports=", summary.get_port_num(), " pins=", summary.get_pin_num(),
              " nets=", summary.get_net_num(), " arcs=", summary.get_arc_num(), " worst_slack=", summary.get_worst_slack(),
              " worst_endpoint=", summary.get_worst_endpoint());
}

#endif

#endif

#endif

// private

STAInterface* STAInterface::_sta_interface_instance = nullptr;

}  // namespace ista
