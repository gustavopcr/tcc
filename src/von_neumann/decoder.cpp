#include "von_neumann/decoder.hpp"

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

Decoder::Decoder(FetchDecodeQueue& input_queue, 
                 ReservationStation& reservation_station, 
                 RegisterAliasTable& rat,
                 ReorderBuffer& rob,
                 RegisterBank& registers)
: input_queue_{input_queue}
, reservation_station_{reservation_station}
, rat_{rat}
, rob_{rob}
, registers_{registers}
{
}

void Decoder::tick()
{
  if(input_queue_.empty() || reservation_station_.is_full() || rob_.is_full())
  {
    return;
  }

  uint32_t inst = input_queue_.front();
  input_queue_.pop();
  Instruction instruction = decode(inst);

  ReservationStationEntry rs_entry{};
  rs_entry.op = get_alu_operation(instruction.op, instruction.funct);

  uint8_t rs_tag = rat_.get_tag(instruction.rs);
  if(rs_tag == 0) //
  {
    rs_entry.src1_is_ready = true;
    rs_entry.Vj = registers_[instruction.rs];
  }
  else
  {
    rs_entry.src1_is_ready = false;
    rs_entry.Qj = rs_tag;
  }

  uint8_t dest_reg = 0;
  switch (instruction.op)
  {
    case 0x00: // R-type (e.g., add, sub)
    {
      uint8_t rt_tag = rat_.get_tag(instruction.rt);
      if(rt_tag == 0)
      {
        rs_entry.src2_is_ready = true;
        rs_entry.Vk = registers_[instruction.rt];
      }
      else
      {
        rs_entry.src2_is_ready = false;
        rs_entry.Qk = rt_tag;
      }
      dest_reg = instruction.rd;
      break;
    }
    case 0x08: // addi (add immediate)
      rs_entry.src2_is_ready = true;
      rs_entry.Vk = instruction.immediate;
      dest_reg = instruction.rt;
      break;

    case 0x02:
      return;
      break;

    case 0x23: // lw (load word)
      rs_entry.src2_is_ready = true;
      rs_entry.Vk = instruction.immediate;
      dest_reg = instruction.rt;
      break;
      
    case 0x2B: // sw (store word)
    {
      uint8_t rt_tag = rat_.get_tag(instruction.rt);
      if(rt_tag == 0)
      {
        rs_entry.src2_is_ready = true;
        rs_entry.Vk = registers_[instruction.rt];
      }else
      {
        rs_entry.src2_is_ready = false;
        rs_entry.Qk = rt_tag;
      }
      break;
    }
    case 0x04: // beq (branch if equal)
      return;
      break;
  }

  RobEntry rob_entry{.is_busy = true,
                      .operation = rs_entry.op,
                      .state = RobState::Waiting,
                      .arch_dest_reg = dest_reg,
                      .physical_dest_reg = 0, // to be assigned
                      .result_value = 0};
                
  uint8_t rob_tag = rob_.add(rob_entry);
  rs_entry.tag = rob_tag;
  rat_.set_busy(dest_reg, rob_tag);

  
  reservation_station_.add(rs_entry);
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