#include "dataflow/units/ex_unit.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include "dataflow/primitives/operation.hpp"
#include <stdexcept>

ExUnit::ExUnit(std::queue<ExecutionPackage>& execute_queue, std::queue<Token>& token_queue)
: execute_queue_{execute_queue}
, token_queue_{token_queue}
{
}

OperationCycles ExUnit::get_op_cycle(Operation op) const
{
  switch (op)
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
  for(auto& a:alus_) // executes currently used ALUs
  {
    if(a.has_value())
    {
      --a->cycles_remaining;
      if(a->cycles_remaining <= 0)
      {
        execute(a.value());
        a.reset();
      }
    }
  }

  for (auto& slot : alus_) // finds work for empty ALUs
  {
    if (!slot.has_value())
    {
      if (execute_queue_.empty())
      {
        break; 
      }

      auto ep = execute_queue_.front();
      execute_queue_.pop();
      auto cycles = get_op_cycle(ep.op);
      slot.emplace(AluSlot{ep, static_cast<int>(cycles)});
    }
  }
}

void ExUnit::execute(AluSlot& as)
{
  size_t result = 0;
  if (as.package.op == Operation::SWITCH)
  {
    // operands[0] = The data value to route
    // operands[1] = The boolean control value (0=F, non-zero=T)
    // destinations[0] = Destination if TRUE
    // destinations[1] = Destination if FALSE 
    size_t data = as.package.operands[0];
    bool control = as.package.operands[1];
    const auto& dest = control ? as.package.destinations[0] : as.package.destinations[1];
    
    token_queue_.emplace(Token{as.package.fp, dest.ip, dest.port, data});
    return;
  }

  switch (as.package.op)
  {
  case Operation::ADD:
    result = as.package.operands[0] + as.package.operands[1];
    break;
  case Operation::SUB:
    result = as.package.operands[0] - as.package.operands[1];
    break;
  
  case Operation::MULT:
    result = as.package.operands[0] * as.package.operands[1];
    break;
  case Operation::DIV:
    result = as.package.operands[0] / as.package.operands[1];
    break;
  case Operation::FORK:
    result = as.package.operands[0];
    break;
  case Operation::MERGE:
    bool control = as.package.operands[2];
    result = control ? as.package.operands[0] : as.package.operands[1];
    break;
  default:
    throw std::runtime_error("UNSUPPORTED INSTRUCTION");
  }

  for(auto dest : as.package.destinations)
  {
    token_queue_.emplace(Token{as.package.fp, dest.ip, dest.port, result});
  }
    
}