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
#include "TimingReporter.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ista {

// public

void TimingReporter::initInst()
{
  if (_tr_instance == nullptr) {
    _tr_instance = new TimingReporter();
  }
}

TimingReporter& TimingReporter::getInst()
{
  if (_tr_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tr_instance;
}

void TimingReporter::destroyInst()
{
  if (_tr_instance != nullptr) {
    delete _tr_instance;
    _tr_instance = nullptr;
  }
}

// function

void TimingReporter::report()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();
  reportTiming(database);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TimingReporter* TimingReporter::_tr_instance = nullptr;

void TimingReporter::reportTiming(Database& database)
{
  outputTimingReportList(database);
  STALOG.info(Loc::current(), "Output iSTA timing report: ", STADM.getConfig().tr_temp_directory_path);
}

void TimingReporter::outputTimingReportList(Database& database)
{
  outputTimingReport(database, DelayType::kMax, StartEndType::kInToOut);
  outputTimingReport(database, DelayType::kMax, StartEndType::kInToReg);
  outputTimingReport(database, DelayType::kMax, StartEndType::kRegToOut);
  outputTimingReport(database, DelayType::kMax, StartEndType::kRegToReg);
  outputTimingReport(database, DelayType::kMin, StartEndType::kInToOut);
  outputTimingReport(database, DelayType::kMin, StartEndType::kInToReg);
  outputTimingReport(database, DelayType::kMin, StartEndType::kRegToOut);
  outputTimingReport(database, DelayType::kMin, StartEndType::kRegToReg);
}

void TimingReporter::outputTimingReport(Database& database, DelayType delay_type, StartEndType start_end_type)
{
  std::string report_file_path = getReportFilePath(delay_type, start_end_type);
  std::ofstream* report_file = STAUTIL.getOutputFileStream(report_file_path);
  outputReportHeader(report_file, database, delay_type, start_end_type);
  outputPathGroupList(report_file, database, delay_type, start_end_type);
  outputReportFooter(report_file);
  STAUTIL.closeFileStream(report_file);
}

std::string TimingReporter::getReportFilePath(DelayType delay_type, StartEndType start_end_type)
{
  return STAUTIL.getString(STADM.getConfig().tr_temp_directory_path, "timing_", getDelayTypeName(delay_type), "_", getReportStartEndTypeName(start_end_type),
                           ".rpt");
}

std::string TimingReporter::getReportStartEndTypeName(StartEndType start_end_type)
{
  if (start_end_type == StartEndType::kInToOut) {
    return "in2out";
  }
  if (start_end_type == StartEndType::kInToReg) {
    return "in2reg";
  }
  if (start_end_type == StartEndType::kRegToOut) {
    return "reg2out";
  }
  return "reg2reg";
}

void TimingReporter::outputReportHeader(std::ofstream* report_file, Database& database, DelayType delay_type, StartEndType start_end_type)
{
  (*report_file) << "****************************************\n";
  (*report_file) << "Design : " << database.get_design_name() << "\n";
  (*report_file) << "DelayType : " << getDelayTypeName(delay_type) << "\n";
  (*report_file) << "StartEndType : " << getStartEndTypeName(start_end_type) << "\n";
  (*report_file) << "MaxSlack : 1000\n";
  (*report_file) << "MaxPaths : " << STADM.getConfig().path_report_number << "\n";
  (*report_file) << "SortBy : slack\n";
  (*report_file) << "****************************************\n\n";
}

std::string TimingReporter::getDelayTypeName(DelayType delay_type)
{
  if (delay_type == DelayType::kMax) {
    return "max";
  }
  return "min";
}

std::string TimingReporter::getStartEndTypeName(StartEndType start_end_type)
{
  if (start_end_type == StartEndType::kInToOut) {
    return "in_to_out";
  }
  if (start_end_type == StartEndType::kInToReg) {
    return "in_to_reg";
  }
  if (start_end_type == StartEndType::kRegToOut) {
    return "reg_to_out";
  }
  return "reg_to_reg";
}

void TimingReporter::outputPathGroupList(std::ofstream* report_file, Database& database, DelayType delay_type, StartEndType start_end_type)
{
  if (database.get_timing_path_group_list().empty()) {
    (*report_file) << "No constrained paths.\n\n";
    return;
  }
  bool has_timing_path = false;
  for (TimingPathGroup& timing_path_group : database.get_timing_path_group_list()) {
    std::vector<TimingPath*> timing_path_list = getSortedTimingPathList(database, timing_path_group, delay_type, start_end_type);
    if (!timing_path_list.empty()) {
      has_timing_path = true;
    }
    outputTimingPathGroup(report_file, database, timing_path_group, delay_type, start_end_type);
  }
  if (!has_timing_path) {
    (*report_file) << "No constrained paths.\n\n";
  }
}

void TimingReporter::outputReportFooter(std::ofstream* report_file)
{
  (*report_file) << "1\n";
}

void TimingReporter::outputTimingPathGroup(std::ofstream* report_file, Database& database, TimingPathGroup& timing_path_group, DelayType delay_type,
                                           StartEndType start_end_type)
{
  int32_t path_idx = 1;
  int32_t path_report_number = STADM.getConfig().path_report_number;
  std::vector<TimingPath*> timing_path_list = getSortedTimingPathList(database, timing_path_group, delay_type, start_end_type);
  for (TimingPath* timing_path : timing_path_list) {
    if (path_idx > path_report_number) {
      break;
    }
    outputTimingPath(report_file, database, *timing_path, timing_path_group.get_group_name(), delay_type);
    path_idx++;
  }
}

std::vector<TimingPath*> TimingReporter::getSortedTimingPathList(Database& database, TimingPathGroup& timing_path_group, DelayType delay_type,
                                                                 StartEndType start_end_type)
{
  std::vector<TimingPath*> timing_path_list;
  for (auto& [end_point, timing_path_end] : timing_path_group.get_timing_path_end_map()) {
    for (TimingPath& timing_path : timing_path_end.get_timing_path_list()) {
      if (isMatchAnalysisType(timing_path, delay_type) && isMatchStartEndType(database, timing_path, start_end_type)) {
        timing_path_list.push_back(&timing_path);
      }
    }
  }
  timing_path_list = getEndpointWorstTimingPathList(timing_path_list);
  std::sort(timing_path_list.begin(), timing_path_list.end(), [](TimingPath* left, TimingPath* right) { return left->get_slack() < right->get_slack(); });
  return timing_path_list;
}

std::vector<TimingPath*> TimingReporter::getEndpointWorstTimingPathList(std::vector<TimingPath*>& timing_path_list)
{
  std::map<std::string, TimingPath*> end_point_timing_path_map;
  for (TimingPath* timing_path : timing_path_list) {
    std::string& end_point = timing_path->get_end_point();
    if (end_point_timing_path_map.count(end_point) == 0 || timing_path->get_slack() < end_point_timing_path_map[end_point]->get_slack()) {
      end_point_timing_path_map[end_point] = timing_path;
    }
  }

  std::vector<TimingPath*> endpoint_worst_timing_path_list;
  for (std::pair<const std::string, TimingPath*>& timing_path_pair : end_point_timing_path_map) {
    endpoint_worst_timing_path_list.push_back(timing_path_pair.second);
  }
  return endpoint_worst_timing_path_list;
}

bool TimingReporter::isMatchAnalysisType(TimingPath& timing_path, DelayType delay_type)
{
  if (delay_type == DelayType::kMin) {
    return timing_path.get_analysis_type() == AnalysisType::kMin;
  }
  return timing_path.get_analysis_type() == AnalysisType::kMax;
}

bool TimingReporter::isMatchStartEndType(Database& database, TimingPath& timing_path, StartEndType start_end_type)
{
  if (isPowerGroundPin(database, timing_path.get_start_point()) || isPowerGroundPin(database, timing_path.get_end_point())) {
    return false;
  }
  bool start_is_port = isPort(database, timing_path.get_start_point());
  bool end_is_port = isPort(database, timing_path.get_end_point());
  if (start_end_type == StartEndType::kInToOut) {
    return start_is_port && end_is_port;
  }
  if (start_end_type == StartEndType::kInToReg) {
    return start_is_port && isRegisterEndPoint(database, timing_path.get_end_point());
  }
  if (start_end_type == StartEndType::kRegToOut) {
    return isRegisterStartPoint(database, timing_path.get_start_point()) && end_is_port;
  }
  return isRegisterStartPoint(database, timing_path.get_start_point()) && isRegisterEndPoint(database, timing_path.get_end_point());
}

bool TimingReporter::isPort(Database& database, std::string& pin_name)
{
  return database.get_pin_map()[pin_name].get_is_port();
}

bool TimingReporter::isRegisterStartPoint(Database& database, std::string& pin_name)
{
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_output_pin_name() && hasClockPoint(database, instance.get_clock_pin_name());
}

bool TimingReporter::isRegisterEndPoint(Database& database, std::string& pin_name)
{
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || !hasClockPoint(database, instance.get_clock_pin_name())) {
    return false;
  }
  for (TimingCheckArc& timing_check_arc : instance.get_check_arc_list()) {
    if (timing_check_arc.get_data_port() == pin_name) {
      return true;
    }
  }
  return false;
}

