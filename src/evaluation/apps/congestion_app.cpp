/*
 * @FilePath: congestion_app.cpp
 * @Author: Yihang Qiu (qiuyihang23@mails.ucas.ac.cn)
 * @Date: 2024-08-24 15:37:27
 * @Description:
 */
#include "utility/logger/Logger.hpp"
#include <iostream>

#include "congestion_api.h"
#include "idm.h"

void TestEgrMap();
void TestRudyMap();
void TestEgrOverflow();
void TestRudyUtilization();
void TestRudyMapFromIDB(const std::string& db_config_path);
void TestEgrMapFromIDB(const std::string& db_config_path);

void PrintUsage(const char* program_name)
{
  IEDALOG.info(ieda::Loc::current(), "Congestion Evaluation");
  IEDALOG.info(ieda::Loc::current(), "Usage: ", program_name, " <function_name>");
  IEDALOG.info(ieda::Loc::current(), "Available parameters:");
  IEDALOG.info(ieda::Loc::current(), "  <map_config_path> Path to the database configuration file.");
  IEDALOG.info(ieda::Loc::current(), "  --help, -h       Show this help message and exit.");
}

int main(const int argc, const char* argv[])
{
  if (argc == 2) {
    if (const std::string arg = argv[1]; arg == "--help" || arg == "-h") {
      PrintUsage(argv[0]);
      return 0;
    } else {
      IEDALOG.info(ieda::Loc::current(), "map_path: ", arg);
      TestRudyMapFromIDB(arg);
      // Here are some test functions that can be uncommented to run
      // TestEgrMapFromIDB(arg);
      // TestEgrMap();
      // TestRudyMap();
      // TestEgrOverflow();
      // TestRudyUtilization();
      return 0;
    }
  }
  IEDALOG.warn(ieda::Loc::current(), "Error: Incorrect number of arguments.");
  PrintUsage(argv[0]);
  return 1;

  IEDALOG.warn(ieda::Loc::current(), "Error: Incorrect number of arguments.");
  PrintUsage(argv[0]);
  return 1;
}

void TestEgrDataStructure()
{
  // specify the path to the early router output directory
  std::string congestion_dir = "./rt/rt_temp_directory/early_router";

  ieval::CongestionAPI api;
  std::map<std::string, std::vector<std::vector<int>>> egr_map = api.getEGRMap();

  for (const auto& pair : egr_map) {
    IEDALOG.info(ieda::Loc::current(), "Layer: ", pair.first);
    const auto& matrix = pair.second;

    for (size_t i = 0; i < matrix.size() && i < 3; ++i) {
      for (const auto& value : matrix[i]) {
        IEDALOG.info(ieda::Loc::current(), value, " ");
      }
      IEDALOG.info(ieda::Loc::current(), "");
    }
    IEDALOG.info(ieda::Loc::current(), "");
  }
}

void TestEgrMap()
{
  ieval::CongestionAPI congestion_api;

  // specify the path to the early router output directory
  std::string map_path = "./rt_temp_directory";
  std::string stage = "place";

  ieval::EGRMapSummary egr_map_summary = congestion_api.egrMap(stage, map_path);
  IEDALOG.info(ieda::Loc::current(), "egr horizontal sum: ", egr_map_summary.horizontal_sum);
  IEDALOG.info(ieda::Loc::current(), "egr vertical sum: ", egr_map_summary.vertical_sum);
  IEDALOG.info(ieda::Loc::current(), "egr union sum: ", egr_map_summary.union_sum);
}

