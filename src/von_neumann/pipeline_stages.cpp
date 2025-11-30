#include "von_neumann/pipeline_stages.hpp"
#include "von_neumann/decoder.hpp"

// ============================================================================
// FetchStage
// ============================================================================

FetchStage::FetchStage(MemoryBus& mem_bus, FetchBuffer& out_buffer, uint32_t start_pc)
    : mem_bus_(mem_bus)
    , out_buffer_(out_buffer)
    , pc_(start_pc)
{}

void FetchStage::tick() {
    if (stalled_) return;
    if (out_buffer_.is_full()) return;

    // Request instruction from memory bus
    mem_bus_.request_fetch(pc_);
    
    // Check if we got a response (might be blocked by data access)
    auto response = mem_bus_.get_fetch_response();
    if (!response.valid) {
        // Memory bus denied our request (data access has priority)
        waiting_for_memory_ = true;
        return;
    }
    
    waiting_for_memory_ = false;
    
    uint32_t instruction = response.data;
    if (instruction == 0) return;  // NOP or end of program
    
    out_buffer_.instructions.push_back(instruction);
    out_buffer_.pcs.push_back(pc_);
    
    // Always-Not-Taken: PC += 4 (never predict taken)
    pc_ += 4;
}

void FetchStage::stall() {
    stalled_ = true;
}

void FetchStage::unstall() {
    stalled_ = false;
}

void FetchStage::flush_and_redirect(uint32_t new_pc) {
    pc_ = new_pc;
    waiting_for_memory_ = false;
    // Note: buffers are flushed by the caller
}

// ============================================================================
// DecodeStage
// ============================================================================

DecodeStage::DecodeStage(FetchBuffer& in_buffer, DecodeBuffer& out_buffer)
    : in_buffer_(in_buffer)
    , out_buffer_(out_buffer)
{}

void DecodeStage::tick() {
    if (in_buffer_.instructions.empty()) return;
    if (out_buffer_.is_full()) return;
    
    uint32_t raw = in_buffer_.instructions.front();
    uint32_t pc = in_buffer_.pcs.front();
    in_buffer_.instructions.pop_front();
    in_buffer_.pcs.pop_front();
    
    Uop uop{};
    uop.raw_inst = raw;
    uop.pc = pc;
    uop.decoded = decode(raw);
    uop.is_valid = true;
    
    // Determine instruction type from opcode
    uint8_t opcode = uop.decoded.op;
    
    // R-type instructions (opcode 0)
    if (opcode == 0) {
        uint8_t funct = uop.decoded.funct;
        switch (funct) {
            case 0x20: uop.alu_op = AluOperation::ADD; break;
            case 0x22: uop.alu_op = AluOperation::SUB; break;
            case 0x24: uop.alu_op = AluOperation::AND; break;
            case 0x25: uop.alu_op = AluOperation::OR; break;
            case 0x26: uop.alu_op = AluOperation::XOR; break;
            case 0x2A: uop.alu_op = AluOperation::SLT; break;
            case 0x00: uop.alu_op = AluOperation::SLL; break;
            case 0x02: uop.alu_op = AluOperation::SRL; break;
            default: uop.alu_op = AluOperation::ADD; break;
        }
        uop.arch_dest = uop.decoded.rd;
    }
    // I-type ALU (ADDI, ANDI, ORI, etc.)
    else if (opcode == 0x08 || opcode == 0x09) {  // ADDI, ADDIU
        uop.alu_op = AluOperation::ADD;
        uop.immediate = static_cast<int16_t>(uop.decoded.immediate);  // Sign extend
        uop.arch_dest = uop.decoded.rt;
    }
    else if (opcode == 0x0C) {  // ANDI
        uop.alu_op = AluOperation::AND;
        uop.immediate = uop.decoded.immediate;  // Zero extend
        uop.arch_dest = uop.decoded.rt;
    }
    else if (opcode == 0x0D) {  // ORI
        uop.alu_op = AluOperation::OR;
        uop.immediate = uop.decoded.immediate;
        uop.arch_dest = uop.decoded.rt;
    }
        // XORI - NEW
    else if (opcode == 0x0E) {
        uop.alu_op = AluOperation::XOR;
        uop.immediate = uop.decoded.immediate;  // Zero extend for logical ops
        uop.arch_dest = uop.decoded.rt;
    }
    // SLTI
    else if (opcode == 0x0A) {
        uop.alu_op = AluOperation::SLT;
        uop.immediate = static_cast<int16_t>(uop.decoded.immediate);  // Sign extend
        uop.arch_dest = uop.decoded.rt;
    }
    // Load instructions
    else if (opcode == 0x23) {  // LW
        uop.is_load = true;
        uop.alu_op = AluOperation::ADD;  // Address calculation
        uop.immediate = static_cast<int16_t>(uop.decoded.immediate);
        uop.arch_dest = uop.decoded.rt;
    }
    // Store instructions
    else if (opcode == 0x2B) {  // SW
        uop.is_store = true;
        uop.alu_op = AluOperation::ADD;  // Address calculation
        uop.immediate = static_cast<int16_t>(uop.decoded.immediate);
        uop.arch_dest = 0;  // Stores don't write to register
    }
    // Branch instructions
    else if (opcode == 0x04) {  // BEQ
        uop.is_branch = true;
        uop.alu_op = AluOperation::SUB;  // Compare via subtraction
        uop.immediate = static_cast<int16_t>(uop.decoded.immediate) << 2;  // Branch offset
        uop.arch_dest = 0;  // Branches don't write to register
    }
    else if (opcode == 0x05) {  // BNE
        uop.is_branch = true;
        uop.alu_op = AluOperation::SUB;
        uop.immediate = static_cast<int16_t>(uop.decoded.immediate) << 2;
        uop.arch_dest = 0;
    }
    
    out_buffer_.uops.push_back(uop);
}

