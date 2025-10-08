#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <queue>
#include <cstdint>

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

enum InstructionType : uint8_t
{
  R_TYPE,
  J_TYPE,
  I_TYPE
};

const size_t MAX_FETCH_DECODE_QUEUE_SIZE = 10;
using FetchDecodeQueue = std::queue<uint32_t>;
#endif