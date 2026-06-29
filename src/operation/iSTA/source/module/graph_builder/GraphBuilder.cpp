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
// WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#include "GraphBuilder.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ista {

// public

void GraphBuilder::initInst()
{
  if (_gb_instance == nullptr) {
    _gb_instance = new GraphBuilder();
  }
}

GraphBuilder& GraphBuilder::getInst()
{
  if (_gb_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_gb_instance;
}

void GraphBuilder::destroyInst()
{
  if (_gb_instance != nullptr) {
    delete _gb_instance;
    _gb_instance = nullptr;
  }
}

// function

bool GraphBuilder::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  buildTimingPointList(database);
  buildCellArcs(database);
  buildInoutPinDirectionByGraph(database);
  buildNetDriverLoadList(database);
  buildNetArcs(database);
  buildStartEndPointList(database);
  breakLoopArcList(database);
  buildTimingOrder(database);
  printLoopInfo(database);

  STALOG.info(Loc::current(), "Build iSTA graph: pins=", database.get_pin_map().size(), " arcs=", database.get_arc_list().size(),
              " start_points=", database.get_start_point_list().size(), " end_points=", database.get_end_point_list().size(),
              " timing_order=", database.get_timing_order_list().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

GraphBuilder* GraphBuilder::_gb_instance = nullptr;

void GraphBuilder::buildTimingPointList(Database& database)
{
  for (std::pair<const std::string, Pin>& pin_pair : database.get_pin_map()) {
    database.get_timing_point_map()[pin_pair.first] = TimingPoint();
  }
}

void GraphBuilder::buildCellArcs(Database& database)
{
  for (auto& [instance_name, instance] : database.get_instance_map()) {
    if (buildLibraryCellArcs(database, instance)) {
      continue;
    }
    std::vector<std::string> input_pin_list = collectInputPins(database, instance);
    std::vector<std::string> output_pin_list = collectOutputPins(database, instance);
    if (input_pin_list.empty() || output_pin_list.empty()) {
      continue;
    }
    for (std::string& input_pin : input_pin_list) {
      for (std::string& output_pin : output_pin_list) {
        if (input_pin == output_pin) {
          continue;
        }
        addArc(database, input_pin, output_pin, ArcType::kCell, instance_name);
      }
    }
  }
}

bool GraphBuilder::buildLibraryCellArcs(Database& database, Instance& instance)
{
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return false;
  }

  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  if (timing_cell.get_cell_arc_list().empty()) {
    return false;
  }

  for (TimingCellArc& timing_cell_arc : timing_cell.get_cell_arc_list()) {
    addCellArc(database, instance, timing_cell_arc);
  }
  return true;
}

std::string GraphBuilder::getInstancePinName(Instance& instance, std::string& port_name)
{
  return instance.get_instance_name() + ":" + port_name;
}

void GraphBuilder::addCellArc(Database& database, Instance& instance, TimingCellArc& timing_cell_arc)
{
  std::string source_pin = getInstancePinName(instance, timing_cell_arc.get_source_port());
  std::string sink_pin = getInstancePinName(instance, timing_cell_arc.get_sink_port());
  if (database.get_pin_map().count(source_pin) == 0 || database.get_pin_map().count(sink_pin) == 0) {
    return;
  }
  bool is_disable_arc = false;
  if (timing_cell_arc.get_lib_arc_set() != nullptr && timing_cell_arc.get_lib_arc_set()->front() != nullptr) {
    is_disable_arc = timing_cell_arc.get_lib_arc_set()->front()->isDisableArc();
  }
  addArc(database, source_pin, sink_pin, ArcType::kCell, instance.get_instance_name(), timing_cell_arc.get_source_port(), timing_cell_arc.get_sink_port(),
         timing_cell_arc.get_is_clock_arc(), is_disable_arc, &timing_cell_arc);
}

void GraphBuilder::addArc(Database& database, const std::string& source_pin, const std::string& sink_pin, ArcType type, const std::string& owner_name,
                          const std::string& library_source_port, const std::string& library_sink_port, bool is_clock_arc, bool is_disable_arc,
                          TimingCellArc* timing_cell_arc)
{
  Arc arc;
  arc.set_arc_name(owner_name + ":" + source_pin + "->" + sink_pin);
  arc.set_source_pin(source_pin);
  arc.set_sink_pin(sink_pin);
  arc.set_owner_name(owner_name);
  arc.set_library_source_port(library_source_port);
  arc.set_library_sink_port(library_sink_port);
  arc.set_type(type);
  arc.set_is_clock_arc(is_clock_arc);
  arc.set_is_disable_arc(is_disable_arc);
  arc.set_timing_cell_arc(timing_cell_arc);

  database.get_arc_list().push_back(arc);
  const std::size_t arc_idx = database.get_arc_list().size() - 1;
  database.get_outgoing_arc_list_map()[source_pin].push_back(arc_idx);
  database.get_incoming_arc_list_map()[sink_pin].push_back(arc_idx);
}

std::vector<std::string> GraphBuilder::collectInputPins(Database& database, Instance& instance)
{
  std::vector<std::string> input_pin_list;
  for (std::string& pin_name : instance.get_pin_name_list()) {
    if (isInputLike(database.get_pin_map()[pin_name].get_direction())) {
      input_pin_list.push_back(pin_name);
    }
  }
  return input_pin_list;
}

bool GraphBuilder::isInputLike(PinDirection direction)
{
  return direction == PinDirection::kInput || direction == PinDirection::kInout;
}

std::vector<std::string> GraphBuilder::collectOutputPins(Database& database, Instance& instance)
{
  std::vector<std::string> output_pin_list;
  for (std::string& pin_name : instance.get_pin_name_list()) {
    if (isOutputLike(database.get_pin_map()[pin_name].get_direction())) {
      output_pin_list.push_back(pin_name);
    }
  }
  return output_pin_list;
}

bool GraphBuilder::isOutputLike(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

void GraphBuilder::buildInoutPinDirectionByGraph(Database& database)
{
  std::map<std::string, PinDirection> inout_pin_direction_map = makeInoutPinDirectionMap(database);
  for (std::pair<const std::string, PinDirection>& pin_direction_pair : inout_pin_direction_map) {
    database.get_pin_map()[pin_direction_pair.first].set_direction(pin_direction_pair.second);
  }
  rebuildCellArcListByPinDirection(database);
}

std::map<std::string, PinDirection> GraphBuilder::makeInoutPinDirectionMap(Database& database)
{
  std::map<std::string, PinDirection> inout_pin_direction_map;
  for (std::pair<const std::string, Pin>& pin_pair : database.get_pin_map()) {
    Pin& pin = pin_pair.second;
    if (pin.get_direction() != PinDirection::kInout) {
      continue;
    }
    if (isFloatingInoutPin(database, pin)) {
      continue;
    }
    PinDirection pin_direction = inferInoutPinDirection(database, pin_pair.first, pin, inout_pin_direction_map);
    if (pin_direction == PinDirection::kNone || pin_direction == PinDirection::kInout) {
      STALOG.error(Loc::current(), "Failed to infer inout pin direction: pin=", pin_pair.first, " net=", pin.get_net_name());
    }
    inout_pin_direction_map[pin_pair.first] = pin_direction;
  }
  return inout_pin_direction_map;
}

bool GraphBuilder::isFloatingInoutPin(Database& database, Pin& pin)
{
  return pin.get_net_name().empty() && inferInoutPinDirectionByTimingCell(database, pin) == PinDirection::kNone;
}

PinDirection GraphBuilder::inferInoutPinDirection(Database& database, const std::string& pin_name, Pin& pin,
                                                  std::map<std::string, PinDirection>& inout_pin_direction_map)
{
  PinDirection timing_cell_direction = inferInoutPinDirectionByTimingCell(database, pin);
  if (timing_cell_direction != PinDirection::kNone) {
    return timing_cell_direction;
  }

  PinDirection timing_graph_direction = inferInoutPinDirectionByTimingGraph(database, pin_name);
  if (timing_graph_direction != PinDirection::kNone) {
    return timing_graph_direction;
  }

  return inferInoutPinDirectionByNet(database, pin, inout_pin_direction_map);
}

PinDirection GraphBuilder::inferInoutPinDirectionByTimingCell(Database& database, Pin& pin)
{
  TimingCellPort* timing_cell_port = getTimingCellPort(database, pin);
  if (timing_cell_port == nullptr) {
    return PinDirection::kNone;
  }
  if (timing_cell_port->get_is_output() && !timing_cell_port->get_is_input()) {
    return PinDirection::kOutput;
  }
  if (timing_cell_port->get_is_input() && !timing_cell_port->get_is_output()) {
    return PinDirection::kInput;
  }
  return PinDirection::kNone;
}

TimingCellPort* GraphBuilder::getTimingCellPort(Database& database, Pin& pin)
{
  if (pin.get_is_port()) {
    return nullptr;
  }
  if (database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return nullptr;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return nullptr;
  }
  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  if (timing_cell.get_port_map().count(pin.get_pin_name()) == 0) {
    return nullptr;
  }
  return &timing_cell.get_port_map()[pin.get_pin_name()];
}

PinDirection GraphBuilder::inferInoutPinDirectionByTimingGraph(Database& database, const std::string& pin_name)
{
  const bool has_outgoing_cell_arc = hasOutgoingCellArc(database, pin_name);
  const bool has_incoming_cell_arc = hasIncomingCellArc(database, pin_name);
  if (has_outgoing_cell_arc && !has_incoming_cell_arc) {
    return PinDirection::kInput;
  }
  if (has_incoming_cell_arc && !has_outgoing_cell_arc) {
    return PinDirection::kOutput;
  }
  return PinDirection::kNone;
}

bool GraphBuilder::hasOutgoingCellArc(Database& database, const std::string& pin_name)
{
  for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
    if (database.get_arc_list()[arc_idx].get_type() == ArcType::kCell) {
      return true;
    }
  }
  return false;
}

bool GraphBuilder::hasIncomingCellArc(Database& database, const std::string& pin_name)
{
  for (std::size_t arc_idx : database.get_incoming_arc_list_map()[pin_name]) {
    if (database.get_arc_list()[arc_idx].get_type() == ArcType::kCell) {
      return true;
    }
  }
  return false;
}

PinDirection GraphBuilder::inferInoutPinDirectionByNet(Database& database, Pin& pin, std::map<std::string, PinDirection>& inout_pin_direction_map)
{
  Net* net = getPinNet(database, pin);
  if (net == nullptr) {
    return PinDirection::kNone;
  }
  int32_t driver_pin_num = getDriverPinNum(database, *net, inout_pin_direction_map);
  int32_t unresolved_inout_pin_num = getUnresolvedInoutPinNum(database, *net, inout_pin_direction_map);
  if (driver_pin_num == 0 && unresolved_inout_pin_num == 1) {
    return getDriverPinDirection(pin);
  }
  if (driver_pin_num == 1) {
    return getLoadPinDirection(pin);
  }
  if (driver_pin_num > 1) {
    std::vector<std::string> driver_pin_list = getDriverPinList(database, *net, inout_pin_direction_map);
    STALOG.warn(Loc::current(), "The net has multiple driver pins: net=", net->get_net_name(), " drivers=", getPinNameListString(driver_pin_list));
    return getLoadPinDirection(pin);
  }
  return PinDirection::kNone;
}

Net* GraphBuilder::getPinNet(Database& database, Pin& pin)
{
  if (pin.get_net_name().empty() || database.get_net_map().count(pin.get_net_name()) == 0) {
    return nullptr;
  }
  return &database.get_net_map()[pin.get_net_name()];
}

std::vector<std::string> GraphBuilder::getDriverPinList(Database& database, Net& net, std::map<std::string, PinDirection>& inout_pin_direction_map)
{
  std::vector<std::string> driver_pin_list;
  for (std::string& pin_name : net.get_pin_name_list()) {
    if (isResolvedDriverPin(database, pin_name, inout_pin_direction_map)) {
      driver_pin_list.push_back(pin_name);
    }
  }
  return driver_pin_list;
}

int32_t GraphBuilder::getDriverPinNum(Database& database, Net& net, std::map<std::string, PinDirection>& inout_pin_direction_map)
{
  return static_cast<int32_t>(getDriverPinList(database, net, inout_pin_direction_map).size());
}

int32_t GraphBuilder::getUnresolvedInoutPinNum(Database& database, Net& net, std::map<std::string, PinDirection>& inout_pin_direction_map)
{
  int32_t unresolved_inout_pin_num = 0;
  for (std::string& pin_name : net.get_pin_name_list()) {
    Pin& pin = database.get_pin_map()[pin_name];
    if (pin.get_direction() == PinDirection::kInout && inout_pin_direction_map.count(pin_name) == 0) {
      unresolved_inout_pin_num++;
    }
  }
  return unresolved_inout_pin_num;
}

bool GraphBuilder::isResolvedDriverPin(Database& database, const std::string& pin_name, std::map<std::string, PinDirection>& inout_pin_direction_map)
{
  Pin& pin = database.get_pin_map()[pin_name];
  PinDirection direction = pin.get_direction();
  if (direction == PinDirection::kInout && inout_pin_direction_map.count(pin_name) > 0) {
    direction = inout_pin_direction_map[pin_name];
  }
  return isDriverDirection(pin, direction);
}

bool GraphBuilder::isDriverDirection(Pin& pin, PinDirection direction)
{
  if (pin.get_is_port()) {
    return direction == PinDirection::kInput;
  }
  return direction == PinDirection::kOutput;
}

PinDirection GraphBuilder::getDriverPinDirection(Pin& pin)
{
  if (pin.get_is_port()) {
    return PinDirection::kInput;
  }
  return PinDirection::kOutput;
}

PinDirection GraphBuilder::getLoadPinDirection(Pin& pin)
{
  if (pin.get_is_port()) {
    return PinDirection::kOutput;
  }
  return PinDirection::kInput;
}

void GraphBuilder::rebuildCellArcListByPinDirection(Database& database)
{
  database.get_arc_list().clear();
  database.get_outgoing_arc_list_map().clear();
  database.get_incoming_arc_list_map().clear();
  buildCellArcs(database);
}

void GraphBuilder::buildNetDriverLoadList(Database& database)
{
  for (std::pair<const std::string, Net>& net_pair : database.get_net_map()) {
    makeNetDriverLoad(database, net_pair.second);
  }
}

void GraphBuilder::makeNetDriverLoad(Database& database, Net& net)
{
  net.get_driver_pin().clear();
  net.get_driver_pin_list().clear();
  net.get_load_pin_list().clear();

  for (std::string& pin_name : net.get_pin_name_list()) {
    Pin& pin = database.get_pin_map()[pin_name];
    if (isDriverPin(pin)) {
      net.set_driver_pin(pin_name);
      net.get_driver_pin_list().push_back(pin_name);
    } else {
      net.get_load_pin_list().push_back(pin_name);
    }
  }

  if (net.get_driver_pin_list().size() > 1) {
    std::vector<std::string> driver_pin_list = net.get_driver_pin_list();
    std::string selected_driver_pin = selectDriverPin(driver_pin_list);
    STALOG.warn(Loc::current(), "The net has multiple driver pins: net=", net.get_net_name(), " drivers=", getPinNameListString(driver_pin_list),
                " selected_driver=", selected_driver_pin);

    net.set_driver_pin(selected_driver_pin);
    net.get_driver_pin_list().clear();
    net.get_driver_pin_list().push_back(selected_driver_pin);
    net.get_load_pin_list().clear();
    for (std::string& pin_name : net.get_pin_name_list()) {
      if (pin_name != selected_driver_pin) {
        net.get_load_pin_list().push_back(pin_name);
      }
    }
  }
}

std::string GraphBuilder::selectDriverPin(std::vector<std::string>& driver_pin_list)
{
  if (driver_pin_list.empty()) {
    return "";
  }
  static std::random_device random_device;
  static std::mt19937 generator(random_device());
  std::uniform_int_distribution<std::size_t> distribution(0, driver_pin_list.size() - 1);
  return driver_pin_list[distribution(generator)];
}

bool GraphBuilder::isDriverPin(Pin& pin)
{
  return isDriverDirection(pin, pin.get_direction());
}

std::string GraphBuilder::getPinNameListString(std::vector<std::string>& pin_name_list)
{
  std::string pin_name_list_string;
  for (std::string& pin_name : pin_name_list) {
    if (!pin_name_list_string.empty()) {
      pin_name_list_string += ", ";
    }
    pin_name_list_string += pin_name;
  }
  return pin_name_list_string;
}

void GraphBuilder::buildNetArcs(Database& database)
{
  for (auto& [net_name, net] : database.get_net_map()) {
    if (net.get_driver_pin().empty()) {
      continue;
    }
    for (std::string& load_pin : net.get_load_pin_list()) {
      if (load_pin == net.get_driver_pin()) {
        continue;
      }
      addArc(database, net.get_driver_pin(), load_pin, ArcType::kNet, net_name);
    }
  }
}

void GraphBuilder::addArc(Database& database, const std::string& source_pin, const std::string& sink_pin, ArcType type, const std::string& owner_name)
{
  Arc arc;
  arc.set_arc_name(owner_name + ":" + source_pin + "->" + sink_pin);
  arc.set_source_pin(source_pin);
  arc.set_sink_pin(sink_pin);
  arc.set_owner_name(owner_name);
  arc.set_type(type);
  arc.set_is_disable_arc(shouldDisableNetArc(database, source_pin, sink_pin));

  database.get_arc_list().push_back(arc);
  const std::size_t arc_idx = database.get_arc_list().size() - 1;
  database.get_outgoing_arc_list_map()[source_pin].push_back(arc_idx);
  database.get_incoming_arc_list_map()[sink_pin].push_back(arc_idx);
}

bool GraphBuilder::shouldDisableNetArc(Database& database, const std::string& source_pin, const std::string& sink_pin)
{
  Pin& source_pin_inst = database.get_pin_map()[source_pin];
  Pin& sink_pin_inst = database.get_pin_map()[sink_pin];
  if (source_pin_inst.get_is_port() || sink_pin_inst.get_is_port()) {
    return false;
  }
  if (database.get_instance_map().count(source_pin_inst.get_instance_name()) == 0
      || database.get_instance_map().count(sink_pin_inst.get_instance_name()) == 0) {
    return false;
  }
  Instance& source_instance = database.get_instance_map()[source_pin_inst.get_instance_name()];
  Instance& sink_instance = database.get_instance_map()[sink_pin_inst.get_instance_name()];
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(source_instance.get_cell_name()) == 0) {
    return false;
  }
  TimingCell& source_cell = timing_cell_map[source_instance.get_cell_name()];
  return source_cell.get_is_sequential() && !source_cell.get_is_clock_gating() && !source_cell.get_is_macro()
         && sink_pin == sink_instance.get_clock_pin_name();
}

