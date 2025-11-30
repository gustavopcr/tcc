#include "von_neumann/reservation_station.hpp"

ReservationStation::ReservationStation(size_t size)
    : entries_(size)
    , capacity_(size)
{}

bool ReservationStation::allocate(const RsEntry& entry) {
    for (auto& slot : entries_) {
        if (!slot.is_busy) {
            slot = entry;
            slot.is_busy = true;
            return true;
        }
    }
    return false;
}

std::optional<size_t> ReservationStation::find_ready() const {
    for (size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].is_ready()) {
            return i;
        }
    }
    return std::nullopt;
}

RsEntry ReservationStation::get_entry(size_t index) const {
    return entries_[index];
}

void ReservationStation::deallocate(size_t index) {
    entries_[index].is_busy = false;
}

void ReservationStation::snoop_cdb(RobIndex rob_id, uint32_t value) {
    for (auto& entry : entries_) {
        if (!entry.is_busy) continue;
        
        if (!entry.src1.is_ready && entry.src1.producer_rob == rob_id) {
            entry.src1.is_ready = true;
            entry.src1.value = value;
        }
        if (!entry.src2.is_ready && entry.src2.producer_rob == rob_id) {
            entry.src2.is_ready = true;
            entry.src2.value = value;
        }
    }
}

bool ReservationStation::is_full() const {
    for (const auto& entry : entries_) {
        if (!entry.is_busy) return false;
    }
    return true;
}

void ReservationStation::flush() {
    for (auto& entry : entries_) {
        entry.is_busy = false;
    }
}