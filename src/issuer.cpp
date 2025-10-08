#include "issuer.hpp"

Issuer::Issuer(FetchDecodeQueue& input_queue, IssueQueue& issue_queue)
: input_queue_{input_queue}
, issue_queue_{issue_queue}
{
}


void Issuer::tick()
{
}


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

    case AluOperation::SLT:
      return (static_cast<int32_t>(rs) < static_cast<int32_t>(rt)) ? 1 : 0;
    
    case AluOperation::NOR:
      return ~(rs | rt);
    
    default:
      return 0; 
  }
}