bool GraphBuilder::isDisableArc(Arc& arc)
{
  return arc.get_is_disable_arc() || arc.get_is_loop_disable();
}

void GraphBuilder::buildStartEndPointList(Database& database)
{
  for (auto& [pin_name, pin] : database.get_pin_map()) {
    if (isStartPoint(database, pin_name, pin)) {
      appendUnique(database.get_start_point_list(), pin_name);
    }
    if (isEndPoint(database, pin_name, pin)) {
      appendUnique(database.get_end_point_list(), pin_name);
    }
  }
}

bool GraphBuilder::isStartPoint(Database& database, const std::string& pin_name, Pin& pin)
{
  if (isRegisterClockStartPoint(database, pin_name, pin)) {
    return true;
  }
  if (isClockPin(database, pin_name, pin)) {
    return false;
  }
  if (isClockSource(database, pin_name)) {
    return isStartPort(pin);
  }
  return !hasIncomingArc(database, pin_name) || isStartPort(pin);
}

bool GraphBuilder::isRegisterClockStartPoint(Database& database, const std::string& pin_name, Pin& pin)
{
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_clock_pin_name();
}

bool GraphBuilder::isClockPin(Database& database, const std::string& pin_name, Pin& pin)
{
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  return pin_name == database.get_instance_map()[pin.get_instance_name()].get_clock_pin_name();
}