bool TimingReporter::hasClockPoint(Database& database, std::string& pin_name)
{
  return database.get_timing_point_map().count(pin_name) > 0 && database.get_timing_point_map()[pin_name].get_is_clock_point();
}

bool TimingReporter::isPowerGroundPin(Database& database, std::string& pin_name)
{
  Pin& pin = database.get_pin_map()[pin_name];
  return pin.get_pin_name() == "VDD" || pin.get_pin_name() == "VSS" || pin.get_pin_name() == "VDDIO" || pin.get_pin_name() == "VSSIO";
}

void TimingReporter::outputTimingPath(std::ofstream* report_file, Database& database, TimingPath& timing_path, std::string& path_group_name,
                                      DelayType delay_type)
{
  outputTimingPathHeader(report_file, database, timing_path, path_group_name, delay_type);
  std::size_t label_width = outputTimingPointList(report_file, database, timing_path, delay_type);
  outputTimingPathSummary(report_file, timing_path, label_width);
}

void TimingReporter::outputTimingPathHeader(std::ofstream* report_file, Database& database, TimingPath& timing_path, std::string& path_group_name,
                                            DelayType delay_type)
{
  (*report_file) << "  Startpoint: " << getStartPointText(database, timing_path) << "\n";
  (*report_file) << "  Endpoint: " << getEndPointText(database, timing_path) << "\n";
  if (isRegisterStartPoint(database, timing_path.get_start_point()) && isRegisterEndPoint(database, timing_path.get_end_point())) {
    (*report_file) << "  Last common pin: " << getPTPinName(timing_path.get_last_common_pin()) << "\n";
  }
  (*report_file) << "  Path Group: " << path_group_name << "\n";
  (*report_file) << "  Path Type: " << getDelayTypeName(delay_type) << "\n\n";
}

