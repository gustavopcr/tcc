#include "von_neumann/execution_unit.hpp"
#include "von_neumann/alu.hpp"

ExecutionUnit::ExecutionUnit(int latency) : latency_(latency) {}

bool ExecutionUnit::accept(const RsEntry& entry) {
    if (current_inst_.has_value()) return false;
    
    ExecutingInst inst{};
    inst.rob_id = entry.rob_id;
    inst.op = entry.op;
    inst.src1_val = entry.src1.value;
    inst.src2_val = entry.src2.value;
    inst.immediate = entry.immediate;
    inst.cycles_remaining = latency_;
    inst.is_load = entry.is_load;
    inst.is_store = entry.is_store;
    
    current_inst_ = inst;
    return true;
}

std::optional<ExecutionResult> ExecutionUnit::tick() {
    if (!current_inst_.has_value()) return std::nullopt;
    
    current_inst_->cycles_remaining--;
    
    if (current_inst_->cycles_remaining <= 0) {
        ExecutionResult result = compute_result(*current_inst_);
        current_inst_.reset();
        return result;
    }
    
    return std::nullopt;
}

bool ExecutionUnit::is_busy() const { 
    return current_inst_.has_value(); 
}

ExecutionResult ExecutionUnit::compute_result(const ExecutingInst& inst) {
    ExecutionResult result{};
    result.rob_id = inst.rob_id;
    
    if (inst.is_store) {
        result.value = 0;
        result.is_store = true;
        result.store_address = inst.src1_val + inst.immediate;
        result.store_data = inst.src2_val;
    } else if (inst.is_load) {
        // For loads: this returns the ADDRESS, actual load happens elsewhere
        result.value = inst.src1_val + inst.immediate;
    } else {
        result.value = alu(inst.op, inst.src1_val, inst.src2_val);
    }
    
    return result;
}