bool GraphBuilder::isClockSource(Database& database, const std::string& pin_name)
{
  for (auto& [clock_name, timing_clock] : database.get_timing_constraint().get_clock_map()) {
    if (STAUTIL.exist(timing_clock.get_source_list(), pin_name)) {
      return true;
    }
  }
  return false;
}

bool GraphBuilder::hasIncomingArc(Database& database, const std::string& pin_name)
{
  return database.get_incoming_arc_list_map().count(pin_name) > 0 && !database.get_incoming_arc_list_map()[pin_name].empty();
}

bool GraphBuilder::isStartPort(Pin& pin)
{
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout);
}

bool GraphBuilder::isEndPoint(Database& database, const std::string& pin_name, Pin& pin)
{
  if (isClockPin(database, pin_name, pin) || isClockSource(database, pin_name)) {
    return false;
  }
  return !hasOutgoingArc(database, pin_name) || isEndPort(pin) || isTimingCheckEndPoint(database, pin_name, pin);
}

bool GraphBuilder::isTimingCheckEndPoint(Database& database, const std::string& pin_name, Pin& pin)
{
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  for (TimingCheckArc& timing_check_arc : instance.get_check_arc_list()) {
    if (timing_check_arc.get_data_port() == pin_name) {
      return true;
    }
  }
  return false;
}

