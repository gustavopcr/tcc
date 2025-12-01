#include "von_neumann/rob.hpp"

std::optional<RobIndex> ReorderBuffer::allocate(uint32_t pc, AluOperation op, uint8_t arch_dest) {
    if (is_full()) return std::nullopt;
    RobIndex idx = next_seq_++;  // Use monotonically increasing ID
    if (next_seq_ == 0) next_seq_ = 1;  // Skip 0 on wrap (unlikely with uint16_t)
    entries_[tail_].state = RobState::Issued;
    entries_[tail_].pc = pc;
    entries_[tail_].op = op;
    entries_[tail_].arch_dest = arch_dest;
    entries_[tail_].has_result = false;
    entries_[tail_].is_store = false;
    entries_[tail_].has_exception = false;
    entries_[tail_].seq_id = idx;  // ADD: Store sequence ID in entry
    tail_ = (tail_ + 1) % ROB_SIZE;
    count_++;
    
    return idx;
}
void ReorderBuffer::write_result(RobIndex rob_id, uint32_t value) {
    RobEntry* entry = get_entry(rob_id);
    if (!entry) return;
    entry->has_result = true;
    entry->result_value = value;
    entry->state = RobState::WriteBack;
}

void ReorderBuffer::prepare_store(RobIndex rob_id, uint32_t address, uint32_t data) {
    RobEntry* entry = get_entry(rob_id);
    if (!entry) return;
    entry->is_store = true;
    entry->store_address = address;
    entry->store_data = data;
    entry->state = RobState::WriteBack;
}

RobEntry* ReorderBuffer::get_head() {
    if (count_ == 0) return nullptr;
    return &entries_[head_];
}

RobIndex ReorderBuffer::get_head_index() const {
    if (count_ == 0) return INVALID_ROB_INDEX;
    return entries_[head_].seq_id;  // CHANGE: Return sequence ID
}

void ReorderBuffer::commit_head() {
    if (count_ == 0) return;
    entries_[head_].state = RobState::Invalid;
    entries_[head_].seq_id = INVALID_ROB_INDEX;  // Clear the seq_id
    head_ = (head_ + 1) % ROB_SIZE;
    count_--;
}

RobEntry* ReorderBuffer::get_entry(RobIndex rob_id) {
    if (rob_id == INVALID_ROB_INDEX) return nullptr;
    // Search by sequence ID - no bounds check on rob_id value
    for (size_t i = 0; i < ROB_SIZE; ++i) {
        if (entries_[i].state != RobState::Invalid && entries_[i].seq_id == rob_id) {
            return &entries_[i];
        }
    }
    return nullptr;
}

RobEntry* ReorderBuffer::get_entry_mut(RobIndex rob_id) {
    return get_entry(rob_id);  // Same implementation
}

bool ReorderBuffer::is_full() const { return count_ >= ROB_SIZE; }
bool ReorderBuffer::is_empty() const { return count_ == 0; }
size_t ReorderBuffer::size() const { return count_; }

void ReorderBuffer::flush() {
    for (auto& entry : entries_) {
        entry.state = RobState::Invalid;
    }
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}