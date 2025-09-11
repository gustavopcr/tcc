#include "decode_stage.hpp"
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

void DecodeStage::decode(uint32_t instruction)
{
  //00000000 00000000 00000000 00000000 
  //u_int32_t decoded_instruction = instruction << 32;
  uint8_t op = (instruction >> 26) & 0x3F;
  if(op == 0) // R-Format)
  {
    uint8_t    rs = (instruction >> 21) & 0x1F;
    uint8_t    rt = (instruction >> 16) & 0x1F;
    uint8_t    rd = (instruction >> 11) & 0x1F;
    uint8_t shamt = (instruction >>  6) & 0x1F;
    uint8_t funct = (instruction >>  0) & 0x3F;
  }
  else if(op == 2 || op == 3) // J-Format
  {
    uint32_t address = instruction & 0x3FFFFFF;
  }
  else // I-Format
  {
    uint8_t         rs = (instruction >> 21) & 0x1F;
    uint8_t         rt = (instruction >> 16) & 0x1F;
    uint16_t immediate = instruction & 0xFFFF;
  }
}