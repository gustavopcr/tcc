#include "von_neumann/reservation_station.hpp"

bool ReservationStation::dispatch(const Uop& uop) {
    for (auto& entry : entries_) {
        if (!entry.is_busy) {
            entry.is_busy = true;
            entry.rob_id = uop.rob_id;
            entry.op = uop.alu_op;
            entry.src1 = uop.src1;
            entry.src2 = uop.src2;
            entry.immediate = uop.immediate;
            entry.is_load = uop.is_load;
            entry.is_store = uop.is_store;
            return true;
        }
    }
    return false;
}

void ReservationStation::snoop_cdb(const CdbMessage& msg) {
    for (auto& entry : entries_) {
        if (!entry.is_busy) continue;
        
        if (!entry.src1.is_ready && entry.src1.producer_rob == msg.rob_id) {
            entry.src1.is_ready = true;
            entry.src1.value = msg.value;
        }
        if (!entry.src2.is_ready && entry.src2.producer_rob == msg.rob_id) {
            entry.src2.is_ready = true;
            entry.src2.value = msg.value;
        }
    }
}

std::optional<RsEntry> ReservationStation::try_issue() {
    for (auto& entry : entries_) {
        if (entry.is_ready()) {
            RsEntry issued = entry;
            entry.is_busy = false;
            return issued;
        }
    }
    return std::nullopt;
}

std::optional<RsEntry> ReservationStation::try_issue_oldest() {
    RsEntry* oldest = nullptr;
    for (auto& entry : entries_) {
        if (entry.is_ready()) {
            if (!oldest || entry.rob_id < oldest->rob_id) {
                oldest = &entry;
            }
        }
    }
    if (oldest) {
        RsEntry issued = *oldest;
        oldest->is_busy = false;
        return issued;
    }
    return std::nullopt;
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