#include "von_neumann/pipeline_stages.hpp"
#include "von_neumann/decoder.hpp"

// FetchStage
FetchStage::FetchStage(uint32_t& pc, Memory& imem, FetchBuffer& out_buffer)
    : pc_(pc), imem_(imem), out_buffer_(out_buffer) {}

void FetchStage::tick() {
    if (out_buffer_.is_full()) return;
    if (stall_) return;
    
    uint32_t inst = imem_.read_word(pc_);
    out_buffer_.instructions.push_back(inst);
    pc_ += 4;
}

void FetchStage::stall() { stall_ = true; }
void FetchStage::unstall() { stall_ = false; }

// DecodeStage
DecodeStage::DecodeStage(FetchBuffer& in_buffer, DecodeBuffer& out_buffer)
    : in_buffer_(in_buffer), out_buffer_(out_buffer) {}

void DecodeStage::tick(uint32_t current_pc) {
    if (in_buffer_.instructions.empty()) return;
    if (out_buffer_.is_full()) return;
    
    uint32_t raw = in_buffer_.instructions.front();
    in_buffer_.instructions.pop_front();
    
    Uop uop{};
    uop.raw_inst = raw;
    uop.pc = current_pc;
    uop.decoded = decode(raw);
    uop.alu_op = get_alu_operation(uop.decoded.op, uop.decoded.funct);
    uop.immediate = uop.decoded.immediate;
    uop.is_valid = true;
    
    switch (uop.decoded.op) {
        case 0x00: // R-type
            uop.arch_dest = uop.decoded.rd;
            break;
        case 0x08: // addi
            uop.arch_dest = uop.decoded.rt;
            break;
        case 0x23: // lw
            uop.arch_dest = uop.decoded.rt;
            uop.is_load = true;
            break;
        case 0x2B: // sw
            uop.arch_dest = 0;
            uop.is_store = true;
            break;
        default:
            break;
    }
    
    out_buffer_.uops.push_back(uop);
}

// DispatchStage
DispatchStage::DispatchStage(DecodeBuffer& in_buffer,
                             RegisterAliasTable& rat,
                             ReorderBuffer& rob,
                             ReservationStation& rs,
                             const RegisterBank& arf)
    : in_buffer_(in_buffer), rat_(rat), rob_(rob), rs_(rs), arf_(arf) {}

void DispatchStage::tick() {
    if (in_buffer_.uops.empty()) return;
    if (rob_.is_full() || rs_.is_full()) return;
    
    Uop uop = in_buffer_.uops.front();
    
    auto rob_id = rob_.allocate(uop.pc, uop.alu_op, uop.arch_dest);
    if (!rob_id) return;
    
    uop.rob_id = *rob_id;
    in_buffer_.uops.pop_front();
    
    uop.src1 = rename_source(uop.decoded.rs);
    
    if (uop.decoded.op == 0x00) { // R-type
        uop.src2 = rename_source(uop.decoded.rt);
    } else if (uop.is_store) { // sw: rt is data source
        uop.src2 = rename_source(uop.decoded.rt);
    } else { // I-type: immediate
        uop.src2 = Operand::ready(uop.immediate);
    }
    
    if (uop.arch_dest != 0) {
        rat_.set_mapping(uop.arch_dest, uop.rob_id);
    }
    
    rs_.dispatch(uop);
}

Operand DispatchStage::rename_source(uint8_t arch_reg) {
    if (arch_reg == 0) return Operand::ready(0);
    
    auto mapping = rat_.get_mapping(arch_reg);
    if (!mapping) {
        return Operand::ready(arf_[arch_reg]);
    }
    
    const RobEntry* entry = rob_.get_entry(*mapping);
    if (entry && entry->has_result) {
        return Operand::ready(entry->result_value);
    }
    
    return Operand::pending(*mapping);
}

// IssueStage
IssueStage::IssueStage(ReservationStation& rs, std::vector<ExecutionUnit>& exec_units)
    : rs_(rs), exec_units_(exec_units) {}

void IssueStage::tick() {
    for (auto& eu : exec_units_) {
        if (eu.is_busy()) continue;
        
        auto entry = rs_.try_issue_oldest();
        if (entry) {
            eu.accept(*entry);
        }
    }
}

// WritebackStage
WritebackStage::WritebackStage(std::vector<ExecutionUnit>& exec_units,
                               ReservationStation& rs,
                               ReorderBuffer& rob)
    : exec_units_(exec_units), rs_(rs), rob_(rob) {}

void WritebackStage::tick() {
    for (auto& eu : exec_units_) {
        auto result = eu.tick();
        if (result) {
            CdbMessage msg{result->rob_id, result->value};
            rs_.snoop_cdb(msg);
            
            if (result->is_store) {
                rob_.prepare_store(result->rob_id,
                                   result->store_address,
                                   result->store_data);
            } else {
                rob_.write_result(result->rob_id, result->value);
            }
        }
    }
}

// CommitStage
CommitStage::CommitStage(ReorderBuffer& rob,
                         RegisterAliasTable& rat,
                         RegisterBank& arf,
                         Memory& dmem)
    : rob_(rob), rat_(rat), arf_(arf), dmem_(dmem) {}

void CommitStage::tick() {
    RobEntry* head = rob_.get_head();
    if (!head) return;
    
    if (head->state != RobState::WriteBack) return;
    
    RobIndex head_idx = rob_.get_head_index();
    
    if (head->is_store) {
        dmem_.store_word(head->store_address, head->store_data);
    }
    
    if (head->arch_dest != 0 && head->has_result) {
        arf_[head->arch_dest] = head->result_value;
    }
    
    rat_.clear_if_matches(head->arch_dest, head_idx);
    
    rob_.commit_head();
}