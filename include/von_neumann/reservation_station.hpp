#ifndef RESERVATION_STATION_HPP
#define RESERVATION_STATION_HPP

#include "von_neumann/uop.hpp"
#include <array>
#include <optional>

class ReservationStation {
public:
    explicit ReservationStation(size_t size = 8);
    
    // Allocation
    bool allocate(const RsEntry& entry);
    
    // Issue
    std::optional<size_t> find_ready() const;
    RsEntry get_entry(size_t index) const;
    void deallocate(size_t index);
    
    // CDB Snooping
    void snoop_cdb(RobIndex rob_id, uint32_t value);
    
    // Status
    bool is_full() const;
    void flush();

private:
    std::vector<RsEntry> entries_;
    size_t capacity_;
};

#endif