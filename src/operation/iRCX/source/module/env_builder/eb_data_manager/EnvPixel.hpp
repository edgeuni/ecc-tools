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
/**
 * @file EnvPixel.hh
 * @brief iRCX module implementation detail.
 */
#pragma once

#include "EnvPixelOverlap.hpp"
#include "LineSegment.hpp"
#include "RCXHeader.hpp"
#include "TopoEdge.hpp"
#include "Utility.hpp"

namespace ircx {

class EnvPixel
{
 public:
  EnvPixel() = default;
  ~EnvPixel() = default;

  // getter
  int32_t get_x0() const { return x0_; }
  int32_t get_y0() const { return y0_; }
  int32_t get_nx() const { return nx_; }
  int32_t get_ny() const { return ny_; }
  int32_t get_dx() const { return dx_; }
  int32_t get_dy() const { return dy_; }

  // setter
  void set_x0(int32_t x0) { x0_ = x0; }
  void set_y0(int32_t y0) { y0_ = y0; }
  void set_nx(int32_t nx) { nx_ = nx; }
  void set_ny(int32_t ny) { ny_ = ny; }
  void set_dx(int32_t dx) { dx_ = dx; }
  void set_dy(int32_t dy) { dy_ = dy; }

  // coordinate mapping
  int32_t coordToXIdx(int32_t coord) const { return (coord - x0_) / dx_; }
  int32_t coordToYIdx(int32_t coord) const { return (coord - y0_) / dy_; }

  int32_t idxToXCoord(int32_t idx) const { return x0_ + idx * dx_; }
  int32_t idxToYCoord(int32_t idx) const { return y0_ + idx * dy_; }

  bool initPixel()
  {
    if (nx_ <= 0 || ny_ <= 0 || dx_ <= 0 || dy_ <= 0) {
      return false;
    }

    pixel_.assign(nx_, std::vector<bool>(ny_, false));
    return true;
  }

  void addEdge(TopoEdge& edge)
  {
    if (pixel_.empty() || pixel_.front().empty()) {
      if (!initPixel()) {
        return;
      }
    }

    const GTLRectInt& rect = edge.get_shape();

    int32_t x0 = RCXUTIL.minX(rect);
    int32_t y0 = RCXUTIL.minY(rect);
    int32_t x1 = RCXUTIL.maxX(rect);
    int32_t y1 = RCXUTIL.maxY(rect);

    if (x0 >= x1 || y0 >= y1) {
      return;
    }

    int32_t x_idx0 = coordToXIdx(x0);
    int32_t y_idx0 = coordToYIdx(y0);
    int32_t x_idx1 = coordToXIdx(x1);
    int32_t y_idx1 = coordToYIdx(y1);

    if (!xValid(x_idx0) || !yValid(y_idx0) || !xValid(x_idx1) || !yValid(y_idx1)) {
      return;
    }

    for (int32_t i = x_idx0; i <= x_idx1; ++i) {
      for (int32_t j = y_idx0; j <= y_idx1; ++j) {
        pixel_[i][j] = true;
      }
    }
  }

  std::vector<EnvPixelOverlap> overlap(const LineSegment& line_seg) const
  {
    std::vector<EnvPixelOverlap> ret;
    if (pixel_.empty() || pixel_.front().empty()) {
      return ret;
    }

    bool is_horz = line_seg.get_is_horizontal();
    int32_t fixed = line_seg.get_coordinate();
    int32_t a0 = line_seg.get_lower();
    int32_t a1 = line_seg.get_upper();

    RCXUTIL.normalizeInterval(a0, a1);

    if (is_horz) {
      const int32_t fixed_y_idx = coordToYIdx(fixed);
      if (!yValid(fixed_y_idx)) {
        return ret;
      }
    }

    if (!is_horz) {
      const int32_t fixed_x_idx = coordToXIdx(fixed);
      if (!xValid(fixed_x_idx)) {
        return ret;
      }
      return collectConductorRuns(a0, a1, fixed_x_idx, false);
    }

    return collectConductorRuns(a0, a1, coordToYIdx(fixed), true);
  }

