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
#include "STAHeader.hpp"
#include "TimingAnalyzer.hpp"
#include "TimingModel.hpp"
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

bool STAInterface::inputIDB(idb::IdbDesign* idb_design)
{
  if (idb_design == nullptr) {
    STALOG.warn(Loc::current(), "IDB design is null, skip iSTA input.");
    return false;
  }

  TimingModel& timing_model = STADM.getTimingModel();
  timing_model.reset();
  timing_model.design_name = idb_design->get_design_name();

  inputInstances(idb_design, timing_model);
  inputPorts(idb_design, timing_model);
  inputNets(idb_design, timing_model);

  STALOG.info(Loc::current(), "Input IDB to iSTA model: instances=", timing_model.instances.size(),
              " ports=", timing_model.summary.port_num, " pins=", timing_model.pins.size(), " nets=", timing_model.nets.size());
  return true;
}

void STAInterface::inputInstances(idb::IdbDesign* idb_design, TimingModel& timing_model)
{
  if (idb_design->get_instance_list() == nullptr) {
    return;
  }

  for (idb::IdbInstance* idb_instance : idb_design->get_instance_list()->get_instance_list()) {
    if (idb_instance == nullptr) {
      continue;
    }

    Instance instance;
    instance.name = idb_instance->get_name();
    if (idb_instance->get_cell_master() != nullptr) {
      instance.cell_name = idb_instance->get_cell_master()->get_name();
    }
    timing_model.instances[instance.name] = instance;

    if (idb_instance->get_pin_list() == nullptr) {
      continue;
    }
    for (idb::IdbPin* idb_pin : idb_instance->get_pin_list()->get_pin_list()) {
      addInstancePin(idb_instance, idb_pin, timing_model);
    }
  }
}

void STAInterface::addInstancePin(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin, TimingModel& timing_model)
{
  if (idb_instance == nullptr || idb_pin == nullptr || idb_pin->get_term() == nullptr) {
    return;
  }

  const std::string full_name = makeInstancePinName(idb_instance, idb_pin);
  Pin pin;
  pin.name = idb_pin->get_term_name();
  pin.full_name = full_name;
  pin.instance_name = idb_instance->get_name();
  pin.direction = convertDirection(idb_pin->get_term()->get_direction());

  if (auto* coordinate = idb_pin->get_average_coordinate(); coordinate != nullptr) {
    pin.x = coordinate->get_x();
    pin.y = coordinate->get_y();
  }

  timing_model.pins[full_name] = pin;
  auto instance_iter = timing_model.instances.find(pin.instance_name);
  if (instance_iter != timing_model.instances.end()) {
    appendUnique(instance_iter->second.pin_names, full_name);
  }
}

std::string STAInterface::makeInstancePinName(idb::IdbInstance* idb_instance, idb::IdbPin* idb_pin) const
{
  if (idb_instance == nullptr || idb_pin == nullptr) {
    return "";
  }
  return idb_instance->get_name() + ":" + idb_pin->get_term_name();
}

PinDirection STAInterface::convertDirection(const idb::IdbConnectDirection& idb_direction) const
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

void STAInterface::appendUnique(std::vector<std::string>& list, const std::string& value)
{
  if (value.empty()) {
    return;
  }
  if (std::find(list.begin(), list.end(), value) == list.end()) {
    list.push_back(value);
  }
}

void STAInterface::inputPorts(idb::IdbDesign* idb_design, TimingModel& timing_model)
{
  if (idb_design->get_io_pin_list() == nullptr) {
    return;
  }

  for (idb::IdbPin* idb_pin : idb_design->get_io_pin_list()->get_pin_list()) {
    addPortPin(idb_pin, timing_model);
  }
  timing_model.summary.port_num = 0;
  for (const auto& [pin_name, pin] : timing_model.pins) {
    if (pin.is_port) {
      ++timing_model.summary.port_num;
    }
  }
}

void STAInterface::addPortPin(idb::IdbPin* idb_pin, TimingModel& timing_model)
{
  if (idb_pin == nullptr || idb_pin->get_term() == nullptr) {
    return;
  }

  const std::string full_name = makePinName(idb_pin);
  Pin pin;
  pin.name = idb_pin->get_pin_name();
  pin.full_name = full_name;
  pin.direction = convertDirection(idb_pin->get_term()->get_direction());
  pin.is_port = true;

  if (auto* coordinate = idb_pin->get_average_coordinate(); coordinate != nullptr) {
    pin.x = coordinate->get_x();
    pin.y = coordinate->get_y();
  }

  timing_model.pins[full_name] = pin;
}

std::string STAInterface::makePinName(idb::IdbPin* idb_pin) const
{
  if (idb_pin == nullptr) {
    return "";
  }
  if (!idb_pin->get_pin_name().empty()) {
    return idb_pin->get_pin_name();
  }
  return idb_pin->get_term_name();
}

