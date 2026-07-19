use crate::vcd_parser::vcd_data;

use super::vcd_data::VCDBit;
use super::vcd_data::VCDFile;
use super::vcd_data::VCDScope;
use super::vcd_data::VCDSignal;
use super::vcd_data::VCDTimeAndValue;
use super::vcd_data::VCDValue;

use std::cell::RefCell;
// use std::collections::HashMap;
// use std::ops::Deref;
use std::rc::Rc;
// use std::sync::{Arc, Mutex};

use threadpool::ThreadPool;

#[derive(Clone)]
pub struct SignalTC {
    pub signal_name: String,
    pub signal_tc: f64,
}

impl SignalTC {
    pub fn new(signal_name: String) -> Self {
        Self {
            signal_name,
            signal_tc: 0.0,
        }
    }

    pub fn add_tc(&mut self, transition_count: f64) {
        self.signal_tc += transition_count;
    }

    pub fn get_name(&self) -> &str {
        &self.signal_name
    }
}

#[derive(Clone)]
pub struct SignalDuration {
    pub signal_name: String,
    pub bit_0_duration: u64,
    pub bit_1_duration: u64,
    pub bit_x_duration: u64,
    pub bit_z_duration: u64,
}

impl SignalDuration {
    pub fn new(signal_name: String) -> Self {
        Self {
            signal_name,
            bit_0_duration: 0,
            bit_1_duration: 0,
            bit_x_duration: 0,
            bit_z_duration: 0,
        }
    }

    pub fn get_name(&self) -> &str {
        &self.signal_name
    }
}

pub trait VcdCounter {
    fn get_transition_count(
        &self,
        pre_time_value: &VCDTimeAndValue,
        cur_time_value: &VCDTimeAndValue,
        bus_index: Option<i32>,
    ) -> f64 {
        if pre_time_value.time == cur_time_value.time {
            return 0.0;
        }

        let pre_bit_value = self.get_bit_value(pre_time_value, bus_index);
        let cur_bit_value = self.get_bit_value(cur_time_value, bus_index);
        if pre_bit_value == cur_bit_value {
            return 0.0;
        }

        if self.is_unknown_bit(pre_bit_value) || self.is_unknown_bit(cur_bit_value) {
            return 0.5;
        }
        1.0
    }

    fn get_bit_value(&self, time_value: &VCDTimeAndValue, bus_index: Option<i32>) -> VCDBit {
        match &time_value.value {
            VCDValue::BitScalar(bit) => *bit,
            VCDValue::BitVector(bit_vec) => {
                let index = bus_index.unwrap() as usize;
                if index < bit_vec.len() {
                    bit_vec[index]
                } else {
                    VCDBit::BitZero
                }
            }
            _ => panic!("Unmatched value"),
        }
    }

    fn is_unknown_bit(&self, bit_value: VCDBit) -> bool {
        bit_value == VCDBit::BitX || bit_value == VCDBit::BitZ
    }

    fn get_duration(
        &self,
        pre_time_value: &VCDTimeAndValue,
        cur_time_value: &VCDTimeAndValue,
    ) -> i64 {
        let pre_time = pre_time_value.time;
        let cur_time = cur_time_value.time;

        cur_time - pre_time
    }

    fn update_duration(
        &self,
        signal_duration: &mut SignalDuration,
        bit_value: VCDBit,
        duration: u64,
    ) {
        match bit_value {
            VCDBit::BitZero => signal_duration.bit_0_duration += duration,
            VCDBit::BitOne => signal_duration.bit_1_duration += duration,
            VCDBit::BitX => signal_duration.bit_x_duration += duration,
            VCDBit::BitZ => signal_duration.bit_z_duration += duration,
        }
    }
}

pub struct VcdScalarCounter<'a> {
    vcd_file: &'a VCDFile,
    signal: &'a VCDSignal,
    signal_name: String,
    pub signal_tc_vec: &'a mut Vec<SignalTC>,
    pub signal_duration_vec: &'a mut Vec<SignalDuration>,
}
impl<'a> VcdCounter for VcdScalarCounter<'a> {}

impl<'a> VcdScalarCounter<'a> {
    pub fn new(
        vcd_file: &'a VCDFile,
        signal: &'a VCDSignal,
        signal_name: String,
        signal_tc_vec: &'a mut Vec<SignalTC>,
        signal_duration_vec: &'a mut Vec<SignalDuration>,
    ) -> Self {
        Self {
            vcd_file,
            signal,
            signal_name,
            signal_tc_vec,
            signal_duration_vec,
        }
    }