std::string TimingReporter::getStartPointText(Database& database, TimingPath& timing_path)
{
  Pin& start_pin = database.get_pin_map()[timing_path.get_start_point()];
  std::string clock_name = getClockName(database, timing_path);
  std::string start_point = getPTPinName(timing_path.get_start_point());
  if (!start_pin.get_is_port()) {
    start_point = start_pin.get_instance_name();
  }
  if (isPort(database, timing_path.get_start_point())) {
    return STAUTIL.getString(start_point, " (input port clocked by ", clock_name, ")");
  }
  return STAUTIL.getString(start_point, " (rising edge-triggered flip-flop clocked by ", clock_name, ")");
}

std::string TimingReporter::getEndPointText(Database& database, TimingPath& timing_path)
{
  Pin& end_pin = database.get_pin_map()[timing_path.get_end_point()];
  std::string clock_name = getClockName(database, timing_path);
  std::string end_point = getPTPinName(timing_path.get_end_point());
  if (!end_pin.get_is_port()) {
    end_point = end_pin.get_instance_name();
  }
  if (isPort(database, timing_path.get_end_point())) {
    return STAUTIL.getString(end_point, " (output port clocked by ", clock_name, ")");
  }
  return getEndPointCheckText(end_point, clock_name, timing_path);
}

