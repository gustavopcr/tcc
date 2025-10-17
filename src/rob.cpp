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