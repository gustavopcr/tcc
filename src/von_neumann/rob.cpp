#include "von_neumann/rob.hpp"

std::optional<RobIndex> ReorderBuffer::allocate(uint32_t pc, AluOperation op, uint8_t arch_dest) {
    if (is_full()) return std::nullopt;
    
    RobIndex idx = static_cast<RobIndex>(tail_ + 1);
    entries_[tail_].state = RobState::Issued;
    entries_[tail_].pc = pc;
    entries_[tail_].op = op;
    entries_[tail_].arch_dest = arch_dest;
    entries_[tail_].has_result = false;
    entries_[tail_].is_store = false;
    entries_[tail_].has_exception = false;
    
    tail_ = (tail_ + 1) % ROB_SIZE;
    count_++;
    
    return idx;
}

void ReorderBuffer::write_result(RobIndex rob_id, uint32_t value) {
    if (rob_id == 0 || rob_id > ROB_SIZE) return;
    size_t idx = rob_id - 1;
    entries_[idx].has_result = true;
    entries_[idx].result_value = value;
    entries_[idx].state = RobState::WriteBack;
}

void ReorderBuffer::prepare_store(RobIndex rob_id, uint32_t address, uint32_t data) {
    if (rob_id == 0 || rob_id > ROB_SIZE) return;
    size_t idx = rob_id - 1;
    entries_[idx].is_store = true;
    entries_[idx].store_address = address;
    entries_[idx].store_data = data;
    entries_[idx].state = RobState::WriteBack;
}

RobEntry* ReorderBuffer::get_head() {
    if (count_ == 0) return nullptr;
    return &entries_[head_];
}

RobIndex ReorderBuffer::get_head_index() const {
    if (count_ == 0) return INVALID_ROB_INDEX;
    return static_cast<RobIndex>(head_ + 1);
}

void ReorderBuffer::commit_head() {
    if (count_ == 0) return;
    entries_[head_].state = RobState::Invalid;
    head_ = (head_ + 1) % ROB_SIZE;
    count_--;
}

RobEntry* ReorderBuffer::get_entry(RobIndex rob_id) {
    if (rob_id == 0 || rob_id > ROB_SIZE) return nullptr;
    return &entries_[rob_id - 1];
}

RobEntry* ReorderBuffer::get_entry_mut(RobIndex rob_id) {
    if (rob_id == 0 || rob_id > ROB_SIZE) return nullptr;
    return &entries_[rob_id - 1];
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