#ifndef UOP_HPP
#define UOP_HPP

#include "von_neumann/alu.hpp"
#include "von_neumann/instruction.hpp"
#include <cstdint>
#include <optional>
#include <deque>

using RobIndex = uint8_t;
constexpr RobIndex INVALID_ROB_INDEX = 0;

using PhysicalReg = uint8_t;
constexpr PhysicalReg INVALID_PHYS_REG = 0;

struct Operand {
    bool is_ready = false;
    uint32_t value = 0;
    RobIndex producer_rob = 0;
    
    static Operand ready(uint32_t val) {
        return {true, val, 0};
    }
    
    static Operand pending(RobIndex rob) {
        return {false, 0, rob};
    }
};

struct Uop {
    uint32_t raw_inst = 0;
    uint32_t pc = 0;
    Instruction decoded{};
    AluOperation alu_op = AluOperation::ADD;
    RobIndex rob_id = INVALID_ROB_INDEX;
    uint8_t arch_dest = 0;
    Operand src1{};
    Operand src2{};
    uint32_t immediate = 0;
    std::optional<uint32_t> result{};
    bool is_load = false;
    bool is_store = false;
    bool is_shift = false;
    bool is_branch = false;           // NEW: branch flag
    bool branch_taken = false;        // NEW: actual branch outcome
    uint32_t branch_target = 0;       // NEW: computed branch target
    uint32_t mem_address = 0;
    uint32_t store_data = 0;
    bool is_valid = false;
    bool executed = false;
    bool ready_to_commit = false;
};

struct CdbMessage {
    RobIndex rob_id;
    uint32_t value;
};

struct RsEntry {
    bool is_busy = false;
    RobIndex rob_id = INVALID_ROB_INDEX;
    AluOperation op = AluOperation::ADD;
    Operand src1{};
    Operand src2{};
    uint32_t immediate = 0;
    bool is_load = false;
    bool is_store = false;
    uint32_t pc = 0;                  // NEW: for branch target calculation
    bool is_branch = false;           // NEW: branch flag
    
    bool is_ready() const {
        return is_busy && src1.is_ready && src2.is_ready;
    }
};

enum class RobState : uint8_t {
    Invalid,
    Issued,
    Executing,
    WaitingMemory,                    // NEW: waiting for memory access
    WriteBack,
    Committed
};

struct RobEntry {
    RobState state = RobState::Invalid;
    uint32_t pc = 0;
    AluOperation op = AluOperation::ADD;
    uint8_t arch_dest = 0;
    bool has_result = false;
    uint32_t result_value = 0;
    bool is_store = false;
    bool is_load = false;             // NEW: load flag
    uint32_t store_address = 0;
    uint32_t store_data = 0;
    uint32_t load_address = 0;        // NEW: for loads
    bool is_branch = false;           // NEW: branch flag
    bool branch_taken = false;        // NEW: actual outcome
    uint32_t branch_target = 0;       // NEW: target if taken
    bool has_exception = false;
};

struct ExecutingInst {
    RobIndex rob_id = INVALID_ROB_INDEX;
    AluOperation op = AluOperation::ADD;
    uint32_t src1_val = 0;
    uint32_t src2_val = 0;
    uint32_t immediate = 0;
    uint32_t pc = 0;                  // NEW: for branch target
    int cycles_remaining = 0;
    bool is_load = false;
    bool is_store = false;
    bool is_branch = false;           // NEW: branch flag
};

struct ExecutionResult {
    RobIndex rob_id = INVALID_ROB_INDEX;
    uint32_t value = 0;
    bool is_store = false;
    bool is_load = false;             // NEW: load flag
    uint32_t store_address = 0;
    uint32_t store_data = 0;
    uint32_t load_address = 0;        // NEW: for loads
    bool is_branch = false;           // NEW: branch flag
    bool branch_taken = false;        // NEW: actual outcome
    uint32_t branch_target = 0;       // NEW: target if taken
};

// NEW: Pending memory operation (for loads that need memory access)
struct PendingMemoryOp {
    RobIndex rob_id = INVALID_ROB_INDEX;
    uint32_t address = 0;
    bool is_load = false;
    bool is_store = false;
    uint32_t store_data = 0;
};

struct FetchBuffer {
    std::deque<uint32_t> instructions;
    std::deque<uint32_t> pcs;         // NEW: track PC for each instruction
    static constexpr size_t MAX_SIZE = 4;
    bool is_full() const { return instructions.size() >= MAX_SIZE; }
};

struct DecodeBuffer {
    std::deque<Uop> uops;
    static constexpr size_t MAX_SIZE = 4;
    bool is_full() const { return uops.size() >= MAX_SIZE; }
};

#endif