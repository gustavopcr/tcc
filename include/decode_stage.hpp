#ifndef DECODE_STAGE_H
#define DECODE_STAGE_H

#include "pipeline_register.hpp"
#include "alu_operation.hpp"
#include <cstdint>
#include <array>

struct Instruction{
  uint8_t    op;
  uint8_t    rs;
  uint8_t    rt;
  uint8_t    rd;
  uint8_t shamt;
  uint8_t funct;

  uint16_t immediate;
  uint32_t   address;
};



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

class DecodeStage{
public:
  explicit DecodeStage(uint32_t& pc, std::array<uint32_t, 32>& registers, IF_ID& if_id, ID_EX& id_ex);
  void run();

private:
  uint32_t& pc_;
  std::array<uint32_t, 32>& registers_;
  IF_ID& if_id_;
  ID_EX& id_ex_;
};

Instruction decode(uint32_t instruction);
AluOperation get_alu_operation(uint8_t opcode, uint8_t funct);
#endif