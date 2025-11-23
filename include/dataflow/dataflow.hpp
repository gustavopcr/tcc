#ifndef DATAFLOW_HPP
#define DATAFLOW_HPP
#include "dataflow/primitives/token.hpp"
#include "dataflow/primitives/node.hpp"
#include "dataflow/primitives/execution_pacakge.hpp"
#include "dataflow/units/wm_unit.hpp"
#include "dataflow/units/if_unit.hpp"
#include "dataflow/units/ex_unit.hpp"
#include "dataflow/units/mem_unit.hpp"

#include <queue>
#include <string>
class Dataflow
{
public:
  Dataflow();
  void Dataflow::run(const std::vector<uint64_t>& initial_data_values);

  void load_program(std::string_view graph_file);
  void set_inputs(const std::vector<uint64_t>& input_data);
  
private:
  NodeGraph node_graph_;
  FiringRuleMap fr_rules_;
  std::queue<Token> token_queue_;
  std::queue<MatchedToken> matched_token_queue_;
  std::queue<ExecutionPackage> execution_queue_;
  bool is_idle() const;

  MemoryUnit mem_unit_;
  WmUnit wm_unit_;
  IfUnit if_unit_;
  ExUnit ex_unit_;

  std::vector<std::vector<Destination>> input_destinations_;
};

Dataflow create_dataflow(std::string_view graph_file);

#endif