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

#include "CcapEntry.hpp"
#include "CornerNetPool.hpp"
#include "CouplingKey.hpp"
#include "CouplingKeyHash.hpp"
#include "TopoPool.hpp"

namespace ircx {

class RCTable
{
 public:
  RCTable() = default;
  ~RCTable() = default;
  // getter
  size_t get_corner_num() const { return _corner_num; }
  size_t get_net_num() const { return _net_num; }
  CornerNetPool<std::vector<double>>& get_corner_net_res_pool() { return _corner_net_res_pool; }
  CornerNetPool<std::vector<double>>& get_corner_net_gcap_pool() { return _corner_net_gcap_pool; }
  std::vector<std::vector<CcapEntry>>& get_net_ccap_entry_list() { return _net_ccap_entry_list; }
  std::unordered_map<CouplingKey, std::vector<double>, CouplingKeyHash>& get_merged_ccap_map() { return _merged_ccap_map; }
  // setter
  void set_corner_num(size_t corner_num) { _corner_num = corner_num; }
  void set_net_num(size_t net_num) { _net_num = net_num; }
  void set_corner_net_res_pool(const CornerNetPool<std::vector<double>>& corner_net_res_pool)
  {
    _corner_net_res_pool = corner_net_res_pool;
  }
  void set_corner_net_gcap_pool(const CornerNetPool<std::vector<double>>& corner_net_gcap_pool)
  {
    _corner_net_gcap_pool = corner_net_gcap_pool;
  }
  void set_net_ccap_entry_list(const std::vector<std::vector<CcapEntry>>& net_ccap_entry_list)
  {
    _net_ccap_entry_list = net_ccap_entry_list;
  }
  void set_merged_ccap_map(const std::unordered_map<CouplingKey, std::vector<double>, CouplingKeyHash>& merged_ccap_map)
  {
    _merged_ccap_map = merged_ccap_map;
  }
  // function
  void init(size_t corner_num, size_t net_num, TopoPool& topo_pool);
  std::span<double> get_corner_net_res_list(CornerNetId corner_net_id);
  std::span<double> get_corner_net_gcap_list(CornerNetId corner_net_id);
  void append_net_ccap_entry(size_t net_idx, size_t first_edge_idx, size_t second_edge_idx, size_t corner_idx, double cap);
  void merge_net_ccap_entry_list();

 private:
  size_t _corner_num = 0;
  size_t _net_num = 0;
  CornerNetPool<std::vector<double>> _corner_net_res_pool;
  CornerNetPool<std::vector<double>> _corner_net_gcap_pool;
  std::vector<std::vector<CcapEntry>> _net_ccap_entry_list;
  std::unordered_map<CouplingKey, std::vector<double>, CouplingKeyHash> _merged_ccap_map;
};

inline void RCTable::init(size_t corner_num, size_t net_num, TopoPool& topo_pool)
{
  _corner_num = corner_num;
  _net_num = net_num;
  _corner_net_res_pool.init(_corner_num, _net_num);
  _corner_net_gcap_pool.init(_corner_num, _net_num);
  _net_ccap_entry_list.assign(_net_num, std::vector<CcapEntry>());
  _merged_ccap_map.clear();

  for (size_t corner_idx = 0; corner_idx < _corner_num; ++corner_idx) {
    for (size_t net_idx = 0; net_idx < _net_num; ++net_idx) {
      CornerNetId corner_net_id(corner_idx, net_idx);
      size_t edge_count = topo_pool.get_net_edge_list(net_idx).size();
      _corner_net_res_pool.get_item(corner_net_id).assign(edge_count, 0.0);
      _corner_net_gcap_pool.get_item(corner_net_id).assign(edge_count, 0.0);
    }
  }
}

inline std::span<double> RCTable::get_corner_net_res_list(CornerNetId corner_net_id)
{
  std::vector<double>& res_list = _corner_net_res_pool.get_item(corner_net_id);
  return std::span<double>(res_list);
}

inline std::span<double> RCTable::get_corner_net_gcap_list(CornerNetId corner_net_id)
{
  std::vector<double>& gcap_list = _corner_net_gcap_pool.get_item(corner_net_id);
  return std::span<double>(gcap_list);
}

inline void RCTable::append_net_ccap_entry(size_t net_idx,
                                           size_t first_edge_idx,
                                           size_t second_edge_idx,
                                           size_t corner_idx,
                                           double cap)
{
  _net_ccap_entry_list.at(net_idx).emplace_back(first_edge_idx, second_edge_idx, corner_idx, cap);
}

inline void RCTable::merge_net_ccap_entry_list()
{
  for (std::vector<CcapEntry>& ccap_entry_list : _net_ccap_entry_list) {
    for (CcapEntry& ccap_entry : ccap_entry_list) {
      CouplingKey coupling_key(ccap_entry.get_first_edge_idx(), ccap_entry.get_second_edge_idx());
      if (_merged_ccap_map.count(coupling_key) == 0) {
        _merged_ccap_map[coupling_key].resize(_corner_num, 0.0);
      }
      _merged_ccap_map[coupling_key].at(ccap_entry.get_corner_idx()) += ccap_entry.get_cap();
    }
    ccap_entry_list.clear();
  }
}

}  // namespace ircx