bool GraphBuilder::hasOutgoingArc(Database& database, const std::string& pin_name)
{
  return database.get_outgoing_arc_list_map().count(pin_name) > 0 && !database.get_outgoing_arc_list_map()[pin_name].empty();
}

bool GraphBuilder::isEndPort(Pin& pin)
{
  return pin.get_is_port() && (pin.get_direction() == PinDirection::kOutput || pin.get_direction() == PinDirection::kInout);
}

void GraphBuilder::appendUnique(std::vector<std::string>& list, const std::string& value)
{
  if (!STAUTIL.exist(list, value)) {
    list.push_back(value);
  }
}

void GraphBuilder::breakLoopArcList(Database& database)
{
  std::size_t disabled_loop_num = breakLoopArcFromStart(database);
  disabled_loop_num += breakLoopArcFromEnd(database);
  disabled_loop_num += breakLoopArcFromFloating(database);
  if (disabled_loop_num > 0) {
    STALOG.info(Loc::current(), "Break iSTA loop arcs: disabled_arcs=", disabled_loop_num);
  }
}

std::size_t GraphBuilder::breakLoopArcFromStart(Database& database)
{
  std::size_t disabled_loop_num = 0;
  std::map<std::string, int32_t> color_map;
  for (std::string& start_point : database.get_start_point_list()) {
    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[start_point]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      if (isDisableArc(arc)) {
        continue;
      }
      traverseDataPath(database, arc.get_sink_pin(), true, color_map, disabled_loop_num);
    }
  }
  return disabled_loop_num;
}