void TestRudyMap()
{
  ieval::CongestionAPI congestion_api;

  ieval::CongestionNets congestion_nets;
  ieval::CongestionNet congestion_net_1;
  ieval::CongestionNet congestion_net_2;

  ieval::CongestionPin congestion_pin_1;
  ieval::CongestionPin congestion_pin_2;
  ieval::CongestionPin congestion_pin_3;
  ieval::CongestionPin congestion_pin_4;

  congestion_pin_1.lx = 1;
  congestion_pin_1.ly = 1;
  congestion_pin_2.lx = 5;
  congestion_pin_2.ly = 7;
  congestion_pin_3.lx = 3;
  congestion_pin_3.ly = 3;
  congestion_pin_4.lx = 7;
  congestion_pin_4.ly = 5;

  congestion_net_1.pins.push_back(congestion_pin_1);
  congestion_net_1.pins.push_back(congestion_pin_2);
  congestion_net_2.pins.push_back(congestion_pin_3);
  congestion_net_2.pins.push_back(congestion_pin_4);

  congestion_nets.push_back(congestion_net_1);
  congestion_nets.push_back(congestion_net_2);

  ieval::CongestionRegion region;
  region.lx = 0;
  region.ly = 0;
  region.ux = 10;
  region.uy = 7;

  int32_t grid_size = 2;
  std::string stage = "place";

  ieval::RUDYMapSummary rudy_map_summary = congestion_api.rudyMap(stage, congestion_nets, region, grid_size);
  IEDALOG.info(ieda::Loc::current(), "rudy horizontal: ", rudy_map_summary.rudy_horizontal);
  IEDALOG.info(ieda::Loc::current(), "rudy vertical: ", rudy_map_summary.rudy_vertical);
  IEDALOG.info(ieda::Loc::current(), "rudy union: ", rudy_map_summary.rudy_union);
  IEDALOG.info(ieda::Loc::current(), "lut rudy horizontal: ", rudy_map_summary.lutrudy_horizontal);
  IEDALOG.info(ieda::Loc::current(), "lut rudy vertical: ", rudy_map_summary.lutrudy_vertical);
  IEDALOG.info(ieda::Loc::current(), "lut rudy union: ", rudy_map_summary.lutrudy_union);
}

void TestEgrOverflow()
{
  ieval::CongestionAPI congestion_api;

  // specify the path to the early router output directory
  std::string map_path = "./rt/rt_temp_directory";
  std::string stage = "place";

  ieval::OverflowSummary overflow_summary;
  overflow_summary = congestion_api.egrOverflow(stage, map_path);
  IEDALOG.info(ieda::Loc::current(), "total overflow horizontal: ", overflow_summary.total_overflow_horizontal);
  IEDALOG.info(ieda::Loc::current(), "total overflow vertical: ", overflow_summary.total_overflow_vertical);
  IEDALOG.info(ieda::Loc::current(), "total overflow union: ", overflow_summary.total_overflow_union);
  IEDALOG.info(ieda::Loc::current(), "max overflow horizontal: ", overflow_summary.max_overflow_horizontal);
  IEDALOG.info(ieda::Loc::current(), "max overflow vertical: ", overflow_summary.max_overflow_vertical);
  IEDALOG.info(ieda::Loc::current(), "max overflow union: ", overflow_summary.max_overflow_union);
  IEDALOG.info(ieda::Loc::current(), "weighted average overflow horizontal: ", overflow_summary.weighted_average_overflow_horizontal);
  IEDALOG.info(ieda::Loc::current(), "weighted average overflow vertical: ", overflow_summary.weighted_average_overflow_vertical);
  IEDALOG.info(ieda::Loc::current(), "weighted average overflow union: ", overflow_summary.weighted_average_overflow_union);
}

void TestRudyUtilization()
{
  ieval::CongestionAPI congestion_api;
  std::string stage = "place";

  // specify the path to the rudy map directory
  std::string map_path = "./map/";
  
  ieval::UtilizationSummary utilization_summary;
  utilization_summary = congestion_api.rudyUtilization(stage, map_path, false);
  IEDALOG.info(ieda::Loc::current(), "max utilization horizontal: ", utilization_summary.max_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "max utilization vertical: ", utilization_summary.max_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "max utilization union: ", utilization_summary.max_utilization_union);
  IEDALOG.info(ieda::Loc::current(), "average utilization horizontal: ", utilization_summary.weighted_average_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "average utilization vertical: ", utilization_summary.weighted_average_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "average utilization union: ", utilization_summary.weighted_average_utilization_union);

  utilization_summary = congestion_api.rudyUtilization(stage, map_path, true);
  IEDALOG.info(ieda::Loc::current(), "max utilization horizontal: ", utilization_summary.max_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "max utilization vertical: ", utilization_summary.max_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "max utilization union: ", utilization_summary.max_utilization_union);
  IEDALOG.info(ieda::Loc::current(), "average utilization horizontal: ", utilization_summary.weighted_average_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "average utilization vertical: ", utilization_summary.weighted_average_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "average utilization union: ", utilization_summary.weighted_average_utilization_union);
}

