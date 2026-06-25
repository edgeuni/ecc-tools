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
#include "DesignLoader.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"
#include "idm.h"
#include "liberty/Lib.hh"
#include "spef/SpefParser.hh"

namespace ista {

// public

void DesignLoader::initInst()
{
  if (_dl_instance == nullptr) {
    _dl_instance = new DesignLoader();
  }
}

DesignLoader& DesignLoader::getInst()
{
  if (_dl_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_dl_instance;
}

void DesignLoader::destroyInst()
{
  if (_dl_instance != nullptr) {
    delete _dl_instance;
    _dl_instance = nullptr;
  }
}

// function

bool DesignLoader::build()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  buildDesign(database);
  readSdc(database);

  STALOG.info(Loc::current(), "Load iSTA design: timing_cells=", database.get_timing_library().get_cell_map().size(),
              " parasitic_nets=", database.get_parasitic_library().get_net_map().size(), " clocks=", database.get_timing_constraint().get_clock_map().size(),
              " port_constraints=", database.get_timing_constraint().get_port_constraint_map().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

DesignLoader* DesignLoader::_dl_instance = nullptr;

void DesignLoader::buildDesign(Database& database)
{
  buildTimingLibrary(database);
  buildInstanceList(database);
  buildNetList(database);
  buildInstanceTimingInfo(database);
  buildParasiticLibrary(database);
}

void DesignLoader::buildTimingLibrary(Database& database)
{
  database.get_timing_library().get_lib_list().clear();
  for (idb::LibertyReader& liberty_reader : dmInst->get_lib_readers()) {
    liberty_reader.linkLib();
    idb::LibBuilder* lib_builder = liberty_reader.get_library_builder();
    database.get_timing_library().get_lib_list().push_back(lib_builder->takeLib());
    delete lib_builder;
    liberty_reader.set_library_builder(nullptr);
  }
  buildTimingCellMap(database);
}

void DesignLoader::buildTimingCellMap(Database& database)
{
  database.get_timing_library().get_cell_map().clear();
  for (std::unique_ptr<idb::LibLibrary>& lib : database.get_timing_library().get_lib_list()) {
    for (std::unique_ptr<idb::LibCell>& lib_cell : lib->get_cells()) {
      makeTimingCell(database, lib_cell.get());
    }
  }
}

void DesignLoader::makeTimingCell(Database& database, idb::LibCell* lib_cell)
{
  TimingCell timing_cell;
  timing_cell.set_cell_name(lib_cell->get_cell_name());
  timing_cell.set_is_sequential(lib_cell->isSequentialCell());
  timing_cell.set_is_clock_gating(lib_cell->isICG());
  timing_cell.set_is_macro(lib_cell->isMacroCell());

  for (std::unique_ptr<idb::LibPort>& lib_port : lib_cell->get_cell_ports()) {
    makeTimingCellPort(timing_cell, lib_port.get());
  }

  for (std::unique_ptr<idb::LibArcSet>& lib_arc_set : lib_cell->get_cell_arcs()) {
    makeTimingCellArc(timing_cell, lib_arc_set.get());
  }

  updateTimingCell(timing_cell);
  database.get_timing_library().get_cell_map()[timing_cell.get_cell_name()] = timing_cell;
}

void DesignLoader::makeTimingCellPort(TimingCell& timing_cell, idb::LibPort* lib_port)
{
  TimingCellPort timing_cell_port;
  timing_cell_port.set_port_name(lib_port->get_port_name());
  timing_cell_port.set_capacitance(lib_port->get_port_cap());
  for (idb::AnalysisMode analysis_mode : {idb::AnalysisMode::kMax, idb::AnalysisMode::kMin}) {
    for (idb::TransType trans_type : {idb::TransType::kRise, idb::TransType::kFall}) {
      std::optional<double> port_cap = lib_port->get_port_cap(analysis_mode, trans_type);
      if (port_cap) {
        timing_cell_port.get_trans_capacitance_map()[getAnalysisType(analysis_mode)][getTransType(trans_type)] = *port_cap;
      }
    }
  }
  timing_cell_port.set_is_input(lib_port->isInput());
  timing_cell_port.set_is_output(lib_port->isOutput());
  timing_cell_port.set_is_clock(lib_port->isClock() || lib_port->get_is_clock_pin() || lib_port->get_is_clock());
  timing_cell.get_port_map()[timing_cell_port.get_port_name()] = timing_cell_port;
}

void DesignLoader::makeTimingCellArc(TimingCell& timing_cell, idb::LibArcSet* lib_arc_set)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  if (lib_arc->isDelayArc()) {
    timing_cell.get_cell_arc_list().push_back(makeDelayArc(lib_arc_set));
  } else if (lib_arc->isCheckArc()) {
    TimingCheckArc timing_check_arc = makeCheckArc(lib_arc_set);
    timing_cell.get_check_arc_list().push_back(timing_check_arc);
    if (timing_check_arc.get_check_type() == TimingCheckType::kSetup) {
      timing_cell.get_setup_arc_list().push_back(timing_check_arc);
    }
  }
}

TimingCellArc DesignLoader::makeDelayArc(idb::LibArcSet* lib_arc_set)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  TimingCellArc timing_cell_arc;
  timing_cell_arc.set_source_port(lib_arc->get_src_port());
  timing_cell_arc.set_sink_port(lib_arc->get_snk_port());
  timing_cell_arc.set_delay(lib_arc->getDelayOrConstrainCheckNs(idb::TransType::kRise, 0.0, 0.0));
  timing_cell_arc.set_delay_max(timing_cell_arc.get_delay());
  timing_cell_arc.set_delay_min(timing_cell_arc.get_delay());
  timing_cell_arc.set_lib_arc_set(lib_arc_set);
  timing_cell_arc.set_is_clock_arc(lib_arc->isRisingTriggerArc() || lib_arc->isFallingTriggerArc());
  return timing_cell_arc;
}

TimingCheckArc DesignLoader::makeCheckArc(idb::LibArcSet* lib_arc_set)
{
  idb::LibArc* lib_arc = lib_arc_set->front();
  TimingCheckArc timing_check_arc;
  timing_check_arc.set_clock_port(lib_arc->get_src_port());
  timing_check_arc.set_data_port(lib_arc->get_snk_port());
  timing_check_arc.set_check_type(getTimingCheckType(lib_arc));
  timing_check_arc.set_check_time(lib_arc->getDelayOrConstrainCheckNs(idb::TransType::kRise, 0.0, 0.0));
  timing_check_arc.set_lib_arc(lib_arc);
  timing_check_arc.set_lib_arc_set(lib_arc_set);
  if (timing_check_arc.get_check_type() == TimingCheckType::kSetup) {
    timing_check_arc.set_setup_time(timing_check_arc.get_check_time());
  }
  return timing_check_arc;
}

TimingCheckType DesignLoader::getTimingCheckType(idb::LibArc* lib_arc)
{
  if (lib_arc->isSetupArc()) {
    return TimingCheckType::kSetup;
  }
  if (lib_arc->isHoldArc()) {
    return TimingCheckType::kHold;
  }
  if (lib_arc->isRecoveryArc()) {
    return TimingCheckType::kRecovery;
  }
  if (lib_arc->isRemovalArc()) {
    return TimingCheckType::kRemoval;
  }
  return TimingCheckType::kNone;
}

AnalysisType DesignLoader::getAnalysisType(idb::AnalysisMode analysis_mode)
{
  if (analysis_mode == idb::AnalysisMode::kMin) {
    return AnalysisType::kMin;
  }
  return AnalysisType::kMax;
}

TransType DesignLoader::getTransType(idb::TransType trans_type)
{
  if (trans_type == idb::TransType::kFall) {
    return TransType::kFall;
  }
  return TransType::kRise;
}

void DesignLoader::updateTimingCell(TimingCell& timing_cell)
{
  if (!timing_cell.get_check_arc_list().empty()) {
    timing_cell.set_is_sequential(true);
  }
}

void DesignLoader::buildInstanceList(Database& database)
{
  makeInstanceList(database);
}

void DesignLoader::makeInstanceList(Database& database)
{
  for (std::pair<const std::string, Instance>& instance_pair : database.get_instance_map()) {
    instance_pair.second.get_pin_name_list().clear();
  }

  for (std::pair<const std::string, Pin>& pin_pair : database.get_pin_map()) {
    Pin& pin = pin_pair.second;
    if (!isInstancePin(pin)) {
      continue;
    }

    makeUniqueName(database.get_instance_map()[pin.get_instance_name()].get_pin_name_list(), pin_pair.first);
  }
}

void DesignLoader::buildInstanceTimingInfo(Database& database)
{
  for (std::pair<const std::string, Instance>& instance_pair : database.get_instance_map()) {
    makeInstanceTimingInfo(database, instance_pair.second);
  }
}

void DesignLoader::makeInstanceTimingInfo(Database& database, Instance& instance)
{
  std::map<std::string, TimingCell>& timing_cell_map = database.get_timing_library().get_cell_map();
  if (timing_cell_map.count(instance.get_cell_name()) == 0) {
    return;
  }

  TimingCell& timing_cell = timing_cell_map[instance.get_cell_name()];
  instance.set_is_sequential(timing_cell.get_is_sequential());
  TimingCellArc* clock_to_q_arc = findClockToQArc(timing_cell);
  if (clock_to_q_arc != nullptr) {
    instance.set_output_pin_name(getInstancePinName(instance, clock_to_q_arc->get_sink_port()));
    instance.set_clock_to_q_delay(clock_to_q_arc->get_delay());
    instance.set_clock_to_q_arc(*clock_to_q_arc);
  } else {
    instance.set_output_pin_name(findOutputPinName(instance, timing_cell));
  }
  if (timing_cell.get_setup_arc_list().empty()) {
    return;
  }

  TimingCheckArc& setup_arc = timing_cell.get_setup_arc_list().front();
  instance.set_clock_pin_name(getInstancePinName(instance, setup_arc.get_clock_port()));
  instance.set_data_pin_name(getInstancePinName(instance, setup_arc.get_data_port()));
  instance.set_setup_time(setup_arc.get_setup_time());
  instance.get_check_arc_list().clear();
  for (TimingCheckArc& timing_check_arc : timing_cell.get_check_arc_list()) {
    instance.get_check_arc_list().push_back(makeInstanceTimingCheckArc(instance, timing_check_arc));
  }
}

TimingCheckArc DesignLoader::makeInstanceTimingCheckArc(Instance& instance, TimingCheckArc& timing_check_arc)
{
  TimingCheckArc instance_timing_check_arc;
  instance_timing_check_arc.set_clock_port(getInstancePinName(instance, timing_check_arc.get_clock_port()));
  instance_timing_check_arc.set_data_port(getInstancePinName(instance, timing_check_arc.get_data_port()));
  instance_timing_check_arc.set_setup_time(timing_check_arc.get_setup_time());
  instance_timing_check_arc.set_check_type(timing_check_arc.get_check_type());
  instance_timing_check_arc.set_check_time(timing_check_arc.get_check_time());
  instance_timing_check_arc.set_lib_arc(timing_check_arc.get_lib_arc());
  instance_timing_check_arc.set_lib_arc_set(timing_check_arc.get_lib_arc_set());
  return instance_timing_check_arc;
}

TimingCellArc* DesignLoader::findClockToQArc(TimingCell& timing_cell)
{
  for (TimingCellArc& timing_cell_arc : timing_cell.get_cell_arc_list()) {
    if (timing_cell_arc.get_is_clock_arc()) {
      return &timing_cell_arc;
    }
  }
  return nullptr;
}

std::string DesignLoader::getInstancePinName(Instance& instance, std::string& port_name)
{
  return instance.get_instance_name() + ":" + port_name;
}

std::string DesignLoader::findOutputPinName(Instance& instance, TimingCell& timing_cell)
{
  for (auto& [port_name, timing_cell_port] : timing_cell.get_port_map()) {
    if (timing_cell_port.get_is_output() && !timing_cell_port.get_is_clock()) {
      return getInstancePinName(instance, timing_cell_port.get_port_name());
    }
  }
  return "";
}

bool DesignLoader::isInstancePin(Pin& pin)
{
  return !pin.get_is_port();
}

void DesignLoader::makeUniqueName(std::vector<std::string>& list, const std::string& value)
{
  if (!STAUTIL.exist(list, value)) {
    list.push_back(value);
  }
}

void DesignLoader::buildNetList(Database& database)
{
  makeNetList(database);
}

void DesignLoader::makeNetList(Database& database)
{
  for (std::pair<const std::string, Pin>& pin_pair : database.get_pin_map()) {
    pin_pair.second.get_net_name().clear();
  }

  for (std::pair<const std::string, Net>& net_pair : database.get_net_map()) {
    makeNet(database, net_pair.first, net_pair.second);
  }
}

void DesignLoader::makeNet(Database& database, const std::string& net_name, Net& net)
{
  net.get_driver_pin().clear();
  net.get_driver_pin_list().clear();
  net.get_load_pin_list().clear();

  for (std::string& pin_name : net.get_pin_name_list()) {
    Pin& pin = database.get_pin_map()[pin_name];
    pin.set_net_name(net_name);
    if (isDriverPin(pin)) {
      if (net.get_driver_pin().empty()) {
        net.set_driver_pin(pin_name);
      }
      makeUniqueName(net.get_driver_pin_list(), pin_name);
    }
  }

  for (std::string& pin_name : net.get_pin_name_list()) {
    if (!STAUTIL.exist(net.get_driver_pin_list(), pin_name)) {
      makeUniqueName(net.get_load_pin_list(), pin_name);
    }
  }
}

bool DesignLoader::isDriverPin(Pin& pin)
{
  if (pin.get_is_port()) {
    return pin.get_direction() == PinDirection::kInput || pin.get_direction() == PinDirection::kInout;
  }
  return isOutputLikeDirection(pin.get_direction());
}

bool DesignLoader::isOutputLikeDirection(PinDirection direction)
{
  return direction == PinDirection::kOutput || direction == PinDirection::kInout;
}

void DesignLoader::buildParasiticLibrary(Database& database)
{
  database.get_parasitic_library().set_spef_file_path(dmInst->get_config().get_spef_path());
  database.get_parasitic_library().get_net_map().clear();
  spef::SpefReader* spef_reader = dmInst->get_spef_reader();
  if (spef_reader == nullptr || spef_reader->getSpefFile() == nullptr) {
    return;
  }

  database.get_parasitic_library().set_capacitive_unit(spef_reader->getSpefCapUnit());
  database.get_parasitic_library().set_resistance_unit(spef_reader->getSpefResUnit());

  spef::Exchange* spef_file = spef_reader->getSpefFile();
  for (spef::Net& spef_net : spef_file->nets) {
    buildParasiticNetMap(database, spef_net);
  }
}

void DesignLoader::buildParasiticNetMap(Database& database, spef::Net& spef_net)
{
  ParasiticNet parasitic_net;
  parasitic_net.set_net_name(spef_net.name);
  parasitic_net.set_lumped_capacitance(spef_net.lcap);
  for (spef::ConnEntry& spef_conn : spef_net.conns) {
    makeParasiticConnection(parasitic_net, spef_conn);
  }
  for (spef::ResCap& spef_cap : spef_net.caps) {
    makeParasiticCapacitance(parasitic_net, spef_cap);
  }
  for (spef::ResCap& spef_res : spef_net.ress) {
    makeParasiticResistance(parasitic_net, spef_res);
  }
  database.get_parasitic_library().get_net_map()[parasitic_net.get_net_name()] = parasitic_net;
}

void DesignLoader::makeParasiticConnection(ParasiticNet& parasitic_net, spef::ConnEntry& spef_conn)
{
  ParasiticNode& parasitic_node = getParasiticNode(parasitic_net, spef_conn.pin_port_name);
  parasitic_node.set_x(spef_conn.coordinate.x);
  parasitic_node.set_y(spef_conn.coordinate.y);
}

void DesignLoader::makeParasiticCapacitance(ParasiticNet& parasitic_net, spef::ResCap& spef_cap)
{
  ParasiticNode& parasitic_node = getParasiticNode(parasitic_net, spef_cap.node1);
  parasitic_node.set_capacitance(parasitic_node.get_capacitance() + spef_cap.res_or_cap);
  if (!spef_cap.node2.empty()) {
    ParasiticNode& coupled_node = getParasiticNode(parasitic_net, spef_cap.node2);
    coupled_node.set_capacitance(coupled_node.get_capacitance() + spef_cap.res_or_cap);
  }
}

void DesignLoader::makeParasiticResistance(ParasiticNet& parasitic_net, spef::ResCap& spef_res)
{
  ParasiticResistor parasitic_resistor;
  parasitic_resistor.set_source_node(spef_res.node1);
  parasitic_resistor.set_sink_node(spef_res.node2);
  parasitic_resistor.set_resistance(spef_res.res_or_cap);
  parasitic_net.get_resistor_list().push_back(parasitic_resistor);
  getParasiticNode(parasitic_net, spef_res.node1);
  getParasiticNode(parasitic_net, spef_res.node2);
}

ParasiticNode& DesignLoader::getParasiticNode(ParasiticNet& parasitic_net, const std::string& node_name)
{
  ParasiticNode& parasitic_node = parasitic_net.get_node_map()[node_name];
  parasitic_node.set_node_name(node_name);
  return parasitic_node;
}

void DesignLoader::readSdc(Database& database)
{
  std::string sdc_file_path = dmInst->get_config().get_sdc_path();
  database.get_timing_constraint().set_sdc_file_path(sdc_file_path);
  database.get_timing_constraint().get_clock_map().clear();
  database.get_timing_constraint().get_port_constraint_map().clear();
  if (sdc_file_path.empty()) {
    return;
  }

  std::vector<std::vector<std::string>> command_list = readCommandList(sdc_file_path);
  for (std::vector<std::string>& token_list : command_list) {
    parseCommand(database, token_list);
  }
}

std::vector<std::vector<std::string>> DesignLoader::readCommandList(std::string& sdc_file_path)
{
  std::ifstream sdc_file(sdc_file_path);
  std::string content;
  std::string line;
  while (std::getline(sdc_file, line)) {
    std::string command_line = removeComment(line);
    bool is_continue = !command_line.empty() && command_line.back() == '\\';
    if (is_continue) {
      command_line.pop_back();
    }
    content += command_line;
    content += is_continue ? " " : "\n";
  }

  std::vector<std::string> token_list = tokenizeSdc(content);
  std::vector<std::vector<std::string>> command_list;
  std::vector<std::string> command_token_list;
  for (std::string& token : token_list) {
    if (token == "\n" || token == ";") {
      if (!command_token_list.empty()) {
        command_list.push_back(command_token_list);
        command_token_list.clear();
      }
    } else {
      command_token_list.push_back(token);
    }
  }
  if (!command_token_list.empty()) {
    command_list.push_back(command_token_list);
  }
  return command_list;
}

std::vector<std::string> DesignLoader::tokenizeSdc(std::string& content)
{
  std::vector<std::string> token_list;
  std::string token;
  bool in_brace = false;
  bool in_quote = false;
  for (char ch : content) {
    if (in_brace) {
      if (ch == '}') {
        token_list.push_back(token);
        token.clear();
        in_brace = false;
      } else {
        token.push_back(ch);
      }
      continue;
    }
    if (in_quote) {
      if (ch == '"') {
        token_list.push_back(token);
        token.clear();
        in_quote = false;
      } else {
        token.push_back(ch);
      }
      continue;
    }
    if (ch == '{') {
      if (!token.empty()) {
        token_list.push_back(token);
        token.clear();
      }
      in_brace = true;
    } else if (ch == '"') {
      if (!token.empty()) {
        token_list.push_back(token);
        token.clear();
      }
      in_quote = true;
    } else if (std::isspace(static_cast<unsigned char>(ch)) || ch == ';') {
      if (!token.empty()) {
        token_list.push_back(token);
        token.clear();
      }
      if (ch == '\n' || ch == ';') {
        token_list.emplace_back(ch == '\n' ? "\n" : ";");
      }
    } else {
      token.push_back(ch);
    }
  }
  if (!token.empty()) {
    token_list.push_back(token);
  }
  return token_list;
}

std::string DesignLoader::removeComment(std::string& line)
{
  std::string result;
  bool in_brace = false;
  bool in_quote = false;
  for (char ch : line) {
    if (ch == '{' && !in_quote) {
      in_brace = true;
    } else if (ch == '}' && !in_quote) {
      in_brace = false;
    } else if (ch == '"' && !in_brace) {
      in_quote = !in_quote;
    }
    if (ch == '#' && !in_brace && !in_quote) {
      break;
    }
    result.push_back(ch);
  }
  return result;
}

void DesignLoader::parseCommand(Database& database, std::vector<std::string>& token_list)
{
  if (token_list.empty()) {
    return;
  }
  if (token_list.front() == "create_clock") {
    parseCreateClock(database, token_list);
  } else if (token_list.front() == "set_input_delay") {
    parseSetInputDelay(database, token_list);
  } else if (token_list.front() == "set_output_delay") {
    parseSetOutputDelay(database, token_list);
  } else if (token_list.front() == "set_input_transition") {
    parseSetInputTransition(database, token_list);
  } else if (token_list.front() == "set_load") {
    parseSetLoad(database, token_list);
  }
}

void DesignLoader::parseCreateClock(Database& database, std::vector<std::string>& token_list)
{
  TimingClock timing_clock;
  timing_clock.set_clock_name(getOptionValue(token_list, "-name"));
  timing_clock.set_period(getOptionDoubleValue(token_list, "-period", 0.0));
  std::vector<std::string> object_list = getObjectList(token_list);
  std::vector<std::string> source_list = resolveObjectList(database, object_list);
  if (timing_clock.get_clock_name().empty() && !source_list.empty()) {
    timing_clock.set_clock_name(source_list.front());
  }
  timing_clock.set_source_list(source_list);
  timing_clock.set_rise_edge(0.0);
  timing_clock.set_fall_edge(timing_clock.get_period() / 2.0);
  updateClock(database, timing_clock);
}

void DesignLoader::parseSetInputDelay(Database& database, std::vector<std::string>& token_list)
{
  const double delay_value = getCommandDoubleValue(token_list);
  const bool set_min = hasOption(token_list, "-min");
  const bool set_max = hasOption(token_list, "-max");
  std::string clock_name = getClockName(token_list);
  std::vector<std::string> object_list = getObjectList(token_list);
  for (std::string& port_name : resolveObjectList(database, object_list)) {
    TimingPortConstraint& port_constraint = getPortConstraint(database, port_name);
    port_constraint.set_clock_name(clock_name);
    if (set_min && !set_max) {
      port_constraint.set_input_delay_min(delay_value);
      port_constraint.set_has_input_delay_min(true);
    } else if (set_max && !set_min) {
      port_constraint.set_input_delay_max(delay_value);
      port_constraint.set_has_input_delay_max(true);
    } else {
      port_constraint.set_input_delay_min(delay_value);
      port_constraint.set_input_delay_max(delay_value);
      port_constraint.set_has_input_delay_min(true);
      port_constraint.set_has_input_delay_max(true);
    }
  }
}

void DesignLoader::parseSetOutputDelay(Database& database, std::vector<std::string>& token_list)
{
  const double delay_value = getCommandDoubleValue(token_list);
  const bool set_min = hasOption(token_list, "-min");
  const bool set_max = hasOption(token_list, "-max");
  std::string clock_name = getClockName(token_list);
  std::vector<std::string> object_list = getObjectList(token_list);
  for (std::string& port_name : resolveObjectList(database, object_list)) {
    TimingPortConstraint& port_constraint = getPortConstraint(database, port_name);
    port_constraint.set_clock_name(clock_name);
    if (set_min && !set_max) {
      port_constraint.set_output_delay_min(delay_value);
      port_constraint.set_has_output_delay_min(true);
    } else if (set_max && !set_min) {
      port_constraint.set_output_delay_max(delay_value);
      port_constraint.set_has_output_delay_max(true);
    } else {
      port_constraint.set_output_delay_min(delay_value);
      port_constraint.set_output_delay_max(delay_value);
      port_constraint.set_has_output_delay_min(true);
      port_constraint.set_has_output_delay_max(true);
    }
  }
}

void DesignLoader::parseSetInputTransition(Database& database, std::vector<std::string>& token_list)
{
  const double transition_value = getCommandDoubleValue(token_list);
  std::vector<std::string> object_list = getObjectList(token_list);
  for (std::string& port_name : resolveObjectList(database, object_list)) {
    TimingPortConstraint& port_constraint = getPortConstraint(database, port_name);
    port_constraint.set_input_transition(transition_value);
    port_constraint.set_has_input_transition(true);
  }
}

void DesignLoader::parseSetLoad(Database& database, std::vector<std::string>& token_list)
{
  const double load_value = getCommandDoubleValue(token_list);
  std::vector<std::string> object_list = getObjectList(token_list);
  for (std::string& port_name : resolveObjectList(database, object_list)) {
    TimingPortConstraint& port_constraint = getPortConstraint(database, port_name);
    port_constraint.set_load(load_value);
    port_constraint.set_has_load(true);
  }
}

double DesignLoader::getCommandDoubleValue(std::vector<std::string>& token_list)
{
  for (size_t i = 1; i < token_list.size(); i++) {
    if (token_list[i].empty() || token_list[i].front() == '-') {
      if (token_list[i] == "-clock" || token_list[i] == "-name") {
        i++;
      }
      continue;
    }
    char* end = nullptr;
    double value = std::strtod(token_list[i].c_str(), &end);
    if (end != token_list[i].c_str() && *end == '\0') {
      return value;
    }
  }
  return 0.0;
}

std::string DesignLoader::getOptionValue(std::vector<std::string>& token_list, const std::string& option)
{
  for (size_t i = 0; i + 1 < token_list.size(); i++) {
    if (token_list[i] == option) {
      return token_list[i + 1];
    }
  }
  return "";
}

double DesignLoader::getOptionDoubleValue(std::vector<std::string>& token_list, const std::string& option, double default_value)
{
  std::string option_value = getOptionValue(token_list, option);
  if (option_value.empty()) {
    return default_value;
  }
  return std::stod(option_value);
}

bool DesignLoader::hasOption(std::vector<std::string>& token_list, const std::string& option)
{
  return STAUTIL.exist(token_list, option);
}

std::string DesignLoader::getClockName(std::vector<std::string>& token_list)
{
  for (std::size_t i = 0; i + 1 < token_list.size(); i++) {
    if (token_list[i] != "-clock") {
      continue;
    }
    if (isClockCollectionCommand(token_list[i + 1])) {
      return getCollectionName(token_list, i + 1);
    }
    std::string clock_name = token_list[i + 1];
    if (!clock_name.empty() && clock_name.back() == ']') {
      clock_name.pop_back();
    }
    return clock_name;
  }
  return "";
}

std::string DesignLoader::getCollectionName(std::vector<std::string>& token_list, std::size_t collection_idx)
{
  std::vector<std::string> name_list;
  for (std::size_t i = collection_idx + 1; i < token_list.size(); i++) {
    std::string object_name = token_list[i];
    bool is_end = false;
    if (object_name == "]") {
      break;
    }
    pushObjectName(name_list, object_name);
    if (is_end) {
      break;
    }
  }
  return name_list.empty() ? "" : name_list.front();
}

std::vector<std::string> DesignLoader::getObjectList(std::vector<std::string>& token_list)
{
  for (std::size_t i = 1; i < token_list.size(); i++) {
    if (!isCollectionCommand(token_list[i])) {
      continue;
    }

    std::vector<std::string> object_list;
    for (std::size_t j = i + 1; j < token_list.size(); j++) {
      std::string object_name = token_list[j];
      bool is_end = false;
      if (object_name == "]") {
        break;
      }
      pushObjectName(object_list, object_name);
      if (is_end) {
        break;
      }
    }
    return object_list;
  }

  for (auto iter = token_list.rbegin(); iter != token_list.rend(); ++iter) {
    std::size_t token_idx = std::distance(iter, token_list.rend()) - 1;
    if (!iter->empty() && iter->front() != '-' && !isCommandOptionValue(token_list, token_idx)) {
      std::vector<std::string> object_list;
      pushObjectName(object_list, *iter);
      return object_list;
    }
  }
  return {};
}

void DesignLoader::pushObjectName(std::vector<std::string>& object_list, std::string object_name)
{
  std::istringstream iss(object_name);
  std::string split_object_name;
  while (iss >> split_object_name) {
    object_list.push_back(getObjectName(split_object_name));
  }
}

std::string DesignLoader::getObjectName(std::string& object_name)
{
  if (!object_name.empty() && object_name.front() == '\\') {
    object_name.erase(object_name.begin());
  }
  return object_name;
}

bool DesignLoader::isCollectionCommand(std::string& token)
{
  return token == "[get_ports" || token == "get_ports" || token == "[get_pins" || token == "get_pins";
}

bool DesignLoader::isClockCollectionCommand(std::string& token)
{
  return token == "[get_clocks" || token == "get_clocks";
}

bool DesignLoader::isCommandOptionValue(std::vector<std::string>& token_list, std::size_t token_idx)
{
  if (token_idx == 0 || token_idx >= token_list.size()) {
    return false;
  }
  std::string& prev_token = token_list[token_idx - 1];
  if (prev_token == "-name" || prev_token == "-clock" || prev_token == "-period") {
    return true;
  }
  char* end = nullptr;
  std::strtod(token_list[token_idx].c_str(), &end);
  return end != token_list[token_idx].c_str() && *end == '\0';
}

std::vector<std::string> DesignLoader::resolveObjectList(Database& database, std::vector<std::string>& object_list)
{
  std::vector<std::string> resolved_object_list;
  for (std::string& object_name : object_list) {
    std::string resolved_object_name = object_name;
    if (resolved_object_name.rfind("[get_ports", 0) == 0) {
      resolved_object_name = resolved_object_name.substr(10);
    }
    if (resolved_object_name.rfind("[get_pins", 0) == 0) {
      resolved_object_name = resolved_object_name.substr(9);
      std::replace(resolved_object_name.begin(), resolved_object_name.end(), '/', ':');
    }
    if (database.get_pin_map().count(resolved_object_name) > 0) {
      resolved_object_list.push_back(resolved_object_name);
      continue;
    }
    if (!resolved_object_name.empty() && resolved_object_name.back() == ']') {
      std::string trim_object_name = resolved_object_name;
      trim_object_name.pop_back();
      if (database.get_pin_map().count(trim_object_name) > 0) {
        resolved_object_list.push_back(trim_object_name);
        continue;
      }
    }
    std::replace(resolved_object_name.begin(), resolved_object_name.end(), '/', ':');
    if (database.get_pin_map().count(resolved_object_name) > 0) {
      resolved_object_list.push_back(resolved_object_name);
    }
  }
  return resolved_object_list;
}

void DesignLoader::updateClock(Database& database, TimingClock& timing_clock)
{
  if (timing_clock.get_clock_name().empty()) {
    return;
  }
  database.get_timing_constraint().get_clock_map()[timing_clock.get_clock_name()] = timing_clock;
}

TimingPortConstraint& DesignLoader::getPortConstraint(Database& database, const std::string& port_name)
{
  TimingPortConstraint& port_constraint = database.get_timing_constraint().get_port_constraint_map()[port_name];
  port_constraint.set_port_name(port_name);
  return port_constraint;
}

}  // namespace ista
