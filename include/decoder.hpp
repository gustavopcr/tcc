#ifndef DECODER_HPP
#define DECODER_HPP

#include "alu.hpp"
#include "instruction.hpp"
#include "register.hpp"
#include "reservation_station.hpp"
#include "rat.hpp"
#include <cstdint>
#include <array>

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

class Decoder{
public:
  explicit Decoder(FetchDecodeQueue& input_queue, 
                   ReservationStation& reservation_station, 
                   RegisterAliasTable& rat,
                   RegisterBank& registers);
  void tick();

private:
  FetchDecodeQueue& input_queue_;
  ReservationStation& reservation_station_;
  RegisterAliasTable& rat_;
  RegisterBank& registers_;
};

Instruction decode(uint32_t instruction);
AluOperation get_alu_operation(uint8_t opcode, uint8_t funct);
#endif