void TestRudyMapFromIDB(const std::string& db_config_path)
{
  dmInst->init(db_config_path);

  std::string stage = "place";

  ieval::CongestionAPI congestion_api;
  ieval::RUDYMapSummary rudy_map_summary = congestion_api.rudyMap(stage);

  IEDALOG.info(ieda::Loc::current(), "rudy horizontal: ", rudy_map_summary.rudy_horizontal);
  IEDALOG.info(ieda::Loc::current(), "rudy vertical: ", rudy_map_summary.rudy_vertical);
  IEDALOG.info(ieda::Loc::current(), "rudy union: ", rudy_map_summary.rudy_union);
  IEDALOG.info(ieda::Loc::current(), "lut rudy horizontal: ", rudy_map_summary.lutrudy_horizontal);
  IEDALOG.info(ieda::Loc::current(), "lut rudy vertical: ", rudy_map_summary.lutrudy_vertical);
  IEDALOG.info(ieda::Loc::current(), "lut rudy union: ", rudy_map_summary.lutrudy_union);

  ieval::UtilizationSummary utilization_summary;
  utilization_summary = congestion_api.rudyUtilization(stage, false);
  IEDALOG.info(ieda::Loc::current(), ">>  RUDY ");
  IEDALOG.info(ieda::Loc::current(), "max utilization horizontal: ", utilization_summary.max_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "max utilization vertical: ", utilization_summary.max_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "max utilization union: ", utilization_summary.max_utilization_union);
  IEDALOG.info(ieda::Loc::current(), "average utilization horizontal: ", utilization_summary.weighted_average_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "average utilization vertical: ", utilization_summary.weighted_average_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "average utilization union: ", utilization_summary.weighted_average_utilization_union);

  utilization_summary = congestion_api.rudyUtilization(stage, true);
  IEDALOG.info(ieda::Loc::current(), ">>  LUTRUDY ");
  IEDALOG.info(ieda::Loc::current(), "max utilization horizontal: ", utilization_summary.max_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "max utilization vertical: ", utilization_summary.max_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "max utilization union: ", utilization_summary.max_utilization_union);
  IEDALOG.info(ieda::Loc::current(), "average utilization horizontal: ", utilization_summary.weighted_average_utilization_horizontal);
  IEDALOG.info(ieda::Loc::current(), "average utilization vertical: ", utilization_summary.weighted_average_utilization_vertical);
  IEDALOG.info(ieda::Loc::current(), "average utilization union: ", utilization_summary.weighted_average_utilization_union);
}

void TestEgrMapFromIDB(const std::string& db_config_path)
{
  dmInst->init(db_config_path);
  std::string stage = "place";

  ieval::CongestionAPI congestion_api;
  ieval::OverflowSummary overflow_summary;
  ieval::EGRMapSummary egr_map_summary = congestion_api.egrMap(stage);
  overflow_summary = congestion_api.egrOverflow(stage);

  IEDALOG.info(ieda::Loc::current(), "egr horizontal sum: ", egr_map_summary.horizontal_sum);
  IEDALOG.info(ieda::Loc::current(), "egr vertical sum: ", egr_map_summary.vertical_sum);
  IEDALOG.info(ieda::Loc::current(), "egr union sum: ", egr_map_summary.union_sum);

  IEDALOG.info(ieda::Loc::current(), "total overflow horizontal: ", overflow_summary.total_overflow_horizontal);
  IEDALOG.info(ieda::Loc::current(), "total overflow vertical: ", overflow_summary.total_overflow_vertical);
  IEDALOG.info(ieda::Loc::current(), "total overflow union: ", overflow_summary.total_overflow_union);
  IEDALOG.info(ieda::Loc::current(), "max overflow horizontal: ", overflow_summary.max_overflow_horizontal);
  IEDALOG.info(ieda::Loc::current(), "max overflow vertical: ", overflow_summary.max_overflow_vertical);
  IEDALOG.info(ieda::Loc::current(), "max overflow union: ", overflow_summary.max_overflow_union);
  IEDALOG.info(ieda::Loc::current(), "weighted average overflow horizontal: ", overflow_summary.weighted_average_overflow_horizontal);
  IEDALOG.info(ieda::Loc::current(), "weighted average overflow vertical: ", overflow_summary.weighted_average_overflow_vertical);
  IEDALOG.info(ieda::Loc::current(), "weighted average overflow union: ", overflow_summary.weighted_average_overflow_union);
}