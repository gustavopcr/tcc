#ifndef EX_UNIT_HPP
#define EX_UNIT_HPP

#include "dataflow/primitives/token.hpp"
#include "dataflow/primitives/node.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include "dataflow/units/mem_unit.hpp"
#include "dataflow/dataflow_stats.hpp"
#include <vector>
#include <queue>
#include <utility>
#include <array>
#include <optional>

constexpr auto ALU_AMOUNT = 4;
struct AluSlot
{
  ExecutionPackage package;
  int cycles_remaining;
  bool just_dispatched = false;  // Prevents decrement on dispatch cycle
};
// Execution Unit
class ExUnit
{
public:
  ExUnit(std::queue<ExecutionPackage>& execute_queue, 
           std::queue<Token>& token_queue,
           MemoryUnit& mem_unit);
  void tick();
  bool is_idle() const;
  uint64_t get_active_alu_count() const;
  uint64_t get_tokens_produced_this_cycle() const;
  void reset_cycle_counters();

private:
  void execute(AluSlot& as);
  size_t fp_counter_{0};
  MemoryUnit& mem_unit_;
  std::array<std::optional<AluSlot>, ALU_AMOUNT> alus_;
  std::queue<ExecutionPackage>& execute_queue_; // input
  std::queue<Token>& token_queue_; // output
  uint64_t tokens_produced_this_cycle_ = 0;  // NEW
};
#endif