#ifndef DATAFLOW_STATS_HPP
#define DATAFLOW_STATS_HPP

#include <cstdint>
#include <algorithm>

struct DataflowStats {
    uint64_t total_cycles = 0;
    uint64_t tokens_generated = 0;
    uint64_t tokens_consumed = 0;
    
    // Parallelism tracking
    uint64_t total_active_alus = 0;      // Sum over all cycles
    uint64_t max_parallel_tasks = 0;
    uint64_t cycles_with_activity = 0;   // Cycles where at least 1 ALU was busy
    
    // NEW: Idle cycles (no instructions fired)
    uint64_t idle_cycles = 0;            // Cycles where 0 ALUs were active
    
    // Matching store pressure
    uint64_t max_matching_store_size = 0;
    
    double get_avg_parallelism() const {
        return cycles_with_activity > 0 
            ? static_cast<double>(total_active_alus) / cycles_with_activity 
            : 0.0;
    }
    
    uint64_t get_total_tokens() const {
        return tokens_generated + tokens_consumed;
    }
    
    // NEW: Idle ratio (analogous to Von Neumann stall ratio)
    double get_idle_ratio() const {
        return total_cycles > 0
            ? static_cast<double>(idle_cycles) / total_cycles
            : 0.0;
    }
};

#endif