std::string TimingReporter::getEndPointCheckText(std::string& end_point, std::string& clock_name, TimingPath& timing_path)
{
  if (timing_path.get_check_type() == TimingCheckType::kRecovery) {
    return STAUTIL.getString(end_point, " (recovery check against rising-edge clock ", clock_name, ")");
  }
  if (timing_path.get_check_type() == TimingCheckType::kRemoval) {
    return STAUTIL.getString(end_point, " (removal check against rising-edge clock ", clock_name, ")");
  }
  return STAUTIL.getString(end_point, " (rising edge-triggered flip-flop clocked by ", clock_name, ")");
}

std::size_t TimingReporter::outputTimingPointList(std::ofstream* report_file, Database& database, TimingPath& timing_path, DelayType delay_type)
{
  std::size_t label_width = getTimingLineLabelWidth(database, timing_path, delay_type);
  outputTimingPointHeader(report_file, label_width);
  (*report_file) << "  " << std::string(label_width + 28, '-') << "\n";
  outputLaunchClockInfo(report_file, database, timing_path, delay_type, label_width);
  bool is_first_point = true;
  for (TimingPathPoint& path_point : timing_path.get_point_list()) {
    if (shouldOutputTimingPoint(database, timing_path, path_point)) {
      outputTimingPoint(report_file, database, timing_path, path_point, is_first_point, label_width);
      is_first_point = false;
    }
  }
  outputTimingSummaryLine(report_file, "data arrival time", timing_path.get_path_delay(), label_width);
  (*report_file) << "\n";
  outputRequiredClockInfo(report_file, database, timing_path, delay_type, label_width);
  return label_width;
}

std::size_t TimingReporter::getTimingLineLabelWidth(Database& database, TimingPath& timing_path, DelayType delay_type)
{
  std::size_t label_width = 35;
  std::string clock_name = getClockName(database, timing_path);
  updateTimingLineLabelWidth(label_width, STAUTIL.getString("clock ", clock_name, " (rise edge)"));
  updateTimingLineLabelWidth(label_width, "clock network delay (propagated)");

  std::string start_clock_pin = getStartClockPin(database, timing_path);
  if (!start_clock_pin.empty() && start_clock_pin != timing_path.get_start_point()) {
    updateTimingLineLabelWidth(label_width, getPinLabel(database, start_clock_pin));
  }

  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  bool has_input_delay = port_constraint_map.count(timing_path.get_start_point()) > 0
                         && (port_constraint_map[timing_path.get_start_point()].get_has_input_delay_max()
                             || port_constraint_map[timing_path.get_start_point()].get_has_input_delay_min());
  if (isPort(database, timing_path.get_start_point()) && has_input_delay) {
    updateTimingLineLabelWidth(label_width, "input external delay");
  }

  for (TimingPathPoint& path_point : timing_path.get_point_list()) {
    if (shouldOutputTimingPoint(database, timing_path, path_point)) {
      updateTimingLineLabelWidth(label_width, getPointLabel(database, path_point));
    }
  }

  updateTimingLineLabelWidth(label_width, "clock reconvergence pessimism");
  if (!timing_path.get_capture_clock_pin().empty()) {
    updateTimingLineLabelWidth(label_width, getPinLabel(database, timing_path.get_capture_clock_pin()));
  }
  if (std::fabs(timing_path.get_check_time()) > STA_ERROR) {
    updateTimingLineLabelWidth(label_width, getLibraryCheckText(timing_path, delay_type));
  } else if (isPort(database, timing_path.get_end_point())) {
    bool has_output_delay = port_constraint_map.count(timing_path.get_end_point()) > 0
                            && (port_constraint_map[timing_path.get_end_point()].get_has_output_delay_max()
                                || port_constraint_map[timing_path.get_end_point()].get_has_output_delay_min());
    if (has_output_delay) {
      updateTimingLineLabelWidth(label_width, "output external delay");
    }
  }
  return label_width;
}

