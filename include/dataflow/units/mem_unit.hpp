#ifndef MEMORY_UNIT_HPP
#define MEMORY_UNIT_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iostream>

class MemoryUnit {
public:
    explicit MemoryUnit(size_t size_bytes) : memory_(size_bytes, 0) {
    }

    uint64_t load(size_t address) const {
        if (address >= memory_.size()) {
            throw std::runtime_error("Segmentation Fault: LOAD Access Violation at " + std::to_string(address));
        }
        return memory_[address];
    }

    void store(size_t address, uint64_t value) {
        if (address >= memory_.size()) {
            throw std::runtime_error("Segmentation Fault: STORE Access Violation at " + std::to_string(address));
        }
        memory_[address] = value;
    }

    // Helper to preload data for simulation
    void direct_write(size_t address, uint64_t value) {
        if (address < memory_.size()) memory_[address] = value;
    }

private:
    std::vector<uint64_t> memory_;
};

#endif