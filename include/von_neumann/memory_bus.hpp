#ifndef MEMORY_BUS_HPP
#define MEMORY_BUS_HPP

#include "von_neumann/memory.hpp"
#include <cstdint>
#include <optional>

// Response from memory bus
struct MemoryResponse {
    bool valid = false;      // Was the request granted this cycle?
    uint32_t data = 0;       // Data (for loads/fetches)
};

// Tracks which unit is requesting memory
enum class MemoryRequester : uint8_t {
    None,
    Fetch,
    Load,
    Store
};

/**
 * MemoryBus - The Von Neumann Bottleneck Enforcer
 * 
 * This class arbitrates access to the single unified memory.
 * Only ONE of {Fetch, Load, Store} can access memory per cycle.
 * 
 * Priority (configurable):
 *   - data_priority=true:  Load/Store > Fetch (default)
 *   - data_priority=false: Fetch > Load/Store
 */
class MemoryBus {
public:
    explicit MemoryBus(Memory& memory, bool data_priority = true);
    
    // === Request Phase (called by pipeline stages) ===
    void request_fetch(uint32_t address);
    void request_load(uint32_t address);
    void request_store(uint32_t address, uint32_t data);
    
    // === Response Phase (called after tick()) ===
    MemoryResponse get_fetch_response();
    MemoryResponse get_load_response();
    bool is_store_complete() const;
    
    // === Cycle Management ===
    void tick();              // Arbitrates and executes ONE memory operation
    void clear_requests();    // Reset for next cycle
    
    // === Statistics ===
    uint64_t get_fetch_stall_cycles() const { return fetch_stall_cycles_; }
    uint64_t get_data_stall_cycles() const { return data_stall_cycles_; }
    uint64_t get_total_accesses() const { return total_accesses_; }
    bool was_busy_this_cycle() const {
        return granted_ != MemoryRequester::None;
    }
    MemoryRequester get_granted() const { return granted_; }

private:
    Memory& memory_;
    bool data_priority_;
    
    // Current cycle requests
    bool fetch_requested_ = false;
    uint32_t fetch_address_ = 0;
    
    bool load_requested_ = false;
    uint32_t load_address_ = 0;
    
    bool store_requested_ = false;
    uint32_t store_address_ = 0;
    uint32_t store_data_ = 0;
    
    // Current cycle responses (set by tick())
    MemoryResponse fetch_response_{};
    MemoryResponse load_response_{};
    bool store_complete_ = false;
    
    // Who won arbitration this cycle
    MemoryRequester granted_ = MemoryRequester::None;
    
    // Statistics
    uint64_t fetch_stall_cycles_ = 0;
    uint64_t data_stall_cycles_ = 0;
    uint64_t total_accesses_ = 0;
};

#endif