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

void ExUnit::tick()
{
    
    // Step 1: Process existing work - decrement timers and complete if done
    for (size_t i = 0; i < alus_.size(); ++i)
    {
        auto& slot = alus_[i];
        if (slot.has_value())
        {        
            if (slot->just_dispatched)
            {
                // First cycle after dispatch - clear flag, don't decrement
                slot->just_dispatched = false;
            }
            else
            {
                // Decrement timer for work that's been in-flight
                --slot->cycles_remaining;
                
                if (slot->cycles_remaining <= 0)
                {
                    execute(slot.value());
                    slot.reset();
                }
            }
        }
    }

    // Step 2: Dispatch new work into available slots
    for (size_t i = 0; i < alus_.size(); ++i)
    {
        auto& slot = alus_[i];
        if (!slot.has_value() && !execute_queue_.empty())
        {
            auto ep = execute_queue_.front();
            execute_queue_.pop();
            auto cycles = static_cast<int>(get_op_cycle(ep.op));
            slot.emplace(AluSlot{ep, cycles, true});  // Mark as just dispatched
        }
    }
    
}

void ExUnit::execute(AluSlot& as) {
    size_t result = 0;
    const auto& ops = as.package.operands;

    // Special Logic: SWITCH (Branching)
    if (as.package.op == Operation::SWITCH) {
        // Port 0: Data
        // Port 1: Control (Boolean)
        size_t data = (ops.size() > 0) ? ops[0] : 0;
        bool condition = (ops.size() > 1) ? (ops[1] != 0) : false; 
        
        if (condition) {
            // True Path -> Dest 0
            if (!as.package.destinations.empty()) {
                auto d = as.package.destinations[0];
                token_queue_.emplace(Token{as.package.fp, d.ip, d.port, data});
              ++tokens_produced_this_cycle_;
            }
        } else {
            // False Path -> Dest 1
            if (as.package.destinations.size() > 1) {
                auto d = as.package.destinations[1];
                token_queue_.emplace(Token{as.package.fp, d.ip, d.port, data});
                ++tokens_produced_this_cycle_;
            }
        }
        return; 
    }

    if (as.package.op == Operation::CALL) {
        size_t new_fp = ++fp_counter_;
        size_t current_fp = as.package.fp;
        
        // Convention: The LAST operand is the "Return Node ID" (where the caller wants the result)
        // The LAST destination is the "Linkage Receiver" in the function
        if (ops.empty()) return; 

        size_t ret_node_id = ops.back(); 
        size_t num_args = ops.size() - 1;

        // 1. Pass Arguments
        for(size_t i = 0; i < num_args; ++i) {
            if (i < as.package.destinations.size() - 1) { // -1 because last dest is linkage
                auto d = as.package.destinations[i];
                token_queue_.emplace(Token{new_fp, d.ip, d.port, ops[i]});
                ++tokens_produced_this_cycle_;
            }
        }

        // 2. Pass Linkage (Return FP AND Return Node ID)
        if (!as.package.destinations.empty()) {
            auto d = as.package.destinations.back();
            // Port 0: Old Frame Pointer
            token_queue_.emplace(Token{new_fp, d.ip, 0, current_fp});
            ++tokens_produced_this_cycle_;
            // Port 1: Return Node ID (Dynamic Destination)
            token_queue_.emplace(Token{new_fp, d.ip, 1, ret_node_id});
            ++tokens_produced_this_cycle_;
        }
        return;
    }
    // Special Logic: RETURN (Recursion Exit)
    if (as.package.op == Operation::RETURN) {
        // Input 0: Result
        // Input 1: Return Address (Old FP)
        // Input 2: Return Node ID (Dynamic IP)
        
        if (ops.size() < 3) return; // Error: Missing linkage info
        
        size_t val = ops[0];
        size_t ret_fp = ops[1];
        size_t ret_ip = ops[2];

        // Send result back to caller's context dynamically
        // We assume the caller expects the result on Port 0
        token_queue_.emplace(Token{ret_fp, ret_ip, 0, val});
        ++tokens_produced_this_cycle_;
        return;
    }

    // Standard ALU Logic
    switch (as.package.op) {
        case Operation::ADD:  result = ops[0] + ops[1]; break;
        case Operation::SUB:  result = ops[0] - ops[1]; break;
        case Operation::MULT: result = ops[0] * ops[1]; break;
        case Operation::DIV:  result = (ops[1] == 0) ? 0 : ops[0] / ops[1]; break;
        
        case Operation::SLT:  result = (ops[0] <  ops[1]) ? 1 : 0; break;
        case Operation::SGT:  result = (ops[0] >  ops[1]) ? 1 : 0; break;
        case Operation::SLE:  result = (ops[0] <= ops[1]) ? 1 : 0; break;
        case Operation::SGE:  result = (ops[0] >= ops[1]) ? 1 : 0; break;
        case Operation::EQ:   result = (ops[0] == ops[1]) ? 1 : 0; break;
        case Operation::FORK: result = ops[0]; break;
        case Operation::MERGE: 
            // Logic: SELECT / PHI-NODE
            // Port 0: False Value
            // Port 1: True Value
            // Port 2: Condition
            if (ops.size() >= 3) {
                result = (ops[2] != 0) ? ops[1] : ops[0]; 
            } else {
                // Fallback for safety or partial inputs
                result = (ops.size() > 0) ? ops[0] : 0;
            }
            break; 
        
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
        ++tokens_produced_this_cycle_;
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

uint64_t ExUnit::get_active_alu_count() const {
    uint64_t count = 0;
    for (const auto& slot : alus_) {
        if (slot.has_value()) ++count;
    }
    return count;
}

uint64_t ExUnit::get_tokens_produced_this_cycle() const {
    return tokens_produced_this_cycle_;
}

void ExUnit::reset_cycle_counters() {
    tokens_produced_this_cycle_ = 0;
}