#ifndef RAT_HPP
#define RAT_HPP

#include "von_neumann/uop.hpp"
#include <array>
#include <optional>

struct RatEntry {
    bool is_mapped = false;
    RobIndex rob_index = 0;
};

class RegisterAliasTable {
public:
    RegisterAliasTable() = default;
    
    std::optional<RobIndex> get_mapping(uint8_t arch_reg) const;
    void set_mapping(uint8_t arch_reg, RobIndex rob_id);
    void clear_if_matches(uint8_t arch_reg, RobIndex rob_id);
    void clear_mapping(uint8_t arch_reg);  // NEW
    void clear_all();                       // NEW: for flush
    void flush() { clear_all(); }           // Alias

private:
    std::array<RatEntry, 32> entries_{};
};

#endif