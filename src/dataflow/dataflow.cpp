#include "dataflow.hpp"
#include <fstream>
#include <sstream>

Operation operation_from_string(const std::string& op_str)
{
  if (op_str == "ADD")   return Operation::ADD;
  if (op_str == "SUB")   return Operation::SUB;
  if (op_str == "MULT")  return Operation::MULT;
  if (op_str == "DIV")   return Operation::DIV;
  if (op_str == "FORK")  return Operation::FORK;
  if (op_str == "SWITCH") return Operation::SWITCH;
  if (op_str == "MERGE")  return Operation::MERGE;
  throw std::runtime_error("Unknown operation: " + op_str);
}

Dataflow::Dataflow()
  // The initializer list "links" all the components
  : node_graph_(),        // Default-construct maps and queues
    fr_rules_(),
    token_queue_(),
    matched_token_queue_(),
    execution_queue_(),
    
    // --- This is the "wiring" ---
    
    // WmUnit needs the Firing Rules and the *output* queue
    // WmUnit(std::unordered_map<TokenTag, std::vector<Token>> wt_mem, FiringRuleMap& fr, std::queue<MatchedToken>& matched_tokens_);
    
    wm_unit_(fr_rules_, matched_token_queue_),

    // IfUnit needs the NodeGraph, *input* queue, and *output* queue
  // IfUnit(std::queue<MatchedToken>& matched_tokens_, NodeGraph& node_graph, std::queue<ExecutionPackage>& execute_queue);

    if_unit_(matched_token_queue_, node_graph_, execution_queue_),

    // ExUnit needs the *input* queue and the *output* queue
    ex_unit_(execution_queue_, token_queue_)
{
  // The constructor body is empty!
  // All initialization is done.
}

Dataflow create_dataflow(std::string_view graph_file)
{
  // 1. Create the processor. The constructor
  //    wires everything up.
  Dataflow df;

  // 2. Load the program into the processor's maps.
  df.load_program(graph_file);

  // 3. Return the fully initialized object.
  return df;
}


// Conceptual implementation of the loader method
void Dataflow::load_program(std::string_view graph_file)
{
  std::ifstream file(graph_file.data());
  if (!file.is_open()) {
    throw std::runtime_error("Could not open graph file.");
  }

  std::string line;
  size_t ip;
  std::string op_str;
  size_t num_inputs;

  // This is a conceptual parser. You will need to build
  // one based on your ISA (see below).
  while (std::getline(file, line))
  {
    std::stringstream ss(line);
    ss >> ip >> op_str >> num_inputs;
    
    Node node;
    node.tag = ip;
    node.op = operation_from_string(op_str); // You'll need this helper
    node.num_inputs = num_inputs;

    // Parse destinations like "12:0" (ip:port)
    std::string dest_str;
    while (ss >> dest_str) {
      size_t colon_pos = dest_str.find(':');
      size_t dest_ip = std::stoull(dest_str.substr(0, colon_pos));
      uint8_t dest_port = std::stoul(dest_str.substr(colon_pos + 1));
      node.destinations.push_back({dest_ip, dest_port});
    }

    // --- The two crucial steps ---
    // 1. Store the node blueprint
    this->node_graph_[ip] = node;
    
    // 2. Store the "firing rule" for the WM Unit
    this->fr_rules_[ip] = num_inputs;
  }
}