bool TimingReporter::shouldOutputTimingPoint(Database& database, TimingPath& timing_path, TimingPathPoint& path_point)
{
  if (isPowerGroundPin(database, path_point.get_pin_name())) {
    return false;
  }
  Pin& pin = database.get_pin_map()[path_point.get_pin_name()];
  if (pin.get_is_port()) {
    return true;
  }
  if (path_point.get_pin_name() == timing_path.get_start_point() || path_point.get_pin_name() == timing_path.get_end_point()) {
    return true;
  }
  return pin.get_direction() == PinDirection::kOutput || pin.get_direction() == PinDirection::kInout;
}

void TimingReporter::updateTimingLineLabelWidth(std::size_t& label_width, std::string label)
{
  label_width = std::max(label_width, label.length());
}

void TimingReporter::outputTimingPointHeader(std::ofstream* report_file, std::size_t label_width)
{
  (*report_file) << "  " << std::left << std::setw(label_width) << "Point" << std::right << std::setw(10) << "Incr" << std::setw(11) << "Path"
                 << "\n";
}

void TimingReporter::outputLaunchClockInfo(std::ofstream* report_file, Database& database, TimingPath& timing_path, DelayType delay_type,
                                           std::size_t label_width)
{
  std::string clock_name = getClockName(database, timing_path);
  double launch_time = timing_path.get_launch_time();
  double launch_clock_network_delay = timing_path.get_launch_clock_network_delay();
  outputTimingLine(report_file, STAUTIL.getString("clock ", clock_name, " (rise edge)"), 0.0, 0.0, true, "", label_width);
  outputTimingLine(report_file, "clock network delay (propagated)", launch_clock_network_delay, launch_time, true, "", label_width);

  std::string start_clock_pin = getStartClockPin(database, timing_path);
  if (!start_clock_pin.empty() && start_clock_pin != timing_path.get_start_point()) {
    outputTimingLine(report_file, getPinLabel(database, start_clock_pin), 0.0, launch_time, true, "r", label_width);
  }

  double input_delay = getInputDelay(database, timing_path, delay_type);
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  bool has_input_delay = port_constraint_map.count(timing_path.get_start_point()) > 0
                         && (port_constraint_map[timing_path.get_start_point()].get_has_input_delay_max()
                             || port_constraint_map[timing_path.get_start_point()].get_has_input_delay_min());
  if (isPort(database, timing_path.get_start_point()) && has_input_delay) {
    outputTimingLine(report_file, "input external delay", input_delay, launch_time + input_delay, true, "r", label_width);
  }
}

std::string TimingReporter::getTransTypeName(TransType trans_type)
{
  if (trans_type == TransType::kFall) {
    return "f";
  }
  if (trans_type == TransType::kRise) {
    return "r";
  }
  return "";
}

void TimingReporter::outputTimingLine(std::ofstream* report_file, std::string label, double incr, double path, bool has_incr, std::string transition,
                                      std::size_t label_width)
{
  if (has_incr) {
    (*report_file) << "  " << std::left << std::setw(label_width + 2) << label << getNumberString(incr) << "\n";
    (*report_file) << "  " << std::setw(label_width + 13) << "" << getNumberString(path);
  } else {
    (*report_file) << "  " << std::left << std::setw(label_width + 13) << label << getNumberString(path);
  }
  if (!transition.empty()) {
    (*report_file) << " " << transition;
  }
  (*report_file) << "\n";
}

void TimingReporter::outputTimingSummaryLine(std::ofstream* report_file, std::string label, double value, std::size_t label_width)
{
  (*report_file) << "  " << std::left << std::setw(label_width + 13) << label << getNumberString(value) << "\n";
}

std::string TimingReporter::getClockName(Database& database, TimingPath& timing_path)
{
  if (!timing_path.get_clock_name().empty()) {
    return timing_path.get_clock_name();
  }
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
  if (!clock_map.empty()) {
    return clock_map.begin()->first;
  }
  return "clk";
}

