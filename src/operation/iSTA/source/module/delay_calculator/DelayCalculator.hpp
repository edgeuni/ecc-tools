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
// WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "Database.hpp"

namespace ista {

#define STADC (ista::DelayCalculator::getInst())

class DelayCalculator
{
 public:
  static void initInst();
  static DelayCalculator& getInst();
  static void destroyInst();
  // function
  bool build();
  double calcArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcArcSlew(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcTimingCellArcDelay(Database& database, std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                                TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcTimingCellArcSlew(Database& database, std::string& output_pin, TimingCellArc& timing_cell_arc, AnalysisType analysis_type,
                               TransType input_trans_type, TransType output_trans_type, double input_slew);
  double calcTimingCheckArcTime(TimingCheckArc& timing_check_arc, AnalysisType analysis_type, TransType clock_trans_type, TransType data_trans_type,
                                double clock_slew, double data_slew);

 private:
  // self
  static DelayCalculator* _dc_instance;

  DelayCalculator() = default;
  DelayCalculator(const DelayCalculator& other) = delete;
  DelayCalculator(DelayCalculator&& other) = delete;
  ~DelayCalculator() = default;
  DelayCalculator& operator=(const DelayCalculator& other) = delete;
  DelayCalculator& operator=(DelayCalculator&& other) = delete;
  // function
  void buildArcDelayList(Database& database);
  void buildArcDelay(Database& database, Arc& arc);
  void buildAnalysisArcDelay(Database& database, Arc& arc, AnalysisType analysis_type);
  void buildTransArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  bool isClockArcTriggerTrans(TimingCellArc& timing_cell_arc, TransType input_trans_type);
  double calcArcDelay(Database& database, Arc& arc);
  double calcCellArcDelay(Database& database, Arc& arc, AnalysisType analysis_type);
  double calcCellArcDelay(Database& database, Arc& arc, AnalysisType analysis_type, TransType input_trans_type);
  TimingCellArc* getTimingCellArc(Database& database, Arc& arc);
  double calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type);
  double calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type);
  double calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                TransType output_trans_type);
  double calcTimingCellArcDelay(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                TransType output_trans_type, double input_slew);
  double calcTimingCellArcSlew(Database& database, Arc& arc, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                               TransType output_trans_type, double input_slew);
  double calcTimingCellArcDelay(Database& database, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                                TransType output_trans_type, double input_slew, double output_load);
  double calcTimingCellArcSlew(Database& database, TimingCellArc& timing_cell_arc, AnalysisType analysis_type, TransType input_trans_type,
                               TransType output_trans_type, double input_slew, double output_load);
  double recoverTableSlew(idb::LibArcSet* lib_arc_set, double output_slew);
  idb::TransType getIDBTransType(TransType trans_type);
  TransType getTransType(idb::TransType trans_type);
  idb::TransType getOutputTransType(idb::LibArcSet* lib_arc_set, idb::TransType input_trans_type);
  std::vector<TransType> getOutputTransTypeList(TimingCellArc& timing_cell_arc, TransType input_trans_type);
  double convertOutputLoad(idb::LibArcSet* lib_arc_set, double output_load);
  double getArcOutputLoad(Database& database, Arc& arc, AnalysisType analysis_type, TransType output_trans_type);
  double getOutputPinLoad(Database& database, std::string& output_pin, AnalysisType analysis_type, TransType output_trans_type);
  double getNetOutputLoad(Database& database, Net& net, AnalysisType analysis_type, TransType output_trans_type);
  double getPinCapacitance(Database& database, std::string& pin_name, AnalysisType analysis_type, TransType trans_type);
  double convertCheckSlewForLookup(TimingCheckArc& timing_check_arc, double data_slew);
  double calcNetArcDelay(Database& database, Arc& arc);
  double calcParasiticDelay(Database& database, Arc& arc);
  double getParasiticNodeCapacitance(ParasiticNet& parasitic_net, std::string& pin_name);
  double getParasiticTotalResistance(ParasiticNet& parasitic_net);
};

}  // namespace ista
