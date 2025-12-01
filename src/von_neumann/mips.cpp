#include "von_neumann/mips.hpp"
#include <iostream>

Mips::Mips(std::array<uint32_t, 4096> program, MipsConfig config)
    : memory_(program)
    , mem_bus_(memory_, config.data_priority)
    , rob_()
    , alu_rs_(config.rs_size)
    , mem_rs_(config.rs_size)
    , alu_eu_(config.alu_latency)
    , mem_eu_(config.mem_latency)
    , config_(config)
{
    arf_[0] = 0;
    
    fetch_stage_ = std::make_unique<FetchStage>(mem_bus_, fetch_buffer_, 0);
    decode_stage_ = std::make_unique<DecodeStage>(fetch_buffer_, decode_buffer_);
    dispatch_stage_ = std::make_unique<DispatchStage>(
        decode_buffer_, rat_, rob_, alu_rs_, mem_rs_, arf_);
    issue_stage_ = std::make_unique<IssueStage>(alu_rs_, mem_rs_, alu_eu_, mem_eu_);
    execute_stage_ = std::make_unique<ExecuteStage>(alu_eu_, mem_eu_, mem_bus_, rob_, cdb_);
    commit_stage_ = std::make_unique<CommitStage>(
        rob_, rat_, mem_bus_, arf_,
        [this](uint32_t target) { handle_misprediction(target); });
}

void Mips::tick() {
    if (halted_) return;
    
    stats_.cycles++;
    
    mem_bus_.clear_requests();
    
    commit_stage_->tick();
    if (commit_stage_->did_commit()) {
        stats_.instructions_committed++;
    }
    execute_stage_->tick();
    issue_stage_->tick();
    dispatch_stage_->tick();
    decode_stage_->tick();
    fetch_stage_->tick();
    mem_bus_.tick();
    
    while (!cdb_.empty()) {
        CdbMessage msg = cdb_.front();
        cdb_.pop();
        alu_rs_.snoop_cdb(msg.rob_id, msg.value);
        mem_rs_.snoop_cdb(msg.rob_id, msg.value);
    }
    
    stats_.fetch_stalls = mem_bus_.get_fetch_stall_cycles();
    stats_.data_stalls = mem_bus_.get_data_stall_cycles();
    stats_.memory_accesses = mem_bus_.get_total_accesses();
    
    if (dispatch_stage_->was_rob_stall()) stats_.rob_full_stalls++;
    if (dispatch_stage_->was_rs_stall()) stats_.rs_full_stalls++;
    if (issue_stage_->was_eu_stall()) stats_.eu_busy_stalls++;

    // Don't check halt too early
    if (stats_.cycles < 25) return;
    
    // After a flush, give the pipeline time to refill
    if (cycles_since_flush_ < 30) {
        cycles_since_flush_++;
        return;
    }
    
    bool fetch_stalled_at_end = fetch_stage_->is_stalled() && 
                                 !fetch_stage_->is_waiting_for_memory();
    bool rs_empty = alu_rs_.is_empty() && mem_rs_.is_empty();
    bool rob_empty = rob_.is_empty();
    bool eu_idle = !alu_eu_.is_busy() && !mem_eu_.is_busy();
    bool buffers_empty = fetch_buffer_.instructions.empty() && 
                         decode_buffer_.uops.empty();
    bool no_pending_load = !execute_stage_->has_pending_load();
    bool no_pending_store = !commit_stage_->has_pending_store();
    MemoryRequester granted = mem_bus_.get_granted();
    bool data_mem_active = (granted == MemoryRequester::Load || 
                            granted == MemoryRequester::Store);
    
    bool pipeline_empty = fetch_stalled_at_end && 
                          buffers_empty && 
                          rob_empty && 
                          rs_empty && 
                          eu_idle && 
                          no_pending_load && 
                          no_pending_store && 
                          !data_mem_active;
    
    // DEBUG: Print halt condition state
    if (pipeline_empty || idle_cycles_ > 0) {
        std::cerr << "Cycle " << stats_.cycles << " halt check:"
                  << " fetch_end=" << fetch_stalled_at_end
                  << " buf_empty=" << buffers_empty  
                  << " rob_empty=" << rob_empty
                  << " rs_empty=" << rs_empty
                  << " eu_idle=" << eu_idle
                  << " no_pend_ld=" << no_pending_load
                  << " no_pend_st=" << no_pending_store
                  << " !data_mem=" << !data_mem_active
                  << " idle=" << idle_cycles_
                  << std::endl;
    }
    
    if (pipeline_empty) {
        idle_cycles_++;
        if (idle_cycles_ >= 10) {
            halted_ = true;
        }
    } else {
        idle_cycles_ = 0;
    }
}
void Mips::run(size_t max_cycles) {
    while (!halted_ && stats_.cycles < max_cycles) {
        tick();
    }
}

void Mips::handle_misprediction(uint32_t target_pc) {
    stats_.branch_mispredictions++;
    flush_pipeline();
    fetch_stage_->flush_and_redirect(target_pc);
    cycles_since_flush_ = 0;
    idle_cycles_ = 0;
}

void Mips::flush_pipeline() {
    fetch_buffer_.instructions.clear();
    fetch_buffer_.pcs.clear();
    decode_buffer_.uops.clear();
    
    alu_rs_.flush();
    mem_rs_.flush();
    
    alu_eu_.flush();
    mem_eu_.flush();
    
    execute_stage_->flush();
    
    rob_.flush();
    
    rat_.clear_all();
    
    while (!cdb_.empty()) cdb_.pop();
}