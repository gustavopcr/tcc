#include "dataflow/units/ex_unit.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include "dataflow/primitives/operation.hpp"
#include <stdexcept>

OperationCycles ExUnit::get_op_cycle(ExecutionPackage ep) const
{
  switch (ep.op)
  {
  case Operation::ADD:
    return OperationCycles::ADD;
  case Operation::SUB:
    return OperationCycles::SUB;
  case Operation::MULT:
    return OperationCycles::MULT;
  case Operation::DIV:
    return OperationCycles::DIV;
  case Operation::FORK:
    return OperationCycles::FORK;
  case Operation::SWITCH:
    return OperationCycles::SWITCH;
  case Operation::MERGE:
    return OperationCycles::MERGE;
  default:
    throw std::runtime_error("UNSUPPORTED INSTRUCTION");
  }
}
void ExUnit::tick()
{

  while(alus_.size() < ALU_AMOUNT)
  {
    auto ep = execute_queue_.front();
    auto cycles = get_op_cycle(ep);
    alus_.emplace_back(AluSlot{ep, static_cast<int>(cycles)});
    execute_queue_.pop();
  }

  for(auto a:alus_)
  {
    execute(a);
  }

  return;
}

void ExUnit::execute(AluSlot as)
{
  --as.cycles_remaining;
  if(as.cycles_remaining <= 0)
  {
    size_t result = 0;
    switch (as.package.op)
    {
    case Operation::ADD:
      result = as.package.operands[0] + as.package.operands[1];
    case Operation::SUB:
      result = as.package.operands[0] - as.package.operands[1];
    case Operation::MULT:
      result = as.package.operands[0] * as.package.operands[1];
    case Operation::DIV:
      result = as.package.operands[0] / as.package.operands[1];
    case Operation::FORK:
      result = as.package.operands[0] + as.package.operands[1]; // ???
    case Operation::SWITCH:
      result = as.package.operands[0] + as.package.operands[1]; // ???
    case Operation::MERGE:
      result = as.package.operands[0] + as.package.operands[1]; // ???
    default:
      throw std::runtime_error("UNSUPPORTED INSTRUCTION");
    }

    for(auto dest : as.package.destinations)
    {
      token_queue_.emplace(Token{0, dest.ip, dest.port, result});
    }
  }
}