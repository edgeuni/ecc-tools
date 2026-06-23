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
#include "SdcReader.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"
#include "idm.h"

namespace ista {

// public

void SdcReader::initInst()
{
  if (_sr_instance == nullptr) {
    _sr_instance = new SdcReader();
  }
}

SdcReader& SdcReader::getInst()
{
  if (_sr_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_sr_instance;
}

void SdcReader::destroyInst()
{
  if (_sr_instance != nullptr) {
    delete _sr_instance;
    _sr_instance = nullptr;
  }
}

// function

bool SdcReader::read()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();

  readSdc(database);

  STALOG.info(Loc::current(), "Read iSTA sdc: clocks=", database.get_timing_constraint().get_clock_map().size(),
              " port_constraints=", database.get_timing_constraint().get_port_constraint_map().size());
  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
  return true;
}

// private

SdcReader* SdcReader::_sr_instance = nullptr;

void SdcReader::readSdc(Database& database)
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

std::vector<std::vector<std::string>> SdcReader::readCommandList(std::string& sdc_file_path)
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

std::vector<std::string> SdcReader::tokenizeSdc(std::string& content)
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

std::string SdcReader::removeComment(std::string& line)
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

void SdcReader::parseCommand(Database& database, std::vector<std::string>& token_list)
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

void SdcReader::parseCreateClock(Database& database, std::vector<std::string>& token_list)
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

void SdcReader::parseSetInputDelay(Database& database, std::vector<std::string>& token_list)
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

void SdcReader::parseSetOutputDelay(Database& database, std::vector<std::string>& token_list)
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

void SdcReader::parseSetInputTransition(Database& database, std::vector<std::string>& token_list)
{
  const double transition_value = getCommandDoubleValue(token_list);
  std::vector<std::string> object_list = getObjectList(token_list);
  for (std::string& port_name : resolveObjectList(database, object_list)) {
    TimingPortConstraint& port_constraint = getPortConstraint(database, port_name);
    port_constraint.set_input_transition(transition_value);
    port_constraint.set_has_input_transition(true);
  }
}

void SdcReader::parseSetLoad(Database& database, std::vector<std::string>& token_list)
{
  const double load_value = getCommandDoubleValue(token_list);
  std::vector<std::string> object_list = getObjectList(token_list);
  for (std::string& port_name : resolveObjectList(database, object_list)) {
    TimingPortConstraint& port_constraint = getPortConstraint(database, port_name);
    port_constraint.set_load(load_value);
    port_constraint.set_has_load(true);
  }
}

double SdcReader::getCommandDoubleValue(std::vector<std::string>& token_list)
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

std::string SdcReader::getOptionValue(std::vector<std::string>& token_list, const std::string& option)
{
  for (size_t i = 0; i + 1 < token_list.size(); i++) {
    if (token_list[i] == option) {
      return token_list[i + 1];
    }
  }
  return "";
}

double SdcReader::getOptionDoubleValue(std::vector<std::string>& token_list, const std::string& option, double default_value)
{
  std::string option_value = getOptionValue(token_list, option);
  if (option_value.empty()) {
    return default_value;
  }
  return std::stod(option_value);
}

bool SdcReader::hasOption(std::vector<std::string>& token_list, const std::string& option)
{
  return STAUTIL.exist(token_list, option);
}

std::string SdcReader::getClockName(std::vector<std::string>& token_list)
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

std::string SdcReader::getCollectionName(std::vector<std::string>& token_list, std::size_t collection_idx)
{
  std::vector<std::string> name_list;
  for (std::size_t i = collection_idx + 1; i < token_list.size(); i++) {
    std::string object_name = token_list[i];
    bool is_end = false;
    if (object_name == "]") {
      break;
    }
    if (!object_name.empty() && object_name.back() == ']') {
      object_name.pop_back();
      is_end = true;
    }
    pushObjectName(name_list, object_name);
    if (is_end) {
      break;
    }
  }
  return name_list.empty() ? "" : name_list.front();
}

std::vector<std::string> SdcReader::getObjectList(std::vector<std::string>& token_list)
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
      if (!object_name.empty() && object_name.back() == ']') {
        object_name.pop_back();
        is_end = true;
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

void SdcReader::pushObjectName(std::vector<std::string>& object_list, std::string object_name)
{
  if (!object_name.empty() && object_name.back() == ']') {
    object_name.pop_back();
  }
  std::istringstream iss(object_name);
  std::string split_object_name;
  while (iss >> split_object_name) {
    object_list.push_back(split_object_name);
  }
}

bool SdcReader::isCollectionCommand(std::string& token)
{
  return token == "[get_ports" || token == "get_ports" || token == "[get_pins" || token == "get_pins";
}

bool SdcReader::isClockCollectionCommand(std::string& token)
{
  return token == "[get_clocks" || token == "get_clocks";
}

bool SdcReader::isCommandOptionValue(std::vector<std::string>& token_list, std::size_t token_idx)
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

std::vector<std::string> SdcReader::resolveObjectList(Database& database, std::vector<std::string>& object_list)
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
    if (!resolved_object_name.empty() && resolved_object_name.back() == ']') {
      resolved_object_name.pop_back();
    }
    if (database.get_pin_map().count(resolved_object_name) > 0) {
      resolved_object_list.push_back(resolved_object_name);
      continue;
    }
    std::replace(resolved_object_name.begin(), resolved_object_name.end(), '/', ':');
    if (database.get_pin_map().count(resolved_object_name) > 0) {
      resolved_object_list.push_back(resolved_object_name);
    }
  }
  return resolved_object_list;
}

void SdcReader::updateClock(Database& database, TimingClock& timing_clock)
{
  if (timing_clock.get_clock_name().empty()) {
    return;
  }
  database.get_timing_constraint().get_clock_map()[timing_clock.get_clock_name()] = timing_clock;
}

TimingPortConstraint& SdcReader::getPortConstraint(Database& database, const std::string& port_name)
{
  TimingPortConstraint& port_constraint = database.get_timing_constraint().get_port_constraint_map()[port_name];
  port_constraint.set_port_name(port_name);
  return port_constraint;
}

}  // namespace ista
