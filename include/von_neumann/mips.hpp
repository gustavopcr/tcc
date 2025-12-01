#ifndef MIPS_HPP
#define MIPS_HPP

#include "von_neumann/memory.hpp"
#include "von_neumann/memory_bus.hpp"
#include "von_neumann/uop.hpp"
#include "von_neumann/rat.hpp"
#include "von_neumann/rob.hpp"
#include "von_neumann/reservation_station.hpp"
#include "von_neumann/execution_unit.hpp"
#include "von_neumann/pipeline_stages.hpp"
#include <array>
#include <memory>
#include <queue>

struct MipsConfig {
    size_t rob_size = 16;
    size_t rs_size = 8;
    int alu_latency = 1;              // NEW
    int mem_latency = 3;              // NEW
    bool data_priority = true;
};
struct MipsStats {
    uint64_t cycles = 0;
    uint64_t instructions_committed = 0;
    uint64_t fetch_stalls = 0;
    uint64_t data_stalls = 0;
    uint64_t branch_mispredictions = 0;
    uint64_t memory_accesses = 0;

    uint64_t rob_full_stalls = 0;      // Dispatch blocked by full ROB
    uint64_t rs_full_stalls = 0;       // Dispatch blocked by full RS
    uint64_t eu_busy_stalls = 0;       // Issue blocked by busy EU
    
    double get_ipc() const {
        return cycles > 0 ? static_cast<double>(instructions_committed) / cycles : 0.0;
    }
    uint64_t get_total_stalls() const {
      return fetch_stalls + data_stalls + rob_full_stalls + rs_full_stalls + eu_busy_stalls;
    }
    double get_memory_contention_ratio() const {
      uint64_t total = get_total_stalls();
      return total > 0 ? static_cast<double>(fetch_stalls) / total : 0.0;
    }
};

class Mips {
public:
    Mips(std::array<uint32_t, 4096> program, MipsConfig config = {});
    
    void tick();
    void run(size_t max_cycles = 10000);
    
    bool is_halted() const { return halted_; }
    const MipsStats& get_stats() const { return stats_; }
    
    // Debug access
    uint32_t get_register(size_t index) const { return arf_[index]; }
    uint32_t get_memory(uint32_t address) const { return memory_.load_word(address); }

private:
    void handle_misprediction(uint32_t target_pc);
    void flush_pipeline();
    
    // Single unified memory (Von Neumann!)
    Memory memory_;
    MemoryBus mem_bus_;
    
    // Architectural state
    std::array<uint32_t, 32> arf_{};  // Architectural Register File
    
    // OoO structures
    RegisterAliasTable rat_;
    ReorderBuffer rob_;
    ReservationStation alu_rs_;
    ReservationStation mem_rs_;
    ExecutionUnit alu_eu_;
    ExecutionUnit mem_eu_;
    
    // Pipeline buffers
    FetchBuffer fetch_buffer_;
    DecodeBuffer decode_buffer_;
    std::queue<CdbMessage> cdb_;
    
    // Pipeline stages
    std::unique_ptr<FetchStage> fetch_stage_;
    std::unique_ptr<DecodeStage> decode_stage_;
    std::unique_ptr<DispatchStage> dispatch_stage_;
    std::unique_ptr<IssueStage> issue_stage_;
    std::unique_ptr<ExecuteStage> execute_stage_;
    std::unique_ptr<CommitStage> commit_stage_;
    
    // State
    bool halted_ = false;
    MipsStats stats_;
    MipsConfig config_;

    uint32_t cycles_since_flush_ = 100;
    uint32_t idle_cycles_ = 0;  // NEW: Reset idle counter on flush
};

#endif