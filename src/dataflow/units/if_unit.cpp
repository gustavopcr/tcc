#include "if_unit.hpp"

void IfUnit::tick()
{
  if(!matched_tokens_.empty())
  {
    auto mt = matched_tokens_.front();
    auto ep = fetch_instruction(mt);
    execute_queue_.push(ep);
  }
}

/*
struct ExecutionPackage
{
  size_t fp;
  Operation op;
  std::vector<size_t> operands;
  std::vector<Destination> destinations;
};
*/
ExecutionPackage IfUnit::fetch_instruction(MatchedToken mt)
{
  auto node = node_graph_.at(mt.ip);
  ExecutionPackage ep{mt.fp, node.op, mt.operands, node.destinations};
  return ep;
}