    fn count_tc_and_glitch(&mut self) {
        let signal_hash = self.signal.get_hash();
        let signal_time_values = self.vcd_file.get_signal_values().get(signal_hash);

        let mut signal_toggle = SignalTC::new(self.signal_name.clone());

        // count the toggle, if current signal value is rise transition or fall
        // transition, count add one.
        let mut prev_time_signal_value: Option<&vcd_data::VCDTimeAndValue> = None;

        if let Some(signal_time_values) = signal_time_values.as_deref() {
            for signal_time_value in signal_time_values {
                let signal_time_value = signal_time_value.as_ref();
                if let Some(prev) = prev_time_signal_value {
                    let transition_count = self.get_transition_count(prev, signal_time_value, None);
                    if transition_count > 0.0 {
                        signal_toggle.add_tc(transition_count);
                    }
                }
                prev_time_signal_value = Some(signal_time_value);
            }
        }
        self.signal_tc_vec.push(signal_toggle)
    }

    fn count_duration(&mut self) {
        let signal_hash = self.signal.get_hash();
        let signal_time_values = self.vcd_file.get_signal_values().get(signal_hash);

        //TODO set simulation time
        let simulation_end_time = self.vcd_file.get_end_time();

        let mut annotate_signal_duration_time = SignalDuration::new(self.signal_name.clone());
        // count signal t0,t1,tx,tz duration, the signal may be not start zero time,
        // need consider the start time, such as t0, we accumulate the VCD bit0 time.
        let mut prev_time_signal_value: Option<&vcd_data::VCDTimeAndValue> = None;
        if let Some(signal_time_values) = signal_time_values.as_deref() {
            for signal_time_value in signal_time_values {
                let signal_time_value = signal_time_value.as_ref();
                if let Some(prev) = prev_time_signal_value {
                    let duration = self.get_duration(prev, signal_time_value);
                    let prev_bit_value = &prev.value;

                    let one_bit_value = prev_bit_value.get_bit_scalar();
                    self.update_duration(
                        &mut annotate_signal_duration_time,
                        one_bit_value,
                        duration.try_into().unwrap(),
                    );
                }
                prev_time_signal_value = Some(signal_time_value);
            }
        }

        // for last time, the signal should steady to end.
        if let Some(signal_time_values) = signal_time_values {
            if let Some(last_time_signal_value) = signal_time_values.back() {
                let last_time = last_time_signal_value.time;
                let last_bit_value = last_time_signal_value.value.get_bit_scalar();
                let last_time_duration = simulation_end_time - last_time;
                self.update_duration(
                    &mut annotate_signal_duration_time,
                    last_bit_value,
                    last_time_duration.try_into().unwrap(),
                );
            }
        }
        self.signal_duration_vec.push(annotate_signal_duration_time);
    }

    pub fn run(&mut self) {
        self.count_tc_and_glitch();
        self.count_duration();
    }
}

pub struct FindScopeClosure {
    pub closure: Box<dyn Fn(&Rc<RefCell<VCDScope>>, &str) -> Option<Rc<RefCell<VCDScope>>>>,
}

impl FindScopeClosure {
    pub fn new() -> Self {
        let closure = Box::new(
            move |parent_scope: &Rc<RefCell<VCDScope>>, top_instance_name: &str| {
                if parent_scope.borrow().get_name() == top_instance_name {
                    return Some(parent_scope.clone());
                }
                let children_scopes = parent_scope
                    .as_ref()
                    .borrow_mut()
                    .get_children_scopes()
                    .clone();
                for child_scope in children_scopes.clone() {
                    if child_scope.borrow().get_name() == top_instance_name {
                        return Some(child_scope);
                    }
                }

                for child_scope in children_scopes.clone() {
                    let recursive_closure = FindScopeClosure::new();
                    if let Some(found_scope) =
                        (recursive_closure.closure)(&child_scope, top_instance_name)
                    {
                        return Some(found_scope);
                    }
                }
                None
            },
        );
        Self { closure }
    }
}

pub struct FindSignalClosure {
    pub closure: Box<dyn Fn(&Rc<RefCell<VCDScope>>, &str) -> Option<Rc<VCDSignal>>>,
}

impl FindSignalClosure {
    pub fn new() -> Self {
        let closure = Box::new(move |scope: &Rc<RefCell<VCDScope>>, signal_name: &str| {
            let signals = scope.borrow().get_scope_signals().clone();
            for signal in signals.clone() {
                if signal.get_name() == signal_name {
                    return Some(signal);
                }
            }

            let children_scopes = scope.borrow().get_children_scopes().clone();
            for child_scope in children_scopes.clone() {
                if let Some(found_signal) =
                    (FindSignalClosure::new().closure)(&child_scope, signal_name)
                {
                    return Some(found_signal);
                }
            }
            None
        });
        Self { closure }
    }
}

