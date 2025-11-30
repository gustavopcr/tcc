#include "von_neumann/mips.hpp"

Mips::Mips(std::array<uint32_t, 4096> program, MipsConfig config)
    : memory_(program)
    , mem_bus_(memory_, config.data_priority)
    , rob_()                          // Use default size from class
    , alu_rs_(config.rs_size)
    , mem_rs_(config.rs_size)
    , alu_eu_(config.alu_latency)     // Pass latency
    , mem_eu_(config.mem_latency)     // Pass latency
    , config_(config)
{
    // Initialize $zero to 0
    arf_[0] = 0;
    
    // Create pipeline stages
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
    
    // Clear memory bus requests from previous cycle
    mem_bus_.clear_requests();
    
    // Execute pipeline stages in reverse order (to simulate parallel operation)
    // This ensures that results from earlier stages are available to later stages
    
    // Commit (updates architectural state)
    commit_stage_->tick();
    
    // Execute (produces results, may request memory for loads)
    execute_stage_->tick();
    
    // Issue (moves from RS to EU)
    issue_stage_->tick();
    
    // Dispatch (renames and allocates RS/ROB)
    dispatch_stage_->tick();
    
    // Decode (decodes instructions)
    decode_stage_->tick();
    
    // Fetch (requests instruction from memory)
    fetch_stage_->tick();
    
    // Process memory bus (arbitrates between fetch and data)
    mem_bus_.tick();
    
    // Snoop CDB to update reservation station entries
    while (!cdb_.empty()) {
        CdbMessage msg = cdb_.front();
        cdb_.pop();
        alu_rs_.snoop_cdb(msg.rob_id, msg.value);
        mem_rs_.snoop_cdb(msg.rob_id, msg.value);
    }
    
    // Update statistics
    stats_.fetch_stalls = mem_bus_.get_fetch_stall_cycles();
    stats_.data_stalls = mem_bus_.get_data_stall_cycles();
    stats_.memory_accesses = mem_bus_.get_total_accesses();
    
    // Check for halt condition (no more work)
    if (fetch_buffer_.instructions.empty() &&
        decode_buffer_.uops.empty() &&
        rob_.is_empty() &&
        !execute_stage_->has_pending_load() &&
        !commit_stage_->has_pending_store()) {
        halted_ = true;
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
}

void Mips::flush_pipeline() {
    // Clear all pipeline buffers
    fetch_buffer_.instructions.clear();
    fetch_buffer_.pcs.clear();
    decode_buffer_.uops.clear();
    
    // Clear reservation stations
    alu_rs_.flush();
    mem_rs_.flush();
    
    // Clear execution units
    alu_eu_.flush();
    mem_eu_.flush();
    
    // Clear execute stage pending operations
    execute_stage_->flush();
    
    // Clear ROB (except committed entries)
    rob_.flush();
    
    // Reset RAT to architectural state
    rat_.clear_all();
    
    // Clear CDB
    while (!cdb_.empty()) cdb_.pop();
}