void DecodeStage::flush() {
    // Clear any buffered work
    // Input buffer is cleared by caller
}

// ============================================================================
// DispatchStage
// ============================================================================

DispatchStage::DispatchStage(DecodeBuffer& in_buffer,
                             RegisterAliasTable& rat,
                             ReorderBuffer& rob,
                             ReservationStation& alu_rs,
                             ReservationStation& mem_rs,
                             std::array<uint32_t, 32>& arf)
    : in_buffer_(in_buffer)
    , rat_(rat)
    , rob_(rob)
    , alu_rs_(alu_rs)
    , mem_rs_(mem_rs)
    , arf_(arf)
{}

Operand DispatchStage::rename_source(uint8_t arch_reg) {
    // $zero is always 0
    if (arch_reg == 0) {
        return Operand::ready(0);
    }
    
    // Check RAT for pending producer
    auto mapping = rat_.get_mapping(arch_reg);
    if (!mapping) {
        // No pending write - read from ARF
        return Operand::ready(arf_[arch_reg]);
    }
    
    // Check if producer has completed (result in ROB)
    const RobEntry* entry = rob_.get_entry(*mapping);
    if (entry && entry->has_result) {
        return Operand::ready(entry->result_value);
    }
    
    // Still waiting for producer
    return Operand::pending(*mapping);
}

void DispatchStage::tick() {
    if (in_buffer_.uops.empty()) return;
    if (rob_.is_full()) return;
    
    const Uop& uop = in_buffer_.uops.front();
    
    // Select appropriate reservation station
    ReservationStation& target_rs = (uop.is_load || uop.is_store) ? mem_rs_ : alu_rs_;
    
    if (target_rs.is_full()) return;
    
    // Allocate ROB entry
    auto rob_id_opt = rob_.allocate(uop.pc, uop.alu_op, uop.arch_dest);
    if (!rob_id_opt) return;
    
    RobIndex rob_id = *rob_id_opt;
    if (rob_id == INVALID_ROB_INDEX) return;
    
    // Setup ROB entry
    RobEntry* rob_entry = rob_.get_entry_mut(rob_id);
    if (!rob_entry) return;
    rob_entry->state = RobState::Issued;
    rob_entry->pc = uop.pc;
    rob_entry->op = uop.alu_op;
    rob_entry->arch_dest = uop.arch_dest;
    rob_entry->is_store = uop.is_store;
    rob_entry->is_load = uop.is_load;
    rob_entry->is_branch = uop.is_branch;
    
    // Update RAT if instruction writes to a register
    if (uop.arch_dest != 0) {
        rat_.set_mapping(uop.arch_dest, rob_id);
    }
    
    // Rename source operands
    Operand src1 = rename_source(uop.decoded.rs);
    Operand src2{};
    
    if (uop.decoded.op == 0) {  // R-type: second source is rt
        src2 = rename_source(uop.decoded.rt);
    } else if (uop.is_store) {  // Store: rt is the data to store
        src2 = rename_source(uop.decoded.rt);
    } else if (uop.is_branch) {  // Branch: compare rs and rt
        src2 = rename_source(uop.decoded.rt);
    } else {  // I-type with immediate
        src2 = Operand::ready(0);  // Immediate handled separately
    }
    
    // Allocate RS entry
    RsEntry rs_entry{};
    rs_entry.is_busy = true;
    rs_entry.rob_id = rob_id;
    rs_entry.op = uop.alu_op;
    rs_entry.src1 = src1;
    rs_entry.src2 = src2;
    rs_entry.immediate = uop.immediate;
    rs_entry.is_load = uop.is_load;
    rs_entry.is_store = uop.is_store;
    rs_entry.is_branch = uop.is_branch;
    rs_entry.pc = uop.pc;
    
    target_rs.allocate(rs_entry);
    
    in_buffer_.uops.pop_front();
}

