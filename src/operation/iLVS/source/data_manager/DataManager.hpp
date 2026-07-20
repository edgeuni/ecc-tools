#pragma once

#include <any>
#include <map>
#include <string>

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
  LVSDatabase& getDatabase() { return _database; }
  const LVSDatabase& getDatabase() const { return _database; }

  // setter

  // function
  void input(std::map<std::string, std::any>& config_map);
  void output();

 private:
  static DataManager* _dm_instance;
  LVSDatabase _database;

  DataManager() = default;
  DataManager(const DataManager& other) = delete;
  DataManager(DataManager&& other) = delete;
  ~DataManager() = default;
  DataManager& operator=(const DataManager& other) = delete;
  DataManager& operator=(DataManager&& other) = delete;
  // function
};

}  // namespace ilvs
