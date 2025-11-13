#ifndef DATAFLOW_HPP
#define DATAFLOW_HPP
#include "dataflow/primitives/token.hpp"
#include "dataflow/primitives/node.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include "dataflow/units/wm_unit.hpp"
#include "dataflow/units/if_unit.hpp"
#include "dataflow/units/ex_unit.hpp"
#include <queue>
#include <string>
class Dataflow
{
public:
  Dataflow();
  void run();
  void load_program(std::string_view graph_file);
private:
  NodeGraph node_graph_;
  FiringRuleMap fr_rules_;
  std::queue<Token> token_queue_;
  std::queue<MatchedToken> matched_token_queue_;
  std::queue<ExecutionPackage> execution_queue_;
  bool is_idle() const;

  WmUnit wm_unit_;
  IfUnit if_unit_;
  ExUnit ex_unit_;
};

Dataflow create_dataflow(std::string_view graph_file);

#endif