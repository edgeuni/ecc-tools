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
#include "TimingCharacterizer.hpp"

#include "DataManager.hpp"
#include "Logger.hpp"
#include "Monitor.hpp"
#include "Utility.hpp"

namespace ista {

// public

void TimingCharacterizer::initInst()
{
  if (_tc_instance == nullptr) {
    _tc_instance = new TimingCharacterizer();
  }
}

TimingCharacterizer& TimingCharacterizer::getInst()
{
  if (_tc_instance == nullptr) {
    STALOG.error(Loc::current(), "The instance not initialized!");
  }
  return *_tc_instance;
}

void TimingCharacterizer::destroyInst()
{
  if (_tc_instance != nullptr) {
    delete _tc_instance;
    _tc_instance = nullptr;
  }
}

// function

void TimingCharacterizer::characterize()
{
  Monitor monitor;
  STALOG.info(Loc::current(), "Starting...");

  Database& database = STADM.getDatabase();
  outputLibFileList(database);

  STALOG.info(Loc::current(), "Completed", monitor.getStatsInfo());
}

void TimingCharacterizer::outputLibFileList(Database& database)
{
  std::string& design_name = database.get_design_name();
  outputLibFile(design_name, "max");
  outputLibFile(design_name, "min");
}

void TimingCharacterizer::outputLibFile(std::string& design_name, std::string analysis_type)
{
  std::string lib_file_path = getLibFilePath(design_name, analysis_type);
  std::ofstream* lib_file = STAUTIL.getOutputFileStream(lib_file_path);
  STAUTIL.closeFileStream(lib_file);
  STALOG.info(Loc::current(), "Output iSTA extracted lib: ", lib_file_path);
}

std::string TimingCharacterizer::getLibFilePath(std::string& design_name, std::string analysis_type)
{
  return STAUTIL.getString(STADM.getConfig().tc_temp_directory_path, design_name, "_", analysis_type, ".lib");
}

// private

TimingCharacterizer* TimingCharacterizer::_tc_instance = nullptr;

}  // namespace ista
