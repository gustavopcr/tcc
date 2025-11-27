#ifndef DECODER_HPP
#define DECODER_HPP

#include "von_neumann/alu.hpp"
#include "von_neumann/instruction.hpp"
#include <cstdint>

// Free functions for instruction decoding (reusable)
Instruction decode(uint32_t instruction);
AluOperation get_alu_operation(uint8_t opcode, uint8_t funct);

#endif