void DispatchStage::flush() {
    // Decode buffer cleared by caller
}

// ============================================================================
// IssueStage
// ============================================================================

IssueStage::IssueStage(ReservationStation& alu_rs,
                       ReservationStation& mem_rs,
                       ExecutionUnit& alu_eu,
                       ExecutionUnit& mem_eu)
    : alu_rs_(alu_rs)
    , mem_rs_(mem_rs)
    , alu_eu_(alu_eu)
    , mem_eu_(mem_eu)
{}

void IssueStage::tick() {
    // Try to issue from ALU RS
    if (!alu_eu_.is_busy()) {
        auto ready_idx = alu_rs_.find_ready();
        if (ready_idx) {
            RsEntry entry = alu_rs_.get_entry(*ready_idx);
            alu_rs_.deallocate(*ready_idx);
            
            ExecutingInst inst{};
            inst.rob_id = entry.rob_id;
            inst.op = entry.op;
            inst.src1_val = entry.src1.value;
            inst.src2_val = entry.src2.value;
            inst.immediate = entry.immediate;
            inst.pc = entry.pc;
            inst.is_branch = entry.is_branch;
            inst.cycles_remaining = 1;  // ALU ops take 1 cycle
            
            alu_eu_.start_execution(inst);
        }
    }
    
    // Try to issue from Memory RS
    if (!mem_eu_.is_busy()) {
        auto ready_idx = mem_rs_.find_ready();
        if (ready_idx) {
            RsEntry entry = mem_rs_.get_entry(*ready_idx);
            mem_rs_.deallocate(*ready_idx);
            
            ExecutingInst inst{};
            inst.rob_id = entry.rob_id;
            inst.op = entry.op;
            inst.src1_val = entry.src1.value;
            inst.src2_val = entry.src2.value;
            inst.immediate = entry.immediate;
            inst.is_load = entry.is_load;
            inst.is_store = entry.is_store;
            inst.cycles_remaining = 1;  // Address calculation takes 1 cycle
            
            mem_eu_.start_execution(inst);
        }
    }
}

void IssueStage::flush() {
    // Reservation stations cleared by caller
}

// ============================================================================
// ExecuteStage
// ============================================================================

ExecuteStage::ExecuteStage(ExecutionUnit& alu_eu,
                           ExecutionUnit& mem_eu,
                           MemoryBus& mem_bus,
                           ReorderBuffer& rob,
                           std::queue<CdbMessage>& cdb)
    : alu_eu_(alu_eu)
    , mem_eu_(mem_eu)
    , mem_bus_(mem_bus)
    , rob_(rob)
    , cdb_(cdb)
{}