pub struct VcdBusCounter<'a> {
    vcd_file: &'a VCDFile,
    signal: &'a VCDSignal,
    signal_name: String,
    pub signal_tc_vec: &'a mut Vec<SignalTC>,
    pub signal_duration_vec: &'a mut Vec<SignalDuration>,
}
impl<'a> VcdCounter for VcdBusCounter<'a> {}

impl<'a> VcdBusCounter<'a> {
    pub fn new(
        vcd_file: &'a VCDFile,
        signal: &'a VCDSignal,
        signal_name: String,
        signal_tc_vec: &'a mut Vec<SignalTC>,
        signal_duration_vec: &'a mut Vec<SignalDuration>,
    ) -> Self {
        Self {
            vcd_file,
            signal,
            signal_name,
            signal_tc_vec,
            signal_duration_vec,
        }
    }
    pub fn count_tc_and_glitch(&mut self) {
        let signal_hash = self.signal.get_hash();
        let signal_time_values = self.vcd_file.get_signal_values().get(signal_hash);
        let bit_index_list = self.get_bit_index_list();

        let mut annotate_signal_toggles: Vec<SignalTC> = Vec::new();
        for bit_index in &bit_index_list {
            let name = format!("{}[{}]", self.signal_name, bit_index);
            annotate_signal_toggles.push(SignalTC::new(name));
        }

        for bit_position in 0..bit_index_list.len() {
            let mut prev_time_signal_value: Option<&vcd_data::VCDTimeAndValue> = None;
            if let Some(signal_time_values) = signal_time_values.as_deref() {
                for signal_time_value in signal_time_values {
                    let signal_time_value = signal_time_value.as_ref();
                    if let Some(prev) = prev_time_signal_value {
                        let transition_count = self.get_transition_count(prev, signal_time_value, Some(bit_position as i32));
                        if transition_count > 0.0 {
                            annotate_signal_toggles[bit_position].add_tc(transition_count);
                        }
                    }
                    prev_time_signal_value = Some(signal_time_value);
                }
            }
        }
        self.signal_tc_vec.extend(annotate_signal_toggles);
    }

    pub fn count_duration(&mut self) {
        let signal_hash = self.signal.get_hash();
        let signal_time_values: Option<&std::collections::VecDeque<Box<VCDTimeAndValue>>> =
            self.vcd_file.get_signal_values().get(signal_hash);

        //TODO set simulation time
        let simulation_end_time = self.vcd_file.get_end_time();
        let bit_index_list = self.get_bit_index_list();

        let mut annotate_signal_duration_times: Vec<SignalDuration> = Vec::new();
        for bit_index in &bit_index_list {
            let name = format!("{}[{}]", self.signal_name, bit_index);
            annotate_signal_duration_times.push(SignalDuration::new(name));
        }

        for bit_position in 0..bit_index_list.len() {
            let mut prev_time_signal_value: Option<&VCDTimeAndValue> = None;
            if let Some(signal_time_values) = signal_time_values.as_deref() {
                for signal_time_value in signal_time_values {
                    let signal_time_value = signal_time_value.as_ref();
                    if let Some(prev) = prev_time_signal_value {
                        let duration = self.get_duration(prev, signal_time_value);
                        let prev_bit_value = prev.value.get_vector_bit(bit_position);
                        self.update_duration(
                            &mut annotate_signal_duration_times[bit_position],
                            prev_bit_value,
                            duration.try_into().unwrap(),
                        );
                    }
                    prev_time_signal_value = Some(signal_time_value);
                }
            }
            if let Some(last_time_signal_value) = prev_time_signal_value {
                let last_time_duration = simulation_end_time - last_time_signal_value.time;
                let last_bit_value = last_time_signal_value.value.get_vector_bit(bit_position);
                self.update_duration(
                    &mut annotate_signal_duration_times[bit_position],
                    last_bit_value,
                    last_time_duration.try_into().unwrap(),
                );
            }
        }
        self.signal_duration_vec
            .extend(annotate_signal_duration_times);
    }

    pub fn run(&mut self) {
        self.count_tc_and_glitch();
        self.count_duration();
    }

    fn get_bit_index_list(&self) -> Vec<i32> {
        let (left_index, right_index) = self.signal.get_bus_index().unwrap();
        if left_index >= right_index {
            (right_index..=left_index).rev().collect()
        } else {
            (left_index..=right_index).collect()
        }
    }
}

