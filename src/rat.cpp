#include "rat.hpp"

RegisterAliasTable::RegisterAliasTable()
: entries_{}
{
}

// Check if a register is waiting for a result from a reservation station
bool RegisterAliasTable::is_busy(uint8_t reg_index) const {
  if (reg_index == 0) return false; // Register 0 is always zero, never busy
  return entries_[reg_index].is_busy;
}

// Get the tag of the RS that will produce the register's value
uint8_t RegisterAliasTable::get_tag(uint8_t reg_index) const {
  return entries_[reg_index].tag;
}

// Mark a register as busy when an instruction is issued
void RegisterAliasTable::set_busy(uint8_t reg_index, uint8_t tag) {
  if (reg_index != 0) {
    entries_[reg_index].is_busy = true;
    entries_[reg_index].tag = tag;
  }
}

void RegisterAliasTable::clear_busy(uint8_t reg_index) {
    if (reg_index != 0) {
      entries_[reg_index].is_busy = false;
      entries_[reg_index].tag = 0;
    }
}

// Update the RAT when a result is broadcast on the CDB
// This is the writeback logic for the RAT.
void RegisterAliasTable::update_from_cdb(uint8_t tag) {
  for (auto& entry : entries_) {
    // If an entry was waiting for this specific tag, it's not busy anymore.
    // The value is now in the physical register file.
    if (entry.is_busy && entry.tag == tag) {
      entry.is_busy = false;
      entry.tag = 0; // Clear the tag for cleanliness
      // We can break here since tags are unique
      break; 
    }
  }
}