double TimingReporter::getClockPeriod(Database& database, std::string& clock_name)
{
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
  if (clock_map.count(clock_name) > 0) {
    return clock_map[clock_name].get_period();
  }
  if (!clock_map.empty()) {
    return clock_map.begin()->second.get_period();
  }
  return 0.0;
}

double TimingReporter::getInputDelay(Database& database, TimingPath& timing_path, DelayType delay_type)
{
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  if (port_constraint_map.count(timing_path.get_start_point()) == 0) {
    return 0.0;
  }
  TimingPortConstraint& port_constraint = port_constraint_map[timing_path.get_start_point()];
  if (delay_type == DelayType::kMin && port_constraint.get_has_input_delay_min()) {
    return port_constraint.get_input_delay_min();
  }
  if (port_constraint.get_has_input_delay_max()) {
    return port_constraint.get_input_delay_max();
  }
  return 0.0;
}

std::string TimingReporter::getStartClockPin(Database& database, TimingPath& timing_path)
{
  Pin& start_pin = database.get_pin_map()[timing_path.get_start_point()];
  if (start_pin.get_is_port() || database.get_instance_map().count(start_pin.get_instance_name()) == 0) {
    return "";
  }
  return database.get_instance_map()[start_pin.get_instance_name()].get_clock_pin_name();
}

void TimingReporter::outputTimingPoint(std::ofstream* report_file, Database& database, TimingPath& timing_path, TimingPathPoint& path_point,
                                       bool is_first_point, std::size_t label_width)
{
  double arc_delay = path_point.get_arc_delay();
  if (is_first_point && !database.get_pin_map()[path_point.get_pin_name()].get_is_port()) {
    arc_delay = path_point.get_arrival() - timing_path.get_launch_time();
  }
  outputTimingLine(report_file, getPointLabel(database, path_point), arc_delay, path_point.get_arrival(), true, getTransTypeName(path_point.get_trans_type()),
                   label_width);
}

std::string TimingReporter::getNumberString(double value)
{
  std::ostringstream oss;
  if (std::fabs(value) < STA_ERROR) {
    value = 0.0;
  }
  oss << std::fixed << std::setprecision(10) << value;
  return oss.str();
}

std::string TimingReporter::getPointLabel(Database& database, TimingPathPoint& path_point)
{
  Pin& pin = database.get_pin_map()[path_point.get_pin_name()];
  if (pin.get_is_port()) {
    if (pin.get_direction() == PinDirection::kInput) {
      return STAUTIL.getString(getPTPinName(path_point.get_pin_name()), " (in)");
    }
    if (pin.get_direction() == PinDirection::kOutput) {
      return STAUTIL.getString(getPTPinName(path_point.get_pin_name()), " (out)");
    }
  }
  std::string point_label = getPTPinName(path_point.get_pin_name());
  std::string cell_name = getPTCellName(path_point);
  if (!cell_name.empty()) {
    point_label = STAUTIL.getString(point_label, " (", cell_name, ")");
  }
  return point_label;
}

std::string TimingReporter::getPTPinName(std::string& pin_name)
{
  std::string pt_pin_name = pin_name;
  std::replace(pt_pin_name.begin(), pt_pin_name.end(), ':', '/');
  return pt_pin_name;
}

std::string TimingReporter::getPTCellName(TimingPathPoint& path_point)
{
  return path_point.get_cell_name();
}

