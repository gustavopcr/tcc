#include "von_neumann/decoder.hpp"

// Keep only the free functions - the Decoder class is deprecated

Instruction decode(uint32_t instruction)
{
    Instruction inst{};
    inst.op = (instruction >> 26) & 0x3F;
    inst.rs = (instruction >> 21) & 0x1F;
    inst.rt = (instruction >> 16) & 0x1F;
    inst.rd = (instruction >> 11) & 0x1F;
    inst.shamt = (instruction >> 6) & 0x1F;
    inst.funct = instruction & 0x3F;
    inst.immediate = instruction & 0xFFFF;
    inst.address = instruction & 0x3FFFFFF;
    
    // Sign-extend immediate for I-type instructions
    if (inst.immediate & 0x8000) {
        inst.immediate |= 0xFFFF0000;
    }
    
    return inst;
}

// AluOperation get_alu_operation(uint8_t opcode, uint8_t funct) {
//     if (opcode == 0x00) { // R-type
//         switch (funct) {
//             case 0x20: return AluOperation::ADD;
//             case 0x22: return AluOperation::SUB;
//             case 0x24: return AluOperation::AND;
//             case 0x25: return AluOperation::OR;
//             case 0x2A: return AluOperation::SLT;
//             default:   return AluOperation::ADD;
//         }
//     } else { // I-type
//         switch (opcode) {
//             case 0x08: return AluOperation::ADD;  // addi
//             case 0x23: return AluOperation::LOAD; // lw
//             case 0x2B: return AluOperation::STORE; // sw
//             default:   return AluOperation::ADD;
//         }
//     }
// }