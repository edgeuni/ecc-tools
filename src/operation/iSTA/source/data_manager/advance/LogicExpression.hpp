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

#include "LogicExpressionTerm.hpp"
#include "PowerActivity.hpp"
#include "STAHeader.hpp"

namespace ista {

class LogicExpression
{
 public:
  LogicExpression() = default;
  ~LogicExpression() = default;
  // getter
  std::vector<LogicExpressionTerm>& get_term_list() { return _term_list; }
  bool get_is_empty() const { return _term_list.empty(); }
  // setter
  void set_term_list(const std::vector<LogicExpressionTerm>& term_list) { _term_list = term_list; }
  // function
  PowerActivity evaluate_activity(std::map<std::string, PowerActivity>& port_activity_map)
  {
    std::vector<PowerActivity> activity_stack;
    for (LogicExpressionTerm& term : _term_list) {
      LogicOperationType operation_type = term.get_operation_type();
      if (operation_type == LogicOperationType::kPort) {
        activity_stack.push_back(get_port_activity(term.get_port_name(), port_activity_map));
        continue;
      }
      if (operation_type == LogicOperationType::kOne) {
        activity_stack.push_back(get_constant_activity(1.0));
        continue;
      }
      if (operation_type == LogicOperationType::kZero) {
        activity_stack.push_back(get_constant_activity(0.0));
        continue;
      }
      if (operation_type == LogicOperationType::kNot) {
        if (activity_stack.empty()) {
          return PowerActivity();
        }
        PowerActivity input_activity = activity_stack.back();
        activity_stack.pop_back();
        activity_stack.push_back(get_not_activity(input_activity));
        continue;
      }
      if (activity_stack.size() < 2) {
        return PowerActivity();
      }
      PowerActivity right_activity = activity_stack.back();
      activity_stack.pop_back();
      PowerActivity left_activity = activity_stack.back();
      activity_stack.pop_back();
      if (operation_type == LogicOperationType::kOr) {
        activity_stack.push_back(get_or_activity(left_activity, right_activity));
      } else if (operation_type == LogicOperationType::kAnd) {
        activity_stack.push_back(get_and_activity(left_activity, right_activity));
      } else if (operation_type == LogicOperationType::kXor) {
        activity_stack.push_back(get_xor_activity(left_activity, right_activity));
      } else {
        return PowerActivity();
      }
    }
    if (activity_stack.size() != 1) {
      return PowerActivity();
    }
    return activity_stack.back();
  }

 private:
  PowerActivity get_port_activity(std::string& port_name, std::map<std::string, PowerActivity>& port_activity_map)
  {
    if (port_activity_map.count(port_name) == 0) {
      return PowerActivity();
    }
    return port_activity_map[port_name];
  }

  PowerActivity get_constant_activity(const double static_probability)
  {
    PowerActivity activity;
    activity.set_static_probability(static_probability);
    activity.set_origin(PowerActivityOrigin::kConstant);
    activity.set_is_valid(true);
    return activity;
  }

  PowerActivity get_not_activity(PowerActivity& input_activity)
  {
    PowerActivity activity;
    activity.set_rise_transition_density(input_activity.get_fall_transition_density());
    activity.set_fall_transition_density(input_activity.get_rise_transition_density());
    activity.set_static_probability(1.0 - input_activity.get_static_probability());
    activity.set_origin(PowerActivityOrigin::kPropagated);
    activity.set_is_valid(input_activity.get_is_valid());
    return activity;
  }

  PowerActivity get_or_activity(PowerActivity& left_activity, PowerActivity& right_activity)
  {
    double left_probability = left_activity.get_static_probability();
    double right_probability = right_activity.get_static_probability();
    double transition_density = left_activity.get_transition_density() * (1.0 - right_probability)
                                + right_activity.get_transition_density() * (1.0 - left_probability);
    return get_binary_activity(transition_density, left_probability + right_probability - left_probability * right_probability,
                               left_activity, right_activity);
  }

  PowerActivity get_and_activity(PowerActivity& left_activity, PowerActivity& right_activity)
  {
    double left_probability = left_activity.get_static_probability();
    double right_probability = right_activity.get_static_probability();
    double transition_density
        = left_activity.get_transition_density() * right_probability + right_activity.get_transition_density() * left_probability;
    return get_binary_activity(transition_density, left_probability * right_probability, left_activity, right_activity);
  }

  PowerActivity get_xor_activity(PowerActivity& left_activity, PowerActivity& right_activity)
  {
    double left_probability = left_activity.get_static_probability();
    double right_probability = right_activity.get_static_probability();
    double transition_density = left_activity.get_transition_density() + right_activity.get_transition_density();
    double static_probability = left_probability * (1.0 - right_probability) + (1.0 - left_probability) * right_probability;
    return get_binary_activity(transition_density, static_probability, left_activity, right_activity);
  }

  PowerActivity get_binary_activity(const double transition_density, const double static_probability, PowerActivity& left_activity,
                                    PowerActivity& right_activity)
  {
    PowerActivity activity;
    activity.set_transition_density(transition_density);
    activity.set_static_probability(static_probability);
    activity.set_origin(PowerActivityOrigin::kPropagated);
    activity.set_is_valid(left_activity.get_is_valid() || right_activity.get_is_valid());
    return activity;
  }

  std::vector<LogicExpressionTerm> _term_list;
};

}  // namespace ista
