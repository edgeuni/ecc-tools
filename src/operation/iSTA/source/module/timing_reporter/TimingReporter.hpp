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
#pragma once

#include "Database.hpp"

namespace ista {

#define STATR (ista::TimingReporter::getInst())

class TimingReporter
{
 private:
  enum class DelayType
  {
    kMax,
    kMin
  };

  enum class StartEndType
  {
    kInToOut,
    kInToReg,
    kRegToOut,
    kRegToReg
  };

 public:
  static void initInst();
  static TimingReporter& getInst();
  static void destroyInst();
  // function
  void report();

 private:
  // self
  static TimingReporter* _tr_instance;

  TimingReporter() = default;
  TimingReporter(const TimingReporter& other) = delete;
  TimingReporter(TimingReporter&& other) = delete;
  ~TimingReporter() = default;
  TimingReporter& operator=(const TimingReporter& other) = delete;
  TimingReporter& operator=(TimingReporter&& other) = delete;
  // function
  void reportTiming(Database& database);
  void outputTimingReportList(Database& database);
  void outputTimingReport(Database& database, DelayType delay_type, StartEndType start_end_type);
  std::string getReportFilePath(DelayType delay_type, StartEndType start_end_type);
  std::string getReportStartEndTypeName(StartEndType start_end_type);
  void outputReportHeader(std::ofstream* report_file, Database& database, DelayType delay_type, StartEndType start_end_type);
  std::string getDelayTypeName(DelayType delay_type);
  std::string getStartEndTypeName(StartEndType start_end_type);
  void outputPathGroupList(std::ofstream* report_file, Database& database, DelayType delay_type, StartEndType start_end_type);
  void outputReportFooter(std::ofstream* report_file);
  void outputTimingPathGroup(std::ofstream* report_file, Database& database, TimingPathGroup& timing_path_group, DelayType delay_type,
                             StartEndType start_end_type);
  std::vector<TimingPath*> getSortedTimingPathList(Database& database, TimingPathGroup& timing_path_group, DelayType delay_type,
                                                   StartEndType start_end_type);
  std::vector<TimingPath*> getEndpointWorstTimingPathList(std::vector<TimingPath*>& timing_path_list);
  bool isMatchAnalysisType(TimingPath& timing_path, DelayType delay_type);
  bool isMatchStartEndType(Database& database, TimingPath& timing_path, StartEndType start_end_type);
  bool isPort(Database& database, std::string& pin_name);
  bool isRegisterStartPoint(Database& database, std::string& pin_name);
  bool isRegisterEndPoint(Database& database, std::string& pin_name);
  bool hasClockPoint(Database& database, std::string& pin_name);
  bool isPowerGroundPin(Database& database, std::string& pin_name);
  void outputTimingPath(std::ofstream* report_file, Database& database, TimingPath& timing_path, std::string& path_group_name, DelayType delay_type);
  void outputTimingPathHeader(std::ofstream* report_file, Database& database, TimingPath& timing_path, std::string& path_group_name, DelayType delay_type);
  std::string getStartPointText(Database& database, TimingPath& timing_path);
  std::string getEndPointText(Database& database, TimingPath& timing_path);
  std::string getEndPointCheckText(std::string& end_point, std::string& clock_name, TimingPath& timing_path);
  std::size_t outputTimingPointList(std::ofstream* report_file, Database& database, TimingPath& timing_path, DelayType delay_type);
  std::size_t getTimingLineLabelWidth(Database& database, TimingPath& timing_path, DelayType delay_type);
  bool shouldOutputTimingPoint(Database& database, TimingPath& timing_path, TimingPathPoint& path_point);
  void updateTimingLineLabelWidth(std::size_t& label_width, std::string label);
  void outputTimingPointHeader(std::ofstream* report_file, std::size_t label_width);
  void outputLaunchClockInfo(std::ofstream* report_file, Database& database, TimingPath& timing_path, DelayType delay_type, std::size_t label_width);
  std::string getTransTypeName(TransType trans_type);
  void outputTimingLine(std::ofstream* report_file, std::string label, double incr, double path, bool has_incr, std::string transition,
                        std::size_t label_width);
  void outputTimingSummaryLine(std::ofstream* report_file, std::string label, double value, std::size_t label_width);
  std::string getClockName(Database& database, TimingPath& timing_path);
  double getClockPeriod(Database& database, std::string& clock_name);
  double getInputDelay(Database& database, TimingPath& timing_path, DelayType delay_type);
  std::string getStartClockPin(Database& database, TimingPath& timing_path);
  void outputTimingPoint(std::ofstream* report_file, Database& database, TimingPath& timing_path, TimingPathPoint& path_point, bool is_first_point,
                         std::size_t label_width);
  std::string getNumberString(double value);
  std::string getPointLabel(Database& database, TimingPathPoint& path_point);
  std::string getPTPinName(std::string& pin_name);
  std::string getPTCellName(TimingPathPoint& path_point);
  void outputRequiredClockInfo(std::ofstream* report_file, Database& database, TimingPath& timing_path, DelayType delay_type, std::size_t label_width);
  std::string getLibraryCheckText(TimingPath& timing_path, DelayType delay_type);
  double getOutputDelay(Database& database, TimingPath& timing_path, DelayType delay_type);
  std::string getPinLabel(Database& database, std::string& pin_name);
  void outputTimingPathSummary(std::ofstream* report_file, TimingPath& timing_path, std::size_t label_width);
  std::string getSlackStatus(TimingPath& timing_path);
};

}  // namespace ista
