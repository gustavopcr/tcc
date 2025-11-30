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
      return rt << rs;  // rt shifted left by rs (shamt goes in rs for R-type)

    case AluOperation::SRL:
      return rt >> rs;  // rt shifted right by rs (shamt goes in rs for R-type)
    
    default:
      return 0; 
  }
}