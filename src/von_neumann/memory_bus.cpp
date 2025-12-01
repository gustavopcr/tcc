#include "von_neumann/memory_bus.hpp"

MemoryBus::MemoryBus(Memory& memory, bool data_priority)
    : memory_(memory)
    , data_priority_(data_priority)
{}

// ============================================================================
// Request Phase - Pipeline stages register their intent
// ============================================================================

void MemoryBus::request_fetch(uint32_t address) {
    fetch_requested_ = true;
    fetch_address_ = address;
}

void MemoryBus::request_load(uint32_t address) {
    load_requested_ = true;
    load_address_ = address;
}

void MemoryBus::request_store(uint32_t address, uint32_t data) {
    store_requested_ = true;
    store_address_ = address;
    store_data_ = data;
}

// ============================================================================
// Arbitration - THE VON NEUMANN BOTTLENECK
// ============================================================================

void MemoryBus::tick() {
    // Reset responses from previous cycle
    fetch_response_ = {};
    load_response_ = {};
    store_complete_ = false;
    granted_ = MemoryRequester::None;
    
    // Count how many requesters we have
    int num_requests = (fetch_requested_ ? 1 : 0) + 
                       (load_requested_ ? 1 : 0) + 
                       (store_requested_ ? 1 : 0);
    
    if (num_requests == 0) {
        return;  // No memory access this cycle
    }
    
    total_accesses_++;
    
    // =========================================================================
    // ARBITRATION LOGIC - Only ONE wins per cycle
    // =========================================================================
    
    if (data_priority_) {
        // Data access (Load/Store) has priority over Fetch
        // This models the typical "data starvation causes fetch stall" scenario
        
        if (store_requested_) {
            // Stores have highest priority (must maintain memory consistency)
            granted_ = MemoryRequester::Store;
            memory_.store_word(store_address_, store_data_);
            store_complete_ = true;
            
            // Track stalls caused to other requesters
            if (fetch_requested_) fetch_stall_cycles_++;
            if (load_requested_) data_stall_cycles_++;  // Load blocked by store
            
        } else if (load_requested_) {
            // Loads have second priority
            granted_ = MemoryRequester::Load;
            load_response_.valid = true;
            load_response_.data = memory_.load_word(load_address_);
            
            // Track stalls caused to fetch
            if (fetch_requested_) fetch_stall_cycles_++;
            
        } else if (fetch_requested_) {
            // Fetch only wins if no data access is pending
            granted_ = MemoryRequester::Fetch;
            fetch_response_.valid = true;
            fetch_response_.data = memory_.load_word(fetch_address_);
        }
        
    } else {
        // Fetch has priority (alternative mode for comparison)
        
        if (fetch_requested_) {
            granted_ = MemoryRequester::Fetch;
            fetch_response_.valid = true;
            fetch_response_.data = memory_.load_word(fetch_address_);
            
            if (load_requested_ || store_requested_) data_stall_cycles_++;
            
        } else if (store_requested_) {
            granted_ = MemoryRequester::Store;
            memory_.store_word(store_address_, store_data_);
            store_complete_ = true;
            
        } else if (load_requested_) {
            granted_ = MemoryRequester::Load;
            load_response_.valid = true;
            load_response_.data = memory_.load_word(load_address_);
        }
    }
}

// ============================================================================
// Response Phase - Stages check if their request was granted
// ============================================================================

MemoryResponse MemoryBus::get_fetch_response() {
    return fetch_response_;
}

MemoryResponse MemoryBus::get_load_response() {
    return load_response_;
}

bool MemoryBus::is_store_complete() const {
    return store_complete_;
}

// ============================================================================
// Cycle Reset
// ============================================================================

void MemoryBus::clear_requests() {
    fetch_requested_ = false;
    load_requested_ = false;
    store_requested_ = false;
}
