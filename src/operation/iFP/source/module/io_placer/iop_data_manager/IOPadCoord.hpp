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
// THIS SOFTWARE IS PROVIDED ON AN \"AS IS\" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
#pragma once

#include "FPHeader.hpp"
#include "IOEdgeType.hpp"

namespace ifp {

class IOPadCoord
{
 public:
  IOPadCoord() = default;
  ~IOPadCoord() = default;
  // getter
  IOEdgeType get_edge_type() const { return _edge_type; }
  std::string& get_orient_name() { return _orient_name; }
  int32_t get_begin_coord() const { return _begin_coord; }
  int32_t get_end_coord() const { return _end_coord; }
  int32_t get_coord() const { return _coord; }
  // const getter
  const std::string& get_orient_name() const { return _orient_name; }

  // setter
  void set_edge_type(IOEdgeType edge_type) { _edge_type = edge_type; }
  void set_orient_name(std::string orient_name) { _orient_name = orient_name; }
  void set_begin_coord(int32_t begin_coord) { _begin_coord = begin_coord; }
  void set_end_coord(int32_t end_coord) { _end_coord = end_coord; }
  void set_coord(int32_t coord) { _coord = coord; }
  // function

 private:
  IOEdgeType _edge_type = IOEdgeType::kNone;
  std::string _orient_name;
  int32_t _begin_coord = -1;
  int32_t _end_coord = -1;
  int32_t _coord = -1;
};

}  // namespace ifp
