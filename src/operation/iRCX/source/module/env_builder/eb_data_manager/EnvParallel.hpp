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
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// See the Mulan PSL v2 for more details.
// ***************************************************************************************
/**
 * @file ParallelUtils.hh
 * @brief Shared OpenMP thread-count helpers for iRCX tools.
 */
#pragma once

#include <omp.h>

#include "RCXHeader.hpp"

namespace ircx {
namespace env_parallel {

inline int32_t cappedWorkItems(size_t work_items)
{
  constexpr size_t max_thread_count = static_cast<size_t>(INT32_MAX);
  return work_items > max_thread_count ? INT32_MAX : static_cast<int32_t>(work_items);
}

inline int32_t threadCount(size_t work_items,
                           int32_t requested_threads)
{
  if (work_items == 0) {
    return 1;
  }

  int32_t threads = requested_threads > 0 ? requested_threads : 1;
  threads = std::min(threads, static_cast<int32_t>(omp_get_max_threads()));
  return std::min(threads, cappedWorkItems(work_items));
}

inline int32_t threadCount(size_t work_items)
{
  return threadCount(work_items, static_cast<int32_t>(omp_get_max_threads()));
}

inline int32_t requestedThreadCount(size_t work_items,
                                    int32_t requested_threads)
{
  if (work_items == 0) {
    return 1;
  }

  const int32_t threads = requested_threads > 0 ? requested_threads : 1;
  return std::min(threads, cappedWorkItems(work_items));
}

}  // namespace env_parallel
}  // namespace ircx
