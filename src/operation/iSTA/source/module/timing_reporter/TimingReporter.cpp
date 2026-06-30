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

  reportTiming();

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

// private

TimingReporter* TimingReporter::_tr_instance = nullptr;

void TimingReporter::reportTiming()
{
  outputTimingReportList();
}

void TimingReporter::outputTimingReportList()
{
  outputTimingReport(DelayType::kMax, StartEndType::kInToOut);
  outputTimingReport(DelayType::kMax, StartEndType::kInToReg);
  outputTimingReport(DelayType::kMax, StartEndType::kRegToOut);
  outputTimingReport(DelayType::kMax, StartEndType::kRegToReg);
  outputTimingReport(DelayType::kMin, StartEndType::kInToOut);
  outputTimingReport(DelayType::kMin, StartEndType::kInToReg);
  outputTimingReport(DelayType::kMin, StartEndType::kRegToOut);
  outputTimingReport(DelayType::kMin, StartEndType::kRegToReg);
}

void TimingReporter::outputTimingReport(DelayType delay_type, StartEndType start_end_type)
{
  std::string report_file_path = getReportFilePath(delay_type, start_end_type);
  std::ofstream* report_file = STAUTIL.getOutputFileStream(report_file_path);
  outputReportHeader(report_file, delay_type, start_end_type);
  outputPathGroupList(report_file, delay_type, start_end_type);
  outputReportFooter(report_file);
  STAUTIL.closeFileStream(report_file);
  outputJsonReport(report_file_path, delay_type, start_end_type);
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

void TimingReporter::outputReportHeader(std::ofstream* report_file, DelayType delay_type, StartEndType start_end_type)
{
  Database& database = STADM.getDatabase();
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

void TimingReporter::outputPathGroupList(std::ofstream* report_file, DelayType delay_type, StartEndType start_end_type)
{
  Database& database = STADM.getDatabase();
  if (database.get_timing_path_group_list().empty()) {
    (*report_file) << "No constrained paths.\n\n";
    return;
  }
  bool has_timing_path = false;
  for (TimingPathGroup& timing_path_group : database.get_timing_path_group_list()) {
    std::vector<TimingPath*> timing_path_list = getSortedTimingPathList(timing_path_group, delay_type, start_end_type);
    if (!timing_path_list.empty()) {
      has_timing_path = true;
    }
    outputTimingPathGroup(report_file, timing_path_group, delay_type, start_end_type);
  }
  if (!has_timing_path) {
    (*report_file) << "No constrained paths.\n\n";
  }
}

void TimingReporter::outputReportFooter(std::ofstream* report_file)
{
  (*report_file) << "1\n";
}

void TimingReporter::outputTimingPathGroup(std::ofstream* report_file, TimingPathGroup& timing_path_group, DelayType delay_type,
                                           StartEndType start_end_type)
{
  std::vector<TimingPath*> timing_path_list = getReportTimingPathList(timing_path_group, delay_type, start_end_type);
  for (TimingPath* timing_path : timing_path_list) {
    outputTimingPath(report_file, *timing_path, timing_path_group.get_group_name(), delay_type);
  }
}

std::vector<TimingPath*> TimingReporter::getReportTimingPathList(TimingPathGroup& timing_path_group, DelayType delay_type,
                                                                 StartEndType start_end_type)
{
  int32_t path_report_number = STADM.getConfig().path_report_number;
  std::vector<TimingPath*> sorted_timing_path_list = getSortedTimingPathList(timing_path_group, delay_type, start_end_type);
  std::vector<TimingPath*> report_timing_path_list;
  for (TimingPath* timing_path : sorted_timing_path_list) {
    if (static_cast<int32_t>(report_timing_path_list.size()) >= path_report_number) {
      break;
    }
    report_timing_path_list.push_back(timing_path);
  }
  return report_timing_path_list;
}

std::vector<TimingPath*> TimingReporter::getSortedTimingPathList(TimingPathGroup& timing_path_group, DelayType delay_type,
                                                                 StartEndType start_end_type)
{
  std::vector<TimingPath*> timing_path_list;
  for (auto& [end_point, timing_path_end] : timing_path_group.get_timing_path_end_map()) {
    for (TimingPath& timing_path : timing_path_end.get_timing_path_list()) {
      if (isMatchAnalysisType(timing_path, delay_type) && isMatchStartEndType(timing_path, start_end_type)) {
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

void TimingReporter::outputJsonReport(std::string& report_file_path, DelayType delay_type, StartEndType start_end_type)
{
  Database& database = STADM.getDatabase();
  nlohmann::json summary_json = nlohmann::json::array();
  nlohmann::json slack_json = nlohmann::json::array();
  nlohmann::json detail_json = nlohmann::json::array();

  for (TimingPathGroup& timing_path_group : database.get_timing_path_group_list()) {
    std::vector<TimingPath*> timing_path_list = getReportTimingPathList(timing_path_group, delay_type, start_end_type);
    buildSummaryJson(summary_json, timing_path_group, timing_path_list, delay_type);
    buildSlackJson(slack_json, timing_path_group, timing_path_list, delay_type);
    buildDetailJson(detail_json, timing_path_group, timing_path_list, delay_type);
  }

  nlohmann::json report_json;
  report_json["summary"] = summary_json;
  report_json["slack"] = slack_json;
  report_json["detail"] = detail_json;

  std::string json_report_file_path = getJsonReportFilePath(report_file_path);
  std::ofstream* json_report_file = STAUTIL.getOutputFileStream(json_report_file_path);
  (*json_report_file) << report_json.dump(4);
  STAUTIL.closeFileStream(json_report_file);
}

std::string TimingReporter::getJsonReportFilePath(std::string& report_file_path)
{
  std::string json_report_file_path = report_file_path;
  std::string report_suffix = ".rpt";
  json_report_file_path.replace(json_report_file_path.length() - report_suffix.length(), report_suffix.length(), ".json");
  return json_report_file_path;
}

void TimingReporter::buildSummaryJson(nlohmann::json& summary_json, TimingPathGroup& timing_path_group,
                                      std::vector<TimingPath*>& timing_path_list, DelayType delay_type)
{
  for (TimingPath* timing_path : timing_path_list) {
    summary_json.push_back(makeSummaryJson(*timing_path, timing_path_group.get_group_name(), delay_type));
  }
}

nlohmann::json TimingReporter::makeSummaryJson(TimingPath& timing_path, std::string& path_group_name, DelayType delay_type)
{
  nlohmann::json timing_path_json;
  timing_path_json["endpoint"] = getPTPinName(timing_path.get_end_point());
  timing_path_json["clock_group"] = path_group_name;
  timing_path_json["delay_type"] = getDelayTypeName(delay_type);
  timing_path_json["path_delay"] = getPathDelayJsonValue(timing_path);
  timing_path_json["path_required"] = getNumberString(timing_path.get_required_time());
  timing_path_json["cppr"] = getNumberString(timing_path.get_clock_reconvergence_pessimism());
  timing_path_json["slack"] = getNumberString(timing_path.get_slack());
  timing_path_json["freq"] = getFrequencyJsonValue(timing_path, delay_type);
  return timing_path_json;
}

std::string TimingReporter::getPathDelayJsonValue(TimingPath& timing_path)
{
  return STAUTIL.getString(getNumberString(timing_path.get_path_delay()), getTransTypeName(timing_path.get_trans_type()));
}

std::string TimingReporter::getFrequencyJsonValue(TimingPath& timing_path, DelayType delay_type)
{
  std::string clock_name = getClockName(timing_path);
  double clock_period = getClockPeriod(clock_name);
  if (delay_type != DelayType::kMax || clock_period <= STA_ERROR) {
    return "NA";
  }
  double effective_period = clock_period - timing_path.get_slack();
  if (effective_period <= STA_ERROR) {
    return "NA";
  }
  return getNumberString(1000.0 / effective_period);
}

void TimingReporter::buildSlackJson(nlohmann::json& slack_json, TimingPathGroup& timing_path_group,
                                    std::vector<TimingPath*>& timing_path_list, DelayType delay_type)
{
  if (timing_path_list.empty()) {
    return;
  }
  slack_json.push_back(makeSlackJson(timing_path_group, timing_path_list, delay_type));
}

nlohmann::json TimingReporter::makeSlackJson(TimingPathGroup& timing_path_group, std::vector<TimingPath*>& timing_path_list,
                                             DelayType delay_type)
{
  double worst_slack = 0.0;
  double total_negative_slack = 0.0;
  bool has_slack = false;
  for (TimingPath* timing_path : timing_path_list) {
    double slack = timing_path->get_slack();
    if (!has_slack || slack < worst_slack) {
      worst_slack = slack;
      has_slack = true;
    }
    if (slack < 0.0) {
      total_negative_slack += slack;
    }
  }

  nlohmann::json slack_json;
  slack_json["clock"] = timing_path_group.get_group_name();
  slack_json["delay_type"] = getDelayTypeName(delay_type);
  slack_json["TNS"] = getNumberString(total_negative_slack);
  slack_json["WNS"] = getNumberString(worst_slack);
  return slack_json;
}

void TimingReporter::buildDetailJson(nlohmann::json& detail_json, TimingPathGroup& timing_path_group,
                                     std::vector<TimingPath*>& timing_path_list, DelayType delay_type)
{
  for (TimingPath* timing_path : timing_path_list) {
    detail_json.push_back(makeDetailJson(*timing_path, timing_path_group.get_group_name(), delay_type));
  }
}

nlohmann::json TimingReporter::makeDetailJson(TimingPath& timing_path, std::string& path_group_name, DelayType delay_type)
{
  nlohmann::json timing_path_json;
  timing_path_json["clock_field"] = path_group_name;
  timing_path_json["type"] = getDelayTypeName(delay_type);
  timing_path_json["slack"] = getNumberString(timing_path.get_slack());
  timing_path_json["summary"] = makeModuleSummaryJson(timing_path);
  timing_path_json["detail"] = nlohmann::json::array();

  bool is_first_point = true;
  double last_arrival = 0.0;
  for (TimingPathPoint& path_point : timing_path.get_point_list()) {
    if (shouldOutputTimingPoint(timing_path, path_point)) {
      if (is_first_point) {
        timing_path_json["start_point"] = getPointLabel(path_point);
        is_first_point = false;
      }
      double incr_delay = path_point.get_arrival() - last_arrival;
      last_arrival = path_point.get_arrival();
      double path_delay = path_point.get_arrival() + timing_path.get_launch_time();
      timing_path_json["detail"].push_back(makeTimingPointJson(path_point, incr_delay, path_delay));
      timing_path_json["end_point"] = getPointLabel(path_point);
    }
  }
  return timing_path_json;
}

nlohmann::json TimingReporter::makeTimingPointJson(TimingPathPoint& path_point, double incr_delay, double path_delay)
{
  nlohmann::json timing_point_json;
  timing_point_json["name"] = getPointLabel(path_point);
  timing_point_json["incr_delay"] = getNumberString(incr_delay);
  timing_point_json["path_delay"] = STAUTIL.getString(getNumberString(path_delay), getTransTypeName(path_point.get_trans_type()));
  return timing_point_json;
}

nlohmann::json TimingReporter::makeModuleSummaryJson(TimingPath& timing_path)
{
  std::map<std::string, std::pair<int32_t, double>> module_summary_map;
  bool is_failed_extract_module_name = false;
  double last_arrival = 0.0;
  for (TimingPathPoint& path_point : timing_path.get_point_list()) {
    if (!shouldOutputTimingPoint(timing_path, path_point)) {
      continue;
    }
    std::string point_name = getPTPinName(path_point.get_pin_name());
    std::string module_name = getJsonModuleName(point_name);
    if (module_name.empty()) {
      is_failed_extract_module_name = true;
      break;
    }
    double incr_delay = path_point.get_arrival() - last_arrival;
    last_arrival = path_point.get_arrival();
    module_summary_map[module_name].first++;
    module_summary_map[module_name].second += incr_delay;
  }

  nlohmann::json module_summary_json = nlohmann::json::array();
  if (is_failed_extract_module_name) {
    return module_summary_json;
  }
  for (std::pair<const std::string, std::pair<int32_t, double>>& module_summary_pair : module_summary_map) {
    module_summary_json.push_back({{"module", module_summary_pair.first},
                                   {"count", module_summary_pair.second.first},
                                   {"total_delay", module_summary_pair.second.second}});
  }
  return module_summary_json;
}

std::string TimingReporter::getJsonModuleName(std::string& point_name)
{
  std::size_t split_pos = point_name.find('/');
  if (split_pos == std::string::npos) {
    return "";
  }
  return point_name.substr(0, split_pos);
}

bool TimingReporter::isMatchAnalysisType(TimingPath& timing_path, DelayType delay_type)
{
  if (delay_type == DelayType::kMin) {
    return timing_path.get_analysis_type() == AnalysisType::kMin;
  }
  return timing_path.get_analysis_type() == AnalysisType::kMax;
}

bool TimingReporter::isMatchStartEndType(TimingPath& timing_path, StartEndType start_end_type)
{
  if (isPowerGroundPin(timing_path.get_start_point()) || isPowerGroundPin(timing_path.get_end_point())) {
    return false;
  }
  bool start_is_port = isPort(timing_path.get_start_point());
  bool end_is_port = isPort(timing_path.get_end_point());
  if (start_end_type == StartEndType::kInToOut) {
    return start_is_port && end_is_port;
  }
  if (start_end_type == StartEndType::kInToReg) {
    return start_is_port && isRegisterEndPoint(timing_path.get_end_point());
  }
  if (start_end_type == StartEndType::kRegToOut) {
    return isRegisterStartPoint(timing_path.get_start_point()) && end_is_port;
  }
  return isRegisterStartPoint(timing_path.get_start_point()) && isRegisterEndPoint(timing_path.get_end_point());
}

bool TimingReporter::isPort(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  return database.get_pin_map()[pin_name].get_is_port();
}

bool TimingReporter::isRegisterStartPoint(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  return instance.get_is_sequential() && pin_name == instance.get_output_pin_name() && hasClockPoint(instance.get_clock_pin_name());
}

bool TimingReporter::isRegisterEndPoint(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  if (pin.get_is_port() || database.get_instance_map().count(pin.get_instance_name()) == 0) {
    return false;
  }
  Instance& instance = database.get_instance_map()[pin.get_instance_name()];
  if (!instance.get_is_sequential() || !hasClockPoint(instance.get_clock_pin_name())) {
    return false;
  }
  for (TimingCheckArc& timing_check_arc : instance.get_check_arc_list()) {
    if (timing_check_arc.get_data_port() == pin_name) {
      return true;
    }
  }
  return false;
}

bool TimingReporter::hasClockPoint(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  return database.get_timing_point_map().count(pin_name) > 0 && database.get_timing_point_map()[pin_name].get_is_clock_point();
}

bool TimingReporter::isClockSourceStartPoint(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  if (!pin.get_is_port()) {
    return false;
  }
  for (std::pair<const std::string, TimingClock>& clock_pair : database.get_timing_constraint().get_clock_map()) {
    if (STAUTIL.exist(clock_pair.second.get_source_list(), pin_name)) {
      return true;
    }
  }
  return false;
}

bool TimingReporter::isPowerGroundPin(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  return pin.get_pin_name() == "VDD" || pin.get_pin_name() == "VSS" || pin.get_pin_name() == "VDDIO" || pin.get_pin_name() == "VSSIO";
}

void TimingReporter::outputTimingPath(std::ofstream* report_file, TimingPath& timing_path, std::string& path_group_name,
                                      DelayType delay_type)
{
  outputTimingPathHeader(report_file, timing_path, path_group_name, delay_type);
  std::size_t label_width = outputTimingPointList(report_file, timing_path, delay_type);
  outputTimingPathSummary(report_file, timing_path, label_width);
}

void TimingReporter::outputTimingPathHeader(std::ofstream* report_file, TimingPath& timing_path, std::string& path_group_name,
                                            DelayType delay_type)
{
  outputStartEndPoint(report_file, "Startpoint", getStartPointText(timing_path));
  outputStartEndPoint(report_file, "Endpoint", getEndPointText(timing_path));
  if (isRegisterStartPoint(timing_path.get_start_point()) && isRegisterEndPoint(timing_path.get_end_point())) {
    (*report_file) << "  Last common pin: " << getPTPinName(timing_path.get_last_common_pin()) << "\n";
  }
  (*report_file) << "  Path Group: " << path_group_name << "\n";
  (*report_file) << "  Path Type: " << getDelayTypeName(delay_type) << "\n\n";
}

void TimingReporter::outputStartEndPoint(std::ofstream* report_file, std::string label, std::string text)
{
  std::string name = getStartEndPointName(text);
  std::string description = getStartEndPointDescription(text);
  (*report_file) << "  " << label << ": " << name;
  if (!description.empty()) {
    if (name.length() <= 10) {
      (*report_file) << " " << description;
    } else {
      (*report_file) << "\n               " << description;
    }
  }
  (*report_file) << "\n";
}

std::string TimingReporter::getStartEndPointName(std::string& text)
{
  std::size_t description_pos = text.find(" (");
  if (description_pos == std::string::npos) {
    return text;
  }
  return text.substr(0, description_pos);
}

std::string TimingReporter::getStartEndPointDescription(std::string& text)
{
  std::size_t description_pos = text.find(" (");
  if (description_pos == std::string::npos) {
    return "";
  }
  return text.substr(description_pos + 1);
}

std::string TimingReporter::getStartPointText(TimingPath& timing_path)
{
  Database& database = STADM.getDatabase();
  Pin& start_pin = database.get_pin_map()[timing_path.get_start_point()];
  std::string clock_name = getClockName(timing_path);
  std::string start_point = getPTPinName(timing_path.get_start_point());
  if (!start_pin.get_is_port()) {
    Instance& start_instance = database.get_instance_map()[start_pin.get_instance_name()];
    if (start_instance.get_is_sequential() && isInternalStartPoint(timing_path)) {
      start_point = getPTPinName(start_instance.get_clock_pin_name());
      return STAUTIL.getString(start_point, " (internal path startpoint clocked by ", clock_name, ")");
    }
    start_point = start_pin.get_instance_name();
  }
  if (isClockSourceStartPoint(timing_path.get_start_point())) {
    return STAUTIL.getString(start_point, " (clock source '", clock_name, "')");
  }
  if (isPort(timing_path.get_start_point())) {
    return STAUTIL.getString(start_point, " (input port clocked by ", clock_name, ")");
  }
  return STAUTIL.getString(start_point, " (rising edge-triggered flip-flop clocked by ", clock_name, ")");
}

bool TimingReporter::isInternalStartPoint(TimingPath& timing_path)
{
  Database& database = STADM.getDatabase();
  Pin& start_pin = database.get_pin_map()[timing_path.get_start_point()];
  Instance& start_instance = database.get_instance_map()[start_pin.get_instance_name()];
  return timing_path.get_start_point() == start_instance.get_clock_pin_name()
         || (timing_path.get_start_point() == start_instance.get_output_pin_name() && isTieDrivenConstantOutput(start_instance));
}

bool TimingReporter::isTieDrivenConstantOutput(Instance& instance)
{
  std::optional<bool> data_value = getTieDriverValue(instance.get_data_pin_name());
  if (!data_value.has_value()) {
    return false;
  }
  if (instance.get_has_clear_arc() && *data_value) {
    return false;
  }
  if (instance.get_has_preset_arc() && !*data_value) {
    return false;
  }
  return true;
}

std::optional<bool> TimingReporter::getTieDriverValue(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
  Pin& pin = database.get_pin_map()[pin_name];
  Net& net = database.get_net_map()[pin.get_net_name()];
  for (std::string& driver_pin_name : net.get_driver_pin_list()) {
    Pin& driver_pin = database.get_pin_map()[driver_pin_name];
    if (driver_pin.get_is_port()) {
      continue;
    }
    Instance& driver_instance = database.get_instance_map()[driver_pin.get_instance_name()];
    if (isTieHighCell(driver_instance)) {
      return true;
    }
    if (isTieLowCell(driver_instance)) {
      return false;
    }
  }
  return std::nullopt;
}

bool TimingReporter::isTieHighCell(Instance& instance)
{
  std::string& cell_name = instance.get_cell_name();
  return cell_name.rfind("TIEHI", 0) == 0;
}

bool TimingReporter::isTieLowCell(Instance& instance)
{
  std::string& cell_name = instance.get_cell_name();
  return cell_name.rfind("TIELO", 0) == 0;
}

std::string TimingReporter::getEndPointText(TimingPath& timing_path)
{
  Database& database = STADM.getDatabase();
  Pin& end_pin = database.get_pin_map()[timing_path.get_end_point()];
  std::string clock_name = getClockName(timing_path);
  std::string end_point = getPTPinName(timing_path.get_end_point());
  if (!end_pin.get_is_port()) {
    end_point = end_pin.get_instance_name();
  }
  if (isPort(timing_path.get_end_point())) {
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

std::size_t TimingReporter::outputTimingPointList(std::ofstream* report_file, TimingPath& timing_path, DelayType delay_type)
{
  std::size_t label_width = getTimingLineLabelWidth(timing_path, delay_type);
  outputTimingPointHeader(report_file, label_width);
  (*report_file) << "  " << std::string(label_width + 28, '-') << "\n";
  outputLaunchClockInfo(report_file, timing_path, delay_type, label_width);
  bool is_first_point = true;
  for (TimingPathPoint& path_point : timing_path.get_point_list()) {
    if (shouldOutputTimingPoint(timing_path, path_point)) {
      outputTimingPoint(report_file, timing_path, path_point, is_first_point, label_width);
      is_first_point = false;
    }
  }
  outputTimingSummaryLine(report_file, "data arrival time", timing_path.get_path_delay(), label_width);
  (*report_file) << "\n";
  outputRequiredClockInfo(report_file, timing_path, delay_type, label_width);
  return label_width;
}

std::size_t TimingReporter::getTimingLineLabelWidth(TimingPath& timing_path, DelayType delay_type)
{
  Database& database = STADM.getDatabase();
  std::size_t label_width = 35;
  std::string clock_name = getClockName(timing_path);
  updateTimingLineLabelWidth(label_width, STAUTIL.getString("clock ", clock_name, " (rise edge)"));
  updateTimingLineLabelWidth(label_width, "clock network delay (propagated)");

  std::string start_clock_pin = getStartClockPin(timing_path);
  if (!start_clock_pin.empty() && start_clock_pin != timing_path.get_start_point()) {
    updateTimingLineLabelWidth(label_width, getPinLabel(start_clock_pin));
  }

  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  bool has_input_delay = port_constraint_map.count(timing_path.get_start_point()) > 0
                         && (port_constraint_map[timing_path.get_start_point()].get_has_input_delay_max()
                             || port_constraint_map[timing_path.get_start_point()].get_has_input_delay_min());
  if (isPort(timing_path.get_start_point()) && has_input_delay) {
    updateTimingLineLabelWidth(label_width, "input external delay");
  }

  for (TimingPathPoint& path_point : timing_path.get_point_list()) {
    if (shouldOutputTimingPoint(timing_path, path_point)) {
      updateTimingLineLabelWidth(label_width, getPointLabel(path_point));
    }
  }

  updateTimingLineLabelWidth(label_width, "clock reconvergence pessimism");
  if (!timing_path.get_capture_clock_pin().empty()) {
    updateTimingLineLabelWidth(label_width, getPinLabel(timing_path.get_capture_clock_pin()));
  }
  if (std::fabs(timing_path.get_check_time()) > STA_ERROR) {
    updateTimingLineLabelWidth(label_width, getLibraryCheckText(timing_path, delay_type));
  } else if (isPort(timing_path.get_end_point())) {
    bool has_output_delay = port_constraint_map.count(timing_path.get_end_point()) > 0
                            && (port_constraint_map[timing_path.get_end_point()].get_has_output_delay_max()
                                || port_constraint_map[timing_path.get_end_point()].get_has_output_delay_min());
    if (has_output_delay) {
      updateTimingLineLabelWidth(label_width, "output external delay");
    }
  }
  return label_width;
}

bool TimingReporter::shouldOutputTimingPoint(TimingPath& timing_path, TimingPathPoint& path_point)
{
  Database& database = STADM.getDatabase();
  if (isPowerGroundPin(path_point.get_pin_name())) {
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

void TimingReporter::outputLaunchClockInfo(std::ofstream* report_file, TimingPath& timing_path, DelayType delay_type,
                                           std::size_t label_width)
{
  Database& database = STADM.getDatabase();
  std::string clock_name = getClockName(timing_path);
  double launch_time = timing_path.get_launch_time();
  double launch_clock_network_delay = timing_path.get_launch_clock_network_delay();
  double launch_clock_edge = isClockSourceStartPoint(timing_path.get_start_point()) ? launch_time : 0.0;
  outputTimingLine(report_file, STAUTIL.getString("clock ", clock_name, " (", getLaunchClockEdgeText(timing_path, delay_type), " edge)"),
                   launch_clock_edge, launch_clock_edge, true, "", label_width);
  if (isClockSourceStartPoint(timing_path.get_start_point())) {
    outputTimingLine(report_file, "clock source latency", 0.0, launch_time, true, "", label_width);
  } else {
    outputTimingLine(report_file, "clock network delay (propagated)", launch_clock_network_delay, launch_time, true, "", label_width);
  }

  std::string start_clock_pin = getStartClockPin(timing_path);
  if (!start_clock_pin.empty() && start_clock_pin != timing_path.get_start_point()) {
    outputTimingLine(report_file, getPinLabel(start_clock_pin), 0.0, launch_time, true, "r", label_width);
  }

  double input_delay = getInputDelay(timing_path, delay_type);
  std::map<std::string, TimingPortConstraint>& port_constraint_map = database.get_timing_constraint().get_port_constraint_map();
  bool has_input_delay = port_constraint_map.count(timing_path.get_start_point()) > 0
                         && (port_constraint_map[timing_path.get_start_point()].get_has_input_delay_max()
                             || port_constraint_map[timing_path.get_start_point()].get_has_input_delay_min());
  if (isPort(timing_path.get_start_point()) && has_input_delay) {
    outputTimingLine(report_file, "input external delay", input_delay, launch_time + input_delay, true, "r", label_width);
  }
}

std::string TimingReporter::getLaunchClockEdgeText(TimingPath& timing_path, DelayType delay_type)
{
  if (isClockSourceStartPoint(timing_path.get_start_point()) && delay_type == DelayType::kMax && timing_path.get_trans_type() == TransType::kFall) {
    return "fall";
  }
  return "rise";
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

std::string TimingReporter::getClockName(TimingPath& timing_path)
{
  Database& database = STADM.getDatabase();
  if (!timing_path.get_clock_name().empty()) {
    return timing_path.get_clock_name();
  }
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
  if (!clock_map.empty()) {
    return clock_map.begin()->first;
  }
  return "clk";
}

double TimingReporter::getClockPeriod(std::string& clock_name)
{
  Database& database = STADM.getDatabase();
  std::map<std::string, TimingClock>& clock_map = database.get_timing_constraint().get_clock_map();
  if (clock_map.count(clock_name) > 0) {
    return clock_map[clock_name].get_period();
  }
  if (!clock_map.empty()) {
    return clock_map.begin()->second.get_period();
  }
  return 0.0;
}

double TimingReporter::getInputDelay(TimingPath& timing_path, DelayType delay_type)
{
  Database& database = STADM.getDatabase();
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

std::string TimingReporter::getStartClockPin(TimingPath& timing_path)
{
  Database& database = STADM.getDatabase();
  Pin& start_pin = database.get_pin_map()[timing_path.get_start_point()];
  if (start_pin.get_is_port() || database.get_instance_map().count(start_pin.get_instance_name()) == 0) {
    return "";
  }
  return database.get_instance_map()[start_pin.get_instance_name()].get_clock_pin_name();
}

void TimingReporter::outputTimingPoint(std::ofstream* report_file, TimingPath& timing_path, TimingPathPoint& path_point,
                                       bool is_first_point, std::size_t label_width)
{
  Database& database = STADM.getDatabase();
  double arc_delay = path_point.get_arc_delay();
  if (is_first_point && !database.get_pin_map()[path_point.get_pin_name()].get_is_port()) {
    arc_delay = path_point.get_arrival() - timing_path.get_launch_time();
  }
  outputTimingLine(report_file, getPointLabel(path_point), arc_delay, path_point.get_arrival(), true, getTransTypeName(path_point.get_trans_type()),
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

std::string TimingReporter::getPointLabel(TimingPathPoint& path_point)
{
  Database& database = STADM.getDatabase();
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

void TimingReporter::outputRequiredClockInfo(std::ofstream* report_file, TimingPath& timing_path, DelayType delay_type,
                                             std::size_t label_width)
{
  Database& database = STADM.getDatabase();
  std::string clock_name = getClockName(timing_path);
  double capture_time = timing_path.get_capture_time();
  double clock_edge = delay_type == DelayType::kMin ? 0.0 : getClockPeriod(clock_name);
  double capture_clock_network_delay = timing_path.get_capture_clock_network_delay();
  outputTimingLine(report_file, STAUTIL.getString("clock ", clock_name, " (rise edge)"), clock_edge, clock_edge, true, "", label_width);
  outputTimingLine(report_file, "clock network delay (propagated)", capture_clock_network_delay, clock_edge + capture_clock_network_delay, true, "",
                   label_width);
  outputTimingLine(report_file, "clock reconvergence pessimism", timing_path.get_clock_reconvergence_pessimism(), capture_time, true, "", label_width);
  if (!timing_path.get_capture_clock_pin().empty()) {
    outputTimingLine(report_file, getPinLabel(timing_path.get_capture_clock_pin()), 0.0, capture_time, false, "r", label_width);
  }
  if (std::fabs(timing_path.get_check_time()) > STA_ERROR) {
    double check_time = timing_path.get_check_time();
    if (delay_type == DelayType::kMax) {
      check_time = -check_time;
    }
    outputTimingLine(report_file, getLibraryCheckText(timing_path, delay_type), check_time, timing_path.get_required_time(), true, "", label_width);
  } else if (isPort(timing_path.get_end_point())) {
    double output_delay = getOutputDelay(timing_path, delay_type);
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

double TimingReporter::getOutputDelay(TimingPath& timing_path, DelayType delay_type)
{
  Database& database = STADM.getDatabase();
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

std::string TimingReporter::getPinLabel(std::string& pin_name)
{
  Database& database = STADM.getDatabase();
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
