#include "von_neumann/alu.hpp"

uint32_t alu(AluOperation op, uint32_t rs, uint32_t rt)
{
  switch (op)
  {
    case AluOperation::ADD:
      return rs + rt;

    case AluOperation::SUB:
      return rs - rt;

    case AluOperation::AND:
      return rs & rt;

    case AluOperation::OR:
      return rs | rt;

    case AluOperation::XOR:
      return rs ^ rt;

    case AluOperation::SLT:
      return (static_cast<int32_t>(rs) < static_cast<int32_t>(rt)) ? 1 : 0;
    
    case AluOperation::NOR:
      return ~(rs | rt);

    case AluOperation::SLL:
      return rs << rt;  // rs (value) shifted left by rt (shift amount)

    case AluOperation::SRL:
      return rs >> rt;  // rs (value) shifted right by rt (shift amount)
    case AluOperation::MUL: return rs * rt;   // NEW: Multiply (lower 32 bits)
    default:
      return 0; 
  }
}