 private:
  std::vector<EnvPixelOverlap> collectConductorRuns(int32_t a0, int32_t a1, int32_t fixed_idx, bool is_horz) const
  {
    std::vector<EnvPixelOverlap> ret;
    if (a0 >= a1) {
      return ret;
    }

    const int32_t a0_idx = getAxisIndex(a0, is_horz);
    const int32_t a1_idx = getAxisIndex(a1, is_horz) + 1;
    if (a0_idx > a1_idx) {
      return ret;
    }

    bool current_type = getPixelOrEmpty(a0_idx, fixed_idx, is_horz);
    int32_t run_start = a0_idx;

    for (int32_t idx = a0_idx + 1; idx <= a1_idx; ++idx) {
      const bool cell = getPixelOrEmpty(idx, fixed_idx, is_horz);
      if (cell != current_type) {
        if (current_type) {
          appendConductorRun(ret, a0, a1, is_horz, run_start, idx);
        }

        run_start = idx;
        current_type = cell;
      }
    }

    if (current_type) {
      appendConductorRun(ret, a0, a1, is_horz, run_start, a1_idx + 1);
    }

    return ret;
  }

  void appendConductorRun(std::vector<EnvPixelOverlap>& pixel_overlap_list, int32_t a0, int32_t a1, bool is_horz, int32_t start_idx,
                          int32_t end_idx_exclusive) const
  {
    if (end_idx_exclusive <= start_idx) {
      return;
    }

    const int32_t lo = RCXUTIL.getIntervalMidpoint(getAxisCoordinate(start_idx, is_horz), getAxisCoordinate(start_idx + 1, is_horz));
    const int32_t hi
        = RCXUTIL.getIntervalMidpoint(getAxisCoordinate(end_idx_exclusive - 1, is_horz), getAxisCoordinate(end_idx_exclusive, is_horz));
    EnvPixelOverlap pixel_overlap = clipPixelOverlap(lo, hi, a0, a1);
    if (!pixel_overlap.empty()) {
      pixel_overlap_list.push_back(pixel_overlap);
    }
  }

  EnvPixelOverlap clipPixelOverlap(int32_t coordinate_lo, int32_t coordinate_hi, int32_t a0, int32_t a1) const
  {
    EnvPixelOverlap pixel_overlap;
    pixel_overlap.set_start_coordinate(std::max(coordinate_lo, a0));
    pixel_overlap.set_end_coordinate(std::min(coordinate_hi, a1));
    return pixel_overlap;
  }

  bool getPixelOrEmpty(int32_t idx, int32_t fixed_idx, bool is_horz) const
  {
    if (!isAxisValid(idx, is_horz)) {
      return false;
    }
    return is_horz ? pixel_[idx][fixed_idx] : pixel_[fixed_idx][idx];
  }

  int32_t getAxisIndex(int32_t coordinate, bool is_horz) const { return is_horz ? coordToXIdx(coordinate) : coordToYIdx(coordinate); }
  int32_t getAxisCoordinate(int32_t idx, bool is_horz) const { return is_horz ? idxToXCoord(idx) : idxToYCoord(idx); }
  bool isAxisValid(int32_t idx, bool is_horz) const { return is_horz ? xValid(idx) : yValid(idx); }

 private:
  std::vector<std::vector<bool>> pixel_;  // true: conductor, false: empty

  int32_t x0_ = 0;
  int32_t y0_ = 0;
  int32_t nx_ = 0;
  int32_t ny_ = 0;
  int32_t dx_ = 0;
  int32_t dy_ = 0;

  bool xValid(int32_t x) const { return 0 <= x && x < nx_; }
  bool yValid(int32_t y) const { return 0 <= y && y < ny_; }
};

}  // namespace ircx