void ExecuteStage::tick() {
    // Handle pending load waiting for memory
    if (pending_load_) {
        auto response = mem_bus_.get_load_response();
        if (response.valid) {
            // Load completed - broadcast result
            cdb_.push({pending_load_->rob_id, response.data});
            
            RobEntry* entry = rob_.get_entry(pending_load_->rob_id);
            if (entry) {
                entry->has_result = true;
                entry->result_value = response.data;
                entry->state = RobState::WriteBack;
            }
            
            pending_load_.reset();
        } else {
            // Still waiting - request again
            mem_bus_.request_load(pending_load_->address);
        }
    }
    
    // Tick execution units
    alu_eu_.tick();
    mem_eu_.tick();
    
    // Check ALU EU for completed instructions
    if (alu_eu_.has_result()) {
        ExecutionResult result = alu_eu_.get_result();
        
        if (result.is_branch) {
            // Branch result - update ROB with outcome
            RobEntry* entry = rob_.get_entry(result.rob_id);
            if (entry) {
                entry->is_branch = true;
                entry->branch_taken = result.branch_taken;
                entry->branch_target = result.branch_target;
                entry->has_result = true;
                entry->state = RobState::WriteBack;
            }
        } else {
            // Regular ALU result
            cdb_.push({result.rob_id, result.value});
            
            RobEntry* entry = rob_.get_entry(result.rob_id);
            if (entry) {
                entry->has_result = true;
                entry->result_value = result.value;
                entry->state = RobState::WriteBack;
            }
        }
    }
    
    // Check Memory EU for completed address calculations
    if (mem_eu_.has_result()) {
        ExecutionResult result = mem_eu_.get_result();
        
        if (result.is_load) {
            // Load: address calculated, now need to access memory
            pending_load_ = PendingMemoryOp{
                result.rob_id,
                result.load_address,
                true,
                false,
                0
            };
            mem_bus_.request_load(result.load_address);
            
            RobEntry* entry = rob_.get_entry(result.rob_id);
            if (entry) {
                entry->load_address = result.load_address;
                entry->state = RobState::WaitingMemory;
            }
        } else if (result.is_store) {
            // Store: address calculated, data ready - update ROB
            RobEntry* entry = rob_.get_entry(result.rob_id);
            if (entry) {
                entry->store_address = result.store_address;
                entry->store_data = result.store_data;
                entry->has_result = true;  // Store is "done" (will commit later)
                entry->state = RobState::WriteBack;
            }
        }
    }
}

void ExecuteStage::flush() {
    pending_load_.reset();
    // EUs cleared separately
}

// ============================================================================
// CommitStage
// ============================================================================

CommitStage::CommitStage(ReorderBuffer& rob,
                         RegisterAliasTable& rat,
                         MemoryBus& mem_bus,
                         std::array<uint32_t, 32>& arf,
                         std::function<void(uint32_t)> on_misprediction)
    : rob_(rob)
    , rat_(rat)
    , mem_bus_(mem_bus)
    , arf_(arf)
    , on_misprediction_(on_misprediction)
{}

void CommitStage::tick() {
    // Handle pending store
    if (pending_store_) {
        if (mem_bus_.is_store_complete()) {
            // Store completed - can now actually commit
            pending_store_.reset();
            rob_.commit_head();
        } else {
            // Still waiting - request again
            mem_bus_.request_store(pending_store_->address, pending_store_->store_data);
            return;  // Can't commit anything else until store completes
        }
    }
    
    RobEntry* head = rob_.get_head();
    if (!head) return;
    if (head->state != RobState::WriteBack) return;
    
    RobIndex head_id = rob_.get_head_index();
    
    // Handle branch misprediction
    if (head->is_branch && head->branch_taken) {
        // Always-Not-Taken predicted not taken, but branch was taken
        // Trigger pipeline flush and redirect
        if (on_misprediction_) {
            on_misprediction_(head->branch_target);
        }
        rob_.commit_head();
        return;
    }
    
    // Handle store commit (must go through memory bus)
    if (head->is_store) {
        pending_store_ = PendingMemoryOp{
            head_id,
            head->store_address,
            false,
            true,
            head->store_data
        };
        mem_bus_.request_store(head->store_address, head->store_data);
        return;  // Wait for store to complete
    }
    
    // Regular commit: update ARF
    if (head->arch_dest != 0 && head->has_result) {
        arf_[head->arch_dest] = head->result_value;
    }
    
    // Clear RAT mapping if this ROB entry was the producer
    if (head->arch_dest != 0) {
        auto mapping = rat_.get_mapping(head->arch_dest);
        if (mapping && *mapping == head_id) {
            rat_.clear_mapping(head->arch_dest);
        }
    }
    
    rob_.commit_head();
}