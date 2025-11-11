#ifndef IF_UNIT_HPP
#define IF_UNIT_HPP

#include "dataflow/primitives/token.hpp"
#include "dataflow/primitives/node.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include <queue>

// InstructionFetch Unit
class IfUnit
{
public:
  IfUnit(std::queue<MatchedToken>& matched_tokens_, NodeGraph& node_graph, std::queue<ExecutionPackage>& execute_queue);
  void tick();

private:
  ExecutionPackage fetch_instruction(MatchedToken mt);
  
  std::queue<MatchedToken>& matched_tokens_;
  NodeGraph& node_graph_;
  std::queue<ExecutionPackage>& execute_queue_;
};
#endif