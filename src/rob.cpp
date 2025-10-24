#include "rob.hpp"

ReorderBuffer::ReorderBuffer(size_t size)
  : entries_(size)
  , head_(0)
  , tail_(0)
  , size_(size)
{
  for (auto &e : entries_) {
    e.is_busy = false;
    e.operation = AluOperation::ADD;
    e.state = RobState::Waiting;
    e.arch_dest_reg = 0;
    e.physical_dest_reg = 0;
    e.result_value = 0;
  }
}

bool ReorderBuffer::is_full() const{
  return entries_[tail_].is_busy;
}

void ReorderBuffer::update_entry(uint8_t tag, uint32_t value) {
    // Tag 0 is invalid (it means no entry or an error)
    if (tag == 0) {
        return; 
    }

    // Convert the 1-based tag back to a 0-based index
    size_t index = static_cast<size_t>(tag - 1);

    // Check if the entry is valid and is the one we're looking for
    // (We can just check if it's busy, as the tag uniquely identifies it)
    if (entries_[index].is_busy) 
    {
        // Store the result
        entries_[index].result_value = value;
        
        // Mark its state as "Writeback", which means it's complete
        // and waiting to be committed.
        entries_[index].state = RobState::Writeback;
    }
    // If it's not busy, it might have been flushed due to a branch
    // misprediction, so we can safely ignore this broadcast.
}

// Return non-zero tag on success. Tag = slot_index + 1, 0 = full/error
uint8_t ReorderBuffer::add(RobEntry entry) {
  if (entries_[tail_].is_busy) {
    // full
    return 0;
  }
  entries_[tail_] = entry;
  entries_[tail_].is_busy = true;
  uint8_t tag = static_cast<uint8_t>(tail_ + 1); // tag 0 reserved
  tail_ = (tail_ + 1) % size_;
  return tag;
}

void ReorderBuffer::commit(RegisterBank& registers, RegisterAliasTable& rat) {
    RobEntry& head_entry = entries_[head_];

    // 2. Check if it's busy and has a result ready (state is Writeback)
    if (!head_entry.is_busy || head_entry.state != RobState::Writeback) {
        return;
    }

    uint8_t dest_reg = head_entry.arch_dest_reg;


    if (dest_reg != 0) {
        registers[dest_reg] = head_entry.result_value;
    }

    uint8_t head_tag = static_cast<uint8_t>(head_ + 1);
    if (rat.get_tag(dest_reg) == head_tag) {
        rat.clear_busy(dest_reg);
    }

    head_entry.is_busy = false;

    head_ = (head_ + 1) % size_;
}