pub struct CalcTcAndSp<'a> {
    vcd_file: &'a VCDFile,
}

impl<'a> CalcTcAndSp<'a> {
    pub fn new(vcd_file: &'a VCDFile) -> Self {
        Self { vcd_file }
    }
    fn count_signal(
        &self,
        signal: &VCDSignal,
        signal_name: String,
        signal_tc_vec: &mut Vec<SignalTC>,
        signal_duration_vec: &mut Vec<SignalDuration>,
    ) {
        let signal_size = signal.get_signal_size();
        if signal_size == 1 {
            // scalar signal
            let mut scalar_counter = VcdScalarCounter::new(
                self.vcd_file,
                signal,
                signal_name,
                signal_tc_vec,
                signal_duration_vec,
            );
            scalar_counter.run();
        } else {
            // bus signal
            let mut bus_counter = VcdBusCounter::new(
                self.vcd_file,
                signal,
                signal_name,
                signal_tc_vec,
                signal_duration_vec,
            );
            bus_counter.run();
        }
    }

    pub fn traverse_scope_signal(
        &self,
        parent_scope: &VCDScope,
        scope_path: &str,
        include_scope_path: bool,
        thread_pool: &ThreadPool,
        signal_tc_vec: &mut Vec<SignalTC>,
        signal_duration_vec: &mut Vec<SignalDuration>,
    ) {
        let signals = parent_scope.get_scope_signals();

        for scope_signal in signals {
            match *scope_signal.get_signal_type() {
                vcd_data::VCDVariableType::VarWire | vcd_data::VCDVariableType::VarReg => {
                    let mut signal_name = if include_scope_path && !scope_path.is_empty() {
                        format!("{}/{}", scope_path, scope_signal.get_name())
                    } else {
                        scope_signal.get_name().to_string()
                    };
                    if scope_signal.get_signal_size() == 1 {
                        if let Some((left_index, right_index)) = scope_signal.get_bus_index() {
                            if left_index == right_index {
                                signal_name = format!("{}[{}]", signal_name, left_index);
                            }
                        }
                    }
                    self.count_signal(
                        scope_signal,
                        signal_name,
                        signal_tc_vec,
                        signal_duration_vec,
                    );
                }
                _ => continue,
            }
        }

        // View the next level of the scope
        let children_scopes = parent_scope.get_children_scopes();
        for child_scope in children_scopes {
            let child_scope_name = child_scope.borrow().get_name().to_string();
            let child_scope_path = if scope_path.is_empty() {
                child_scope_name
            } else {
                format!("{}/{}", scope_path, child_scope_name)
            };
            self.traverse_scope_signal(
                &child_scope.borrow(),
                &child_scope_path,
                include_scope_path,
                thread_pool,
                signal_tc_vec,
                signal_duration_vec,
            );
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct TestCounter;
    impl VcdCounter for TestCounter {}

    fn make_time_and_value(time: i64, bit: VCDBit) -> VCDTimeAndValue {
        VCDTimeAndValue {
            time,
            value: VCDValue::BitScalar(bit),
        }
    }

    #[test]
    fn counts_unknown_state_transition_as_half_toggle() {
        let counter = TestCounter;
        let bit_zero = make_time_and_value(0, VCDBit::BitZero);
        let bit_one = make_time_and_value(1, VCDBit::BitOne);
        let bit_x = make_time_and_value(2, VCDBit::BitX);
        let bit_z = make_time_and_value(3, VCDBit::BitZ);
        let bit_one_after_z = make_time_and_value(4, VCDBit::BitOne);
        let bit_zero_same_time = make_time_and_value(4, VCDBit::BitZero);

        assert_eq!(counter.get_transition_count(&bit_zero, &bit_one, None), 1.0);
        assert_eq!(counter.get_transition_count(&bit_one, &bit_x, None), 0.5);
        assert_eq!(counter.get_transition_count(&bit_x, &bit_z, None), 0.5);
        assert_eq!(counter.get_transition_count(&bit_z, &bit_one_after_z, None), 0.5);
        assert_eq!(counter.get_transition_count(&bit_one_after_z, &bit_zero_same_time, None), 0.0);
    }

    #[test]
    fn records_the_first_vcd_timestamp_as_the_simulation_start() {
        let mut vcd_file_parser = vcd_data::VCDFileParser::new();
        vcd_file_parser.set_current_time(10);
        vcd_file_parser.set_current_time(50);

        let vcd_file = vcd_file_parser.get_vcd_file();
        assert_eq!(vcd_file.get_start_time(), 10);
        assert_eq!(vcd_file.get_end_time(), 50);
    }
}
