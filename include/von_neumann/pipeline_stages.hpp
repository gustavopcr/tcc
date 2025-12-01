#ifndef PIPELINE_STAGES_HPP
#define PIPELINE_STAGES_HPP

#include "von_neumann/uop.hpp"
#include "von_neumann/memory_bus.hpp"
#include "von_neumann/rat.hpp"
#include "von_neumann/rob.hpp"
#include "von_neumann/reservation_station.hpp"
#include "von_neumann/execution_unit.hpp"
#include <array>
#include <queue>
#include <functional>

class FetchStage {
public:
    FetchStage(MemoryBus& mem_bus, FetchBuffer& out_buffer, uint32_t start_pc = 0);
    
    void tick();
    void stall();
    void unstall();
    bool is_stalled() const { return stalled_; }
    
    // NEW: Branch misprediction handling
    void flush_and_redirect(uint32_t new_pc);
    uint32_t get_pc() const { return pc_; }

private:
    MemoryBus& mem_bus_;
    FetchBuffer& out_buffer_;
    uint32_t pc_;
    uint32_t pending_pc_;  // NEW: PC of the pending fetch request
    bool stalled_ = false;
    bool waiting_for_memory_ = false;  // NEW: track if we're waiting
};

class DecodeStage {
public:
    DecodeStage(FetchBuffer& in_buffer, DecodeBuffer& out_buffer);
    
    void tick();
    
    // NEW: Flush support
    void flush();

private:
    FetchBuffer& in_buffer_;
    DecodeBuffer& out_buffer_;
};

class DispatchStage {
public:
    DispatchStage(DecodeBuffer& in_buffer, 
                  RegisterAliasTable& rat,
                  ReorderBuffer& rob,
                  ReservationStation& alu_rs,
                  ReservationStation& mem_rs,      // NEW: memory RS
                  std::array<uint32_t, 32>& arf);
    
    void tick();
    
    // NEW: Flush support
    void flush();

    bool was_rob_stall() const { return last_rob_stall_; }
    bool was_rs_stall() const { return last_rs_stall_; }

private:
    Operand rename_source(uint8_t arch_reg);
    
    DecodeBuffer& in_buffer_;
    RegisterAliasTable& rat_;
    ReorderBuffer& rob_;
    ReservationStation& alu_rs_;
    ReservationStation& mem_rs_;                   // NEW: memory RS
    std::array<uint32_t, 32>& arf_;
    bool last_rob_stall_ = false;
    bool last_rs_stall_ = false;
};

class IssueStage {
public:
    IssueStage(ReservationStation& alu_rs,
               ReservationStation& mem_rs,         // NEW: memory RS
               ExecutionUnit& alu_eu,
               ExecutionUnit& mem_eu);             // NEW: memory EU
    
    void tick();
    
    // NEW: Flush support
    void flush();
    bool was_eu_stall() const { return last_eu_stall_; }

private:
    ReservationStation& alu_rs_;
    ReservationStation& mem_rs_;
    ExecutionUnit& alu_eu_;
    ExecutionUnit& mem_eu_;

    bool last_eu_stall_ = false;
};

class ExecuteStage {
public:
    ExecuteStage(ExecutionUnit& alu_eu,
                 ExecutionUnit& mem_eu,            // NEW: memory EU
                 MemoryBus& mem_bus,               // NEW: for load execution
                 ReorderBuffer& rob,
                 std::queue<CdbMessage>& cdb);
    
    void tick();
    
    // NEW: Flush support
    void flush();
    
    // NEW: Check if a load is waiting for memory
    bool has_pending_load() const { return pending_load_.has_value(); }

private:
    ExecutionUnit& alu_eu_;
    ExecutionUnit& mem_eu_;
    MemoryBus& mem_bus_;
    ReorderBuffer& rob_;
    std::queue<CdbMessage>& cdb_;
    
    // NEW: Track pending load waiting for memory
    std::optional<PendingMemoryOp> pending_load_;
};

class CommitStage {
public:
    CommitStage(ReorderBuffer& rob,
                RegisterAliasTable& rat,
                MemoryBus& mem_bus,                // CHANGED: use bus instead of direct memory
                std::array<uint32_t, 32>& arf,
                std::function<void(uint32_t)> on_misprediction);  // NEW: callback for flush
    
    void tick();
    
    // NEW: Check if store is waiting for memory
    bool has_pending_store() const { return pending_store_.has_value(); }

private:
    ReorderBuffer& rob_;
    RegisterAliasTable& rat_;
    MemoryBus& mem_bus_;
    std::array<uint32_t, 32>& arf_;
    std::function<void(uint32_t)> on_misprediction_;
    
    // NEW: Track pending store waiting for memory
    std::optional<PendingMemoryOp> pending_store_;
};

#endif