bool GraphBuilder::traverseDataPath(Database& database, std::string& pin_name, bool is_forward, std::map<std::string, int32_t>& color_map,
                                    std::size_t& disabled_loop_num)
{
  if (stopTraverse(database, pin_name, is_forward) || isBlack(color_map, pin_name)) {
    return false;
  }
  if (isGray(color_map, pin_name)) {
    return true;
  }

  color_map[pin_name] = 1;
  std::vector<std::size_t>& arc_idx_list
      = is_forward ? database.get_outgoing_arc_list_map()[pin_name] : database.get_incoming_arc_list_map()[pin_name];
  for (std::size_t arc_idx : arc_idx_list) {
    Arc& arc = database.get_arc_list()[arc_idx];
    if (isDisableArc(arc)) {
      continue;
    }

    std::string& next_pin_name = is_forward ? arc.get_sink_pin() : arc.get_source_pin();
    if (isBlack(color_map, next_pin_name)) {
      continue;
    }
    if (isGray(color_map, next_pin_name)) {
      if (disableLoopArc(arc)) {
        ++disabled_loop_num;
      }
      continue;
    }
    if (traverseDataPath(database, next_pin_name, is_forward, color_map, disabled_loop_num)) {
      if (disableLoopArc(arc)) {
        ++disabled_loop_num;
      }
      continue;
    }
  }
  color_map[pin_name] = 2;
  return false;
}

