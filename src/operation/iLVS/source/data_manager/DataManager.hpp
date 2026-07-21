#pragma once

#include <any>
#include <map>
#include <string>

#include "Config.hpp"
#include "LVSDatabase.hpp"

namespace ilvs {

#define LVSDM (ilvs::DataManager::getInst())

class DataManager
{
 public:
  static void initInst();
  static DataManager& getInst();
  static void destroyInst();

  // getter
  Config& getConfig() { return _config; }
  const Config& getConfig() const { return _config; }
  LVSDatabase& getDatabase() { return _database; }
  const LVSDatabase& getDatabase() const { return _database; }

  // setter

  // function
  void input(std::map<std::string, std::any>& config_map);
  void output();

 private:
  static DataManager* _dm_instance;
  Config _config;
  LVSDatabase _database;

  DataManager() = default;
  DataManager(const DataManager& other) = delete;
  DataManager(DataManager&& other) = delete;
  ~DataManager() = default;
  DataManager& operator=(const DataManager& other) = delete;
  DataManager& operator=(DataManager&& other) = delete;
  // function
#if 1  // build
  void buildConfig();
  void buildDatabase();
  void printConfig();
  void printDatabase();
#endif
};

}  // namespace ilvs
