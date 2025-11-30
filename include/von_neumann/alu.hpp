#ifndef ALU_HPP
#define ALU_HPP

#include <cstdint>

enum class AluOperation : uint8_t {
    AND = 0b0000,
    OR  = 0b0001,
    ADD = 0b0010,
    XOR = 0b0011,   // NEW: Bitwise XOR
    SLL = 0b0100,   // NEW: Shift Left Logical
    SRL = 0b0101,   // NEW: Shift Right Logical
    SUB = 0b0110,
    SLT = 0b0111,
    NOR = 0b1100,
    STORE = 0b1110,
    LOAD = 0b1111
};

uint32_t alu(AluOperation op, uint32_t rs, uint32_t rt);

#endif