bool GraphBuilder::stopTraverse(Database& database, std::string& pin_name, bool is_forward)
{
  if (is_forward) {
    return STAUTIL.exist(database.get_end_point_list(), pin_name);
  }
  return STAUTIL.exist(database.get_start_point_list(), pin_name);
}

bool GraphBuilder::isBlack(std::map<std::string, int32_t>& color_map, std::string& pin_name)
{
  return color_map.count(pin_name) > 0 && color_map[pin_name] == 2;
}

bool GraphBuilder::isGray(std::map<std::string, int32_t>& color_map, std::string& pin_name)
{
  return color_map.count(pin_name) > 0 && color_map[pin_name] == 1;
}

bool GraphBuilder::disableLoopArc(Arc& arc)
{
  if (arc.get_is_loop_disable()) {
    return false;
  }
  arc.set_is_loop_disable(true);
  return true;
}

std::size_t GraphBuilder::breakLoopArcFromEnd(Database& database)
{
  std::size_t disabled_loop_num = 0;
  std::map<std::string, int32_t> color_map;
  for (std::string& end_point : database.get_end_point_list()) {
    for (std::size_t arc_idx : database.get_incoming_arc_list_map()[end_point]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      if (isDisableArc(arc)) {
        continue;
      }
      traverseDataPath(database, arc.get_source_pin(), false, color_map, disabled_loop_num);
    }
  }
  return disabled_loop_num;
}

