#include "decode_stage.hpp"
#include <iostream>

//Instruction types:
/* 
  I-format:
    op    rs    rt        immediate          example
001000 10011 01010 0000000000000100 addi $t2, $s3, 4

  J-Format:
    op                    address example
000010 00000000000000000100000001  j LOOP   

  R-Format:
    op    rs    rt    rd  shamt  funct             example
000000 10001 10010 10000  00000 100000   add $s0, $s1, $s2
000000 01000 01001 00100  00000 100000
*/

DecodeStage::DecodeStage(uint32_t& pc, std::array<uint32_t, 32>& registers, IF_ID& if_id, ID_EX& id_ex)
: pc_{pc}
, registers_{registers}
, if_id_{if_id}
, id_ex_{id_ex}
{
}


void DecodeStage::run()
{
  id_ex_ = ID_EX{};

  Instruction instruction = decode(if_id_.instruction);
  
  id_ex_.control.alu_op = get_alu_operation(instruction.op, instruction.funct);
  id_ex_.next_pc = if_id_.next_pc;
  id_ex_.read_data_1 = registers_[instruction.rs];
  id_ex_.read_data_2 = registers_[instruction.rt];
  id_ex_.sign_extended_immediate = static_cast<int32_t>(static_cast<int16_t>(instruction.immediate));
  id_ex_.rt = instruction.rt;
  id_ex_.rs = instruction.rs;
  id_ex_.rd = instruction.rd;

  switch (instruction.op)
  {
    case 0x00: // R-type (e.g., add, sub)
      id_ex_.control.alu_src = false;
      id_ex_.control.reg_dst = true;
      id_ex_.control.reg_write = true;
      break;

    case 0x02:
      pc_ = (if_id_.next_pc & 0xF0000000) | (instruction.address << 2);
      id_ex_ = ID_EX{};
      return;
      break;

    case 0x23: // lw (load word)
      id_ex_.control.alu_src = true;
      id_ex_.control.mem_to_reg = true;
      id_ex_.control.reg_write = true;
      id_ex_.control.mem_read = true;
      break;
      
    case 0x2B: // sw (store word)
      id_ex_.control.alu_src = true;
      id_ex_.control.mem_write = true;
      break;

    case 0x04: // beq (branch if equal)
      id_ex_.control.branch = true;
      break;

    case 0x08: // addi (add immediate)
      id_ex_.control.alu_src = true;
      id_ex_.control.reg_dst = false;
      id_ex_.control.reg_write = true;
      break;
  }
}

Instruction decode(uint32_t instruction)
{
  const uint8_t op = (instruction >> 26) & 0x3F;
  Instruction decoded_instruction;
  decoded_instruction.op = op;
  if(op == 0) // R-Format
  {
    decoded_instruction.rs = (instruction >> 21) & 0x1F;
    decoded_instruction.rt = (instruction >> 16) & 0x1F;
    decoded_instruction.rd = (instruction >> 11) & 0x1F;
    decoded_instruction.shamt = (instruction >>  6) & 0x1F;
    decoded_instruction.funct = (instruction >>  0) & 0x3F;
  }
  else if(op == 2 || op == 3) // J-Format
  {
    decoded_instruction.address = instruction & 0x3FFFFFF;
  }
  else // I-Format
  {
    decoded_instruction.rs = (instruction >> 21) & 0x1F;
    decoded_instruction.rt = (instruction >> 16) & 0x1F;
    decoded_instruction.immediate = instruction & 0xFFFF;
  }
  return decoded_instruction;
}

AluOperation get_alu_operation(uint8_t opcode, uint8_t funct) {
  // For lw, sw, and addi, the ALU always does addition
  if (opcode == 0x23 || opcode == 0x2B || opcode == 0x08) {
      return AluOperation::ADD;
  }

  // For beq, the ALU always does subtraction
  if (opcode == 0x04) {
      return AluOperation::SUB;
  }

  // For R-type, we must look at the funct field
  if (opcode == 0x00) {
      switch (funct) {
          case 0x20: return AluOperation::ADD;
          case 0x22: return AluOperation::SUB;
          case 0x24: return AluOperation::AND;
          case 0x25: return AluOperation::OR;
          case 0x2A: return AluOperation::SLT;
          case 0x27: return AluOperation::NOR;
          // Add other R-type functions here
      }
  }

  // Default case (for instructions that don't use the ALU)
  return AluOperation::ADD;
}