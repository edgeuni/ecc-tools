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
  void outputReportHeader(std::ofstream* report_file, Database& database, DelayType delay_type, StartEndType start_end_type);
  std::string getDelayTypeName(DelayType delay_type);
  std::string getStartEndTypeName(StartEndType start_end_type);
  std::string getReportDate();
  void outputPathGroupList(std::ofstream* report_file, Database& database, DelayType delay_type, StartEndType start_end_type);
  void outputReportFooter(std::ofstream* report_file);
  void outputTimingPathGroup(std::ofstream* report_file, Database& database, TimingPathGroup& timing_path_group,
                             DelayType delay_type, StartEndType start_end_type);
  std::vector<TimingPath*> getSortedTimingPathList(Database& database, TimingPathGroup& timing_path_group,
                                                   StartEndType start_end_type);
  bool isMatchStartEndType(Database& database, TimingPath& timing_path, StartEndType start_end_type);
  bool isPort(Database& database, std::string& pin_name);
  void outputTimingPath(std::ofstream* report_file, Database& database, TimingPath& timing_path, std::string& path_group_name,
                        DelayType delay_type);
  void outputTimingPathHeader(std::ofstream* report_file, Database& database, TimingPath& timing_path, std::string& path_group_name,
                              DelayType delay_type);
  std::string getStartPointText(Database& database, TimingPath& timing_path);
  std::string getEndPointText(Database& database, TimingPath& timing_path);
  void outputTimingPointList(std::ofstream* report_file, TimingPath& timing_path);
  void outputTimingPoint(std::ofstream* report_file, TimingPathPoint& path_point);
  std::string getNumberString(double value);
  std::string getPointLabel(TimingPathPoint& path_point);
  std::string getPTPinName(std::string& pin_name);
  std::string getPTCellName(TimingPathPoint& path_point);
  void outputTimingPathSummary(std::ofstream* report_file, TimingPath& timing_path);
  std::string getSlackStatus(TimingPath& timing_path);
};

}  // namespace ista