std::size_t GraphBuilder::breakLoopArcFromFloating(Database& database)
{
  std::size_t disabled_loop_num = 0;
  std::map<std::string, int32_t> color_map;
  for (std::pair<const std::string, TimingPoint>& timing_pair : database.get_timing_point_map()) {
    std::string pin_name = timing_pair.first;
    traverseFloatingDataPath(database, pin_name, color_map, disabled_loop_num);
  }
  return disabled_loop_num;
}

void GraphBuilder::traverseFloatingDataPath(Database& database, std::string& pin_name, std::map<std::string, int32_t>& color_map,
                                            std::size_t& disabled_loop_num)
{
  if (isBlack(color_map, pin_name)) {
    return;
  }
  (void) traverseDataPath(database, pin_name, true, color_map, disabled_loop_num);
}

void GraphBuilder::buildTimingOrder(Database& database)
{
  std::map<std::string, std::size_t> indegree_map = makeIndegreeMap(database);
  std::queue<std::string> pin_queue;
  pushRootPinList(database, indegree_map, pin_queue);

  database.get_timing_order_list().clear();
  while (!pin_queue.empty()) {
    std::string pin_name = pin_queue.front();
    pin_queue.pop();
    database.get_timing_order_list().push_back(pin_name);

    for (std::size_t arc_idx : database.get_outgoing_arc_list_map()[pin_name]) {
      Arc& arc = database.get_arc_list()[arc_idx];
      if (isDisableArc(arc)) {
        continue;
      }
      updateSinkLevel(database, arc);
      updateSinkIndegree(arc, indegree_map, pin_queue);
    }
  }
}

std::map<std::string, std::size_t> GraphBuilder::makeIndegreeMap(Database& database)
{
  std::map<std::string, std::size_t> indegree_map;
  for (std::pair<const std::string, TimingPoint>& timing_pair : database.get_timing_point_map()) {
    timing_pair.second.set_level(0);
    std::size_t indegree = 0;
    for (std::size_t arc_idx : database.get_incoming_arc_list_map()[timing_pair.first]) {
      if (!isDisableArc(database.get_arc_list()[arc_idx])) {
        ++indegree;
      }
    }
    indegree_map[timing_pair.first] = indegree;
  }
  return indegree_map;
}

void GraphBuilder::pushRootPinList(Database& database, std::map<std::string, std::size_t>& indegree_map, std::queue<std::string>& pin_queue)
{
  for (std::pair<const std::string, TimingPoint>& timing_pair : database.get_timing_point_map()) {
    if (indegree_map[timing_pair.first] == 0) {
      database.get_timing_point_map()[timing_pair.first].set_level(1);
      pin_queue.push(timing_pair.first);
    }
  }
}

void GraphBuilder::updateSinkLevel(Database& database, Arc& arc)
{
  TimingPoint& source_point = database.get_timing_point_map()[arc.get_source_pin()];
  TimingPoint& sink_point = database.get_timing_point_map()[arc.get_sink_pin()];
  sink_point.set_level(std::max(sink_point.get_level(), source_point.get_level() + 1));
}

void GraphBuilder::updateSinkIndegree(Arc& arc, std::map<std::string, std::size_t>& indegree_map, std::queue<std::string>& pin_queue)
{
  if (indegree_map[arc.get_sink_pin()] > 0) {
    --indegree_map[arc.get_sink_pin()];
  }
  if (indegree_map[arc.get_sink_pin()] == 0) {
    pin_queue.push(arc.get_sink_pin());
  }
}

void GraphBuilder::printLoopInfo(Database& database)
{
  std::size_t loop_pin_num = database.get_timing_point_map().size() - database.get_timing_order_list().size();
  if (loop_pin_num > 0) {
    STALOG.warn(Loc::current(), "Detected ", loop_pin_num, " vertex(es) in combinational loop or unresolved dependency.");
  }
}

}  // namespace ista
