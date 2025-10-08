#ifndef ISSUER_H
#define ISSUER_H

#include "alu.hpp"

enum InstructionType : uint8_t
{
  R_TYPE,
  J_TYPE,
  I_TYPE
};

class Issuer{
public:
  explicit Issuer();
  void tick();

private:
};

uint32_t alu(AluOperation op, uint32_t rs, uint32_t rt);

#endif