#include "von_neumann/mips.hpp"

Mips::Mips(std::array<uint32_t, 4096> program, MipsConfig config)
    : imem_(program)
    , dmem_(program)
{
    for (size_t i = 0; i < config.num_alus; ++i) {
        alu_units_.emplace_back(config.alu_latency);
    }
    for (size_t i = 0; i < config.num_mem_units; ++i) {
        mem_units_.emplace_back(config.mem_latency);
    }
    
    fetch_stage_ = std::make_unique<FetchStage>(pc_, imem_, fetch_buffer_);
    decode_stage_ = std::make_unique<DecodeStage>(fetch_buffer_, decode_buffer_);
    dispatch_stage_ = std::make_unique<DispatchStage>(
        decode_buffer_, rat_, rob_, alu_rs_, arf_);
    alu_issue_stage_ = std::make_unique<IssueStage>(alu_rs_, alu_units_);
    mem_issue_stage_ = std::make_unique<IssueStage>(mem_rs_, mem_units_);
    alu_wb_stage_ = std::make_unique<WritebackStage>(alu_units_, alu_rs_, rob_);
    mem_wb_stage_ = std::make_unique<WritebackStage>(mem_units_, mem_rs_, rob_);
    commit_stage_ = std::make_unique<CommitStage>(rob_, rat_, arf_, dmem_);
}

void Mips::tick() {
    // Reverse order for cycle accuracy
    commit_stage_->tick();
    alu_wb_stage_->tick();
    mem_wb_stage_->tick();
    alu_issue_stage_->tick();
    mem_issue_stage_->tick();
    dispatch_stage_->tick();
    decode_stage_->tick(pc_ - 4);
    fetch_stage_->tick();
    
    cycle_count_++;
}

void Mips::run(size_t max_cycles) {
    while (!halted_ && cycle_count_ < max_cycles) {
        tick();
        
        if (pc_ >= 4096 * 4) {
            halted_ = true;
        }
    }
}

const RegisterBank& Mips::get_registers() const { return arf_; }
uint32_t Mips::get_pc() const { return pc_; }
size_t Mips::get_cycle_count() const { return cycle_count_; }
bool Mips::is_halted() const { return halted_; }