void TimingReporter::outputRequiredClockInfo(std::ofstream* report_file, Database& database, TimingPath& timing_path, DelayType delay_type,
                                             std::size_t label_width)
{
  std::string clock_name = getClockName(database, timing_path);
  double capture_time = timing_path.get_capture_time();
  double clock_edge = delay_type == DelayType::kMin ? 0.0 : getClockPeriod(database, clock_name);
  double capture_clock_network_delay = timing_path.get_capture_clock_network_delay();
  outputTimingLine(report_file, STAUTIL.getString("clock ", clock_name, " (rise edge)"), clock_edge, clock_edge, true, "", label_width);
  outputTimingLine(report_file, "clock network delay (propagated)", capture_clock_network_delay, clock_edge + capture_clock_network_delay, true, "",
                   label_width);
  outputTimingLine(report_file, "clock reconvergence pessimism", timing_path.get_clock_reconvergence_pessimism(), capture_time, true, "", label_width);
  if (!timing_path.get_capture_clock_pin().empty()) {
    outputTimingLine(report_file, getPinLabel(database, timing_path.get_capture_clock_pin()), 0.0, capture_time, false, "r", label_width);
  }
  if (std::fabs(timing_path.get_check_time()) > STA_ERROR) {
    double check_time = timing_path.get_check_time();
    if (delay_type == DelayType::kMax) {
      check_time = -check_time;
    }
    outputTimingLine(report_file, getLibraryCheckText(timing_path, delay_type), check_time, timing_path.get_required_time(), true, "", label_width);
  } else if (isPort(database, timing_path.get_end_point())) {
    double output_delay = getOutputDelay(database, timing_path, delay_type);
    std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
    bool has_output_delay = port_constraint_map.count(timing_path.get_end_point()) > 0
                            && (port_constraint_map[timing_path.get_end_point()].get_has_output_delay_max()
                                || port_constraint_map[timing_path.get_end_point()].get_has_output_delay_min());
    if (has_output_delay) {
      outputTimingLine(report_file, "output external delay", output_delay, timing_path.get_required_time(), true, "", label_width);
    }
  }
  outputTimingSummaryLine(report_file, "data required time", timing_path.get_required_time(), label_width);
}

std::string TimingReporter::getLibraryCheckText(TimingPath& timing_path, DelayType delay_type)
{
  if (timing_path.get_check_type() == TimingCheckType::kRecovery) {
    return "library recovery time";
  }
  if (timing_path.get_check_type() == TimingCheckType::kRemoval) {
    return "library removal time";
  }
  if (delay_type == DelayType::kMin) {
    return "library hold time";
  }
  return "library setup time";
}

double TimingReporter::getOutputDelay(Database& database, TimingPath& timing_path, DelayType delay_type)
{
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  if (port_constraint_map.count(timing_path.get_end_point()) == 0) {
    return 0.0;
  }
  TimingPortConstraint& port_constraint = port_constraint_map[timing_path.get_end_point()];
  if (delay_type == DelayType::kMin && port_constraint.get_has_output_delay_min()) {
    return port_constraint.get_output_delay_min();
  }
  if (port_constraint.get_has_output_delay_max()) {
    return port_constraint.get_output_delay_max();
  }
  return 0.0;
}

std::string TimingReporter::getPinLabel(Database& database, std::string& pin_name)
{
  std::string point_label = getPTPinName(pin_name);
  Pin& pin = database.get_pin_map()[pin_name];
  if (!pin.get_instance_name().empty() && database.get_instance_map().count(pin.get_instance_name()) > 0) {
    point_label = STAUTIL.getString(point_label, " (", database.get_instance_map()[pin.get_instance_name()].get_cell_name(), ")");
  }
  return point_label;
}

void TimingReporter::outputTimingPathSummary(std::ofstream* report_file, TimingPath& timing_path, std::size_t label_width)
{
  (*report_file) << "  " << std::string(label_width + 28, '-') << "\n";
  outputTimingSummaryLine(report_file, "data required time", timing_path.get_required_time(), label_width);
  outputTimingSummaryLine(report_file, "data arrival time", -timing_path.get_path_delay(), label_width);
  (*report_file) << "  " << std::string(label_width + 28, '-') << "\n";
  outputTimingSummaryLine(report_file, STAUTIL.getString("slack (", getSlackStatus(timing_path), ")"), timing_path.get_slack(), label_width);
  (*report_file) << "\n\n";
}

std::string TimingReporter::getSlackStatus(TimingPath& timing_path)
{
  return timing_path.get_slack() < 0.0 ? "VIOLATED" : "MET";
}

}  // namespace ista
