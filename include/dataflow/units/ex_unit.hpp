#ifndef EX_UNIT_HPP
#define EX_UNIT_HPP

#include "dataflow/primitives/token.hpp"
#include "dataflow/primitives/node.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
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
};

// Execution Unit
class ExUnit
{
public:
  void tick();
private:
  void execute(AluSlot& as);
  OperationCycles get_op_cycle(Operation op) const;
  std::array<std::optional<AluSlot>, ALU_AMOUNT> alus_;
  std::queue<ExecutionPackage>& execute_queue_; // input
  std::queue<Token>& token_queue_; // output
};
#endif