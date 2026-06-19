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

#include "Logger.hpp"
#include "STAHeader.hpp"

namespace ista {

#define STAUTIL (ista::Utility::getInst())

class Utility
{
 public:
  static void initInst();
  static Utility& getInst();
  static void destroyInst();
  // function

#if 1  // std数据结构工具函数

  template <typename T, typename... Args>
  static std::string getString(T value, Args... args)
  {
    std::stringstream oss;
    pushStream(oss, value, args...);
    std::string string = oss.str();
    oss.clear();
    return string;
  }

  static std::string formatSec(double seconds);
  static std::string formatByTwoDecimalPlaces(double value);
  template <typename T>
  static T getConfigValue(std::map<std::string, std::any>& config_map, const std::string& config_name, const T& default_value)
  {
    if (config_map.find(config_name) != config_map.end()) {
      return std::any_cast<T>(config_map[config_name]);
    }
    STALOG.warn(Loc::current(), "The config '", config_name, "' uses the default value!");
    return default_value;
  }
  static void createDirByFile(std::string file_path)
  {
    std::filesystem::path parent_path = std::filesystem::path(file_path).parent_path();
    if (!parent_path.empty()) {
      createDir(parent_path.string());
    }
  }
  static void createDir(std::string dir_path)
  {
    if (!std::filesystem::exists(dir_path)) {
      std::error_code system_error;
      if (!std::filesystem::create_directories(dir_path, system_error)) {
        STALOG.error(Loc::current(), "Failed to create directory '", dir_path, "', system_error:", system_error.message());
      }
    }
  }
  static void removeDir(const std::string& dir_path)
  {
    std::error_code system_error;
    if (std::filesystem::exists(dir_path, system_error) && !std::filesystem::remove_all(dir_path, system_error)) {
      STALOG.error(Loc::current(), "Failed to remove directory '", dir_path, "'. Error: ", system_error.message());
    }
  }

  static std::string getSpaceByTabNum(int32_t tab_num)
  {
    std::string all = "";
    for (int32_t i = 0; i < tab_num; i++) {
      all += "  ";
    }
    return all;
  }

#endif

 private:
  static Utility* _util_instance;

  Utility() = default;
  Utility(const Utility& other) = delete;
  Utility(Utility&& other) = delete;
  ~Utility() = default;
  Utility& operator=(const Utility& other) = delete;
  Utility& operator=(Utility&& other) = delete;
  // function

  template <typename Stream, typename T, typename... Args>
  static void pushStream(Stream& stream, T t, const Args&... args)
  {
    stream << t;
    pushStream(stream, args...);
  }

  template <typename Stream, typename T>
  static void pushStream(Stream& stream, T t)
  {
    stream << t;
  }
};

}  // namespace ista
