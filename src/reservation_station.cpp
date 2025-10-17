#include "reservation_station.hpp"

ReservationStation::ReservationStation(size_t size) 
: entries_(size)
{
  for (size_t i = 0; i < size; ++i) {
      entries_[i].arch_reg = (i + 1);
  }
}

bool ReservationStation::is_full() const 
{
  for (const auto& entry : entries_) {
      if (!entry.is_busy) {
          return false;
      }
  }
  return true;
}

void ReservationStation::add(const ReservationStationEntry& entry_to_add) {
  for (auto& internal_entry : entries_) {
    if (!internal_entry.is_busy) {
        // Found a free slot. Copy the data from the provided entry.
        internal_entry.op = entry_to_add.op;
        internal_entry.src1_is_ready = entry_to_add.src1_is_ready;
        internal_entry.Vj = entry_to_add.Vj;
        internal_entry.Qj = entry_to_add.Qj;
        internal_entry.src2_is_ready = entry_to_add.src2_is_ready;
        internal_entry.Vk = entry_to_add.Vk;
        internal_entry.Qk = entry_to_add.Qk;
        internal_entry.immediate = entry_to_add.immediate;
        // ... copy any other relevant fields ...
        
        // Now, mark it as busy.
        internal_entry.is_busy = true;
        break;
    }
  }
}

void ReservationStation::update_with_cdb_message(uint8_t tag, uint32_t value) 
{
  for (auto& entry : entries_) {
    if (entry.is_busy) {
      if (!entry.src1_is_ready && entry.Qj == tag) {
          entry.Vj = value;
          entry.src1_is_ready = true;
      }
      if (!entry.src2_is_ready && entry.Qk == tag) {
          entry.Vk = value;
          entry.src2_is_ready = true;
      }
    }
  }
}

std::vector<ReservationStationEntry>& ReservationStation::get_entries() {
    return entries_;
}