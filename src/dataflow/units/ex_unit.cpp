#include "dataflow/units/ex_unit.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include "dataflow/primitives/operation.hpp"
#include <stdexcept>

ExUnit::ExUnit(std::queue<ExecutionPackage>& execute_queue, 
               std::queue<Token>& token_queue,
               MemoryUnit& mem_unit)
    : execute_queue_{execute_queue}
    , token_queue_{token_queue}
    , mem_unit_{mem_unit} 
{}

OperationCycles ExUnit::get_op_cycle(Operation op) const {
    switch(op) {
        case Operation::ADD: return OperationCycles::ADD;
        case Operation::SUB: return OperationCycles::SUB;
        case Operation::MULT: return OperationCycles::MULT;
        case Operation::DIV: return OperationCycles::DIV;
        case Operation::FORK: return OperationCycles::FORK;
        case Operation::SWITCH: return OperationCycles::SWITCH;
        case Operation::MERGE: return OperationCycles::MERGE;
        case Operation::LOAD: return OperationCycles::LOAD;
        case Operation::STORE: return OperationCycles::STORE;
        case Operation::INPUT: return OperationCycles::INPUT;
        case Operation::OUTPUT: return OperationCycles::OUTPUT;
        default: return OperationCycles::ADD;
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

void ExUnit::execute(AluSlot& as) {
    size_t result = 0;
    const auto& ops = as.package.operands;

    // Special Logic: SWITCH (Branching)
    if (as.package.op == Operation::SWITCH) {
        size_t data = ops[0];
        bool condition = (ops[1] != 0); // Port 1 is control
        
        // If True, output to Dest[0]. If False, output to Dest[1] (if exists)
        if (condition && !as.package.destinations.empty()) {
            auto d = as.package.destinations[0];
            token_queue_.emplace(Token{as.package.fp, d.ip, d.port, data});
        } else if (!condition && as.package.destinations.size() > 1) {
            auto d = as.package.destinations[1];
            token_queue_.emplace(Token{as.package.fp, d.ip, d.port, data});
        }
        return; 
    }

    // Standard ALU Logic
    switch (as.package.op) {
        case Operation::ADD:  result = ops[0] + ops[1]; break;
        case Operation::SUB:  result = ops[0] - ops[1]; break;
        case Operation::MULT: result = ops[0] * ops[1]; break;
        case Operation::DIV:  result = (ops[1] == 0) ? 0 : ops[0] / ops[1]; break;
        case Operation::FORK: result = ops[0]; break;
        case Operation::MERGE: result = (ops[2]) ? ops[1] : ops[0]; break; // Select
        
        // --- MEMORY OPS VIA MEMORY UNIT ---
        case Operation::LOAD: 
            // Input: Address. Output: Data
            result = mem_unit_.load(ops[0]); 
            break;
        case Operation::STORE:
            // Input: Addr (0), Data (1). Output: Signal (Data)
            mem_unit_.store(ops[0], ops[1]);
            result = ops[1]; // Pass data through as acknowledgment
            break;

        case Operation::OUTPUT:
            std::cout << ">> OUTPUT: " << ops[0] << std::endl;
            return; // Terminate token
            
        default: break;
    }

    // Broadcast result
    for (const auto& dest : as.package.destinations) {
        token_queue_.emplace(Token{as.package.fp, dest.ip, dest.port, result});
    }
}


bool ExUnit::is_idle() const
{
  for (const auto& slot : alus_)
  {
    if (slot.has_value())
    {
      return false;
    }
  }
  return true;
}