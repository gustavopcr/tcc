#include "if_unit.hpp"

IfUnit::IfUnit(std::queue<MatchedToken>& matched_tokens, NodeGraph& node_graph, std::queue<ExecutionPackage>& execute_queue)
: matched_tokens_{matched_tokens}
, node_graph_{node_graph}
, execute_queue_{execute_queue}
{
}

void IfUnit::tick()
{
  if(!matched_tokens_.empty())
  {
    auto mt = matched_tokens_.front();
    auto ep = fetch_instruction(mt);
    execute_queue_.push(ep);
  }
}

ExecutionPackage IfUnit::fetch_instruction(MatchedToken mt)
{
  auto node = node_graph_.at(mt.ip);
  ExecutionPackage ep{mt.fp, node.op, mt.operands, node.destinations};
  return ep;
}