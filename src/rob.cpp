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

// This function would be part of your ReorderBuffer class
void ReorderBuffer::commit() {
    if (entries_[head_].is_busy && entries_[head_].state == RobState::Writeback) {
        entries_[head_].is_busy = false;
        
        // 3. Advance the head pointer to the next oldest instruction
        head_ = (head_ + 1) % size_;
    }
}