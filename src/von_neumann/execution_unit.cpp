#include "von_neumann/execution_unit.hpp"
#include "von_neumann/alu.hpp"

ExecutionUnit::ExecutionUnit(int latency)
    : latency_(latency)
{}

void ExecutionUnit::start_execution(const ExecutingInst& inst) {
    current_inst_ = inst;
    current_inst_.cycles_remaining = latency_;
    busy_ = true;
    has_result_ = false;
}

void ExecutionUnit::tick() {
    if (!busy_) {
        return;
    }
    
    current_inst_.cycles_remaining--;
    
    if (current_inst_.cycles_remaining <= 0) {
        // Execution complete - compute result
        result_ = compute_result(current_inst_);
        has_result_ = true;
        busy_ = false;
    }
}

ExecutionResult ExecutionUnit::compute_result(const ExecutingInst& inst) {
    ExecutionResult result{};
    result.rob_id = inst.rob_id;
    result.is_store = inst.is_store;
    result.is_load = inst.is_load;
    result.is_branch = inst.is_branch;
    
    if (inst.is_store) {
        // Store: compute address, prepare data
        result.store_address = inst.src1_val + inst.immediate;
        result.store_data = inst.src2_val;
        result.value = result.store_address;  // For debugging
        
    } else if (inst.is_load) {
        // Load: compute address (actual memory read happens elsewhere)
        result.load_address = inst.src1_val + inst.immediate;
        result.value = result.load_address;  // Address, not data yet
        
    } else if (inst.is_branch) {
        // Branch: evaluate condition and compute target
        bool condition = false;
        
        switch (inst.op) {
            case AluOperation::BEQ:
            case AluOperation::SUB:  // Fallback for compatibility
                condition = (inst.src1_val == inst.src2_val);
                break;
            case AluOperation::BNE:
                condition = (inst.src1_val != inst.src2_val);
                break;
            case AluOperation::SLT:
                condition = (static_cast<int32_t>(inst.src1_val) < 
                            static_cast<int32_t>(inst.src2_val));
                break;
            default:
                condition = (inst.src1_val == inst.src2_val);
                break;
        }
        
        result.branch_taken = condition;
        // Branch target = PC + 4 + (immediate << 2)
        // Immediate is the raw offset from instruction, shift here
        result.branch_target = inst.pc + 4 + (inst.immediate << 2);
        result.value = condition ? 1 : 0;
    } else {
        // Regular ALU operation
        result.value = alu(inst.op, inst.src1_val, inst.src2_val);
    }
    
    return result;
}

bool ExecutionUnit::has_result() const {
    return has_result_;
}

ExecutionResult ExecutionUnit::get_result() {
    has_result_ = false;
    return result_;
}

bool ExecutionUnit::is_busy() const {
    return busy_;
}

void ExecutionUnit::flush() {
    busy_ = false;
    has_result_ = false;
    current_inst_ = {};
    result_ = {};
}