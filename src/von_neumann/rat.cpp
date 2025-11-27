#include "von_neumann/rat.hpp"

std::optional<RobIndex> RegisterAliasTable::get_mapping(uint8_t arch_reg) const {
    if (arch_reg == 0) return std::nullopt;
    if (!entries_[arch_reg].is_mapped) return std::nullopt;
    return entries_[arch_reg].rob_index;
}

void RegisterAliasTable::set_mapping(uint8_t arch_reg, RobIndex rob_id) {
    if (arch_reg == 0) return;
    entries_[arch_reg].is_mapped = true;
    entries_[arch_reg].rob_index = rob_id;
}

void RegisterAliasTable::clear_if_matches(uint8_t arch_reg, RobIndex rob_id) {
    if (arch_reg == 0) return;
    if (entries_[arch_reg].is_mapped && entries_[arch_reg].rob_index == rob_id) {
        entries_[arch_reg].is_mapped = false;
        entries_[arch_reg].rob_index = 0;
    }
}

void RegisterAliasTable::flush() {
    for (auto& entry : entries_) {
        entry.is_mapped = false;
        entry.rob_index = 0;
    }
}