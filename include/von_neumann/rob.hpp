#ifndef ROB_HPP
#define ROB_HPP

#include "von_neumann/uop.hpp"
#include <array>
#include <optional>

class ReorderBuffer {
public:
    static constexpr size_t ROB_SIZE = 16;
    
    ReorderBuffer() = default;
    
    std::optional<RobIndex> allocate(uint32_t pc, AluOperation op, uint8_t arch_dest);
    void write_result(RobIndex rob_id, uint32_t value);
    void prepare_store(RobIndex rob_id, uint32_t address, uint32_t data);
    RobEntry* get_head();
    RobIndex get_head_index() const;
    void commit_head();
    RobEntry* get_entry(RobIndex rob_id);
    RobEntry* get_entry_mut(RobIndex rob_id);
    bool is_full() const;
    bool is_empty() const;
    size_t size() const;
    void flush();

private:
    std::array<RobEntry, ROB_SIZE> entries_{};
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t count_ = 0;
    RobIndex next_seq_ = 1;  // ADD: Unique sequence number
};

#endif