void STAInterface::inputNets(idb::IdbDesign* idb_design, TimingModel& timing_model)
{
  if (idb_design->get_net_list() != nullptr) {
    for (idb::IdbNet* idb_net : idb_design->get_net_list()->get_net_list()) {
      inputNet(idb_net, timing_model);
    }
  }

  if (idb_design->get_special_net_list() != nullptr) {
    for (idb::IdbSpecialNet* idb_net : idb_design->get_special_net_list()->get_net_list()) {
      inputSpecialNet(idb_net, timing_model);
    }
  }
}

void STAInterface::inputNet(idb::IdbNet* idb_net, TimingModel& timing_model)
{
  if (idb_net == nullptr || !isSignalNet(idb_net->get_connect_type())) {
    return;
  }

  Net net;
  net.name = idb_net->get_net_name();
  collectNetPins(idb_net, net.name, timing_model, net);
  if (!net.name.empty() && net.pin_names.size() >= 2) {
    timing_model.nets[net.name] = net;
  }
}

bool STAInterface::isSignalNet(idb::IdbConnectType connect_type)
{
  return connect_type == idb::IdbConnectType::kNone || connect_type == idb::IdbConnectType::kSignal
         || connect_type == idb::IdbConnectType::kClock || connect_type == idb::IdbConnectType::kReset
         || connect_type == idb::IdbConnectType::kScan || connect_type == idb::IdbConnectType::kTieOff;
}

void STAInterface::collectNetPins(idb::IdbNet* idb_net, const std::string& net_name, TimingModel& timing_model, Net& net)
{
  collectNetPins(idb_net->get_io_pins(), idb_net->get_instance_pin_list(), net_name, timing_model, net);
}

void STAInterface::collectNetPins(idb::IdbPins* io_pin_list, idb::IdbPins* instance_pin_list, const std::string& net_name,
                                  TimingModel& timing_model, Net& net)
{
  if (io_pin_list != nullptr) {
    for (idb::IdbPin* idb_pin : io_pin_list->get_pin_list()) {
      collectNetPin(idb_pin, net_name, timing_model, net);
    }
  }
  if (instance_pin_list != nullptr) {
    for (idb::IdbPin* idb_pin : instance_pin_list->get_pin_list()) {
      collectNetPin(idb_pin, net_name, timing_model, net);
    }
  }

  if (net.driver_pin.empty() && !net.pin_names.empty()) {
    net.driver_pin = net.pin_names.front();
  }
  for (const std::string& pin_name : net.pin_names) {
    if (pin_name != net.driver_pin) {
      appendUnique(net.load_pins, pin_name);
    }
  }
}

void STAInterface::collectNetPin(idb::IdbPin* idb_pin, const std::string& net_name, TimingModel& timing_model, Net& net)
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

  auto pin_iter = timing_model.pins.find(full_name);
  if (pin_iter == timing_model.pins.end()) {
    return;
  }

  Pin& pin = pin_iter->second;
  pin.net_name = net_name;
  appendUnique(net.pin_names, full_name);

  if (net.driver_pin.empty() && shouldDriveNet(pin)) {
    net.driver_pin = full_name;
  } else if (shouldLoadNet(pin)) {
    appendUnique(net.load_pins, full_name);
  }
}

bool STAInterface::shouldDriveNet(const Pin& pin)
{
  if (pin.is_port) {
    return pin.direction == PinDirection::kInput || pin.direction == PinDirection::kInout;
  }
  return isOutputLike(pin.direction);
}

bool STAInterface::isOutputLike(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

bool STAInterface::shouldLoadNet(const Pin& pin)
{
  if (pin.is_port) {
    return pin.direction == PinDirection::kOutput || pin.direction == PinDirection::kInout;
  }
  return isInputLike(pin.direction);
}

bool STAInterface::isInputLike(PinDirection direction)
{
  return direction == PinDirection::kInput || direction == PinDirection::kInout;
}

void STAInterface::inputSpecialNet(idb::IdbSpecialNet* idb_net, TimingModel& timing_model)
{
  if (idb_net == nullptr || !isSignalNet(idb_net->get_connect_type())) {
    return;
  }

  Net net;
  net.name = idb_net->get_net_name();
  collectNetPins(idb_net, net.name, timing_model, net);
  if (!net.name.empty() && net.pin_names.size() >= 2) {
    timing_model.nets[net.name] = net;
  }
}

void STAInterface::collectNetPins(idb::IdbSpecialNet* idb_net, const std::string& net_name, TimingModel& timing_model, Net& net)
{
  collectNetPins(idb_net->get_io_pins(), idb_net->get_instance_pin_list(), net_name, timing_model, net);
}

#endif

#if 1  // output
#endif

#endif

#endif

// private

STAInterface* STAInterface::_sta_interface_instance = nullptr;

}  // namespace ista
