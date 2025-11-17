#include "dataflow.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

Operation operation_from_string(const std::string& op_str)
{
 if (op_str == "ADD")  return Operation::ADD;
 if (op_str == "SUB")  return Operation::SUB;
 if (op_str == "MULT") return Operation::MULT;
 if (op_str == "DIV")  return Operation::DIV;
 if (op_str == "FORK") return Operation::FORK;
 if (op_str == "SWITCH") return Operation::SWITCH;
 if (op_str == "MERGE") return Operation::MERGE;
 if (op_str == "LOAD") return Operation::LOAD;
 if (op_str == "STORE") return Operation::STORE;
 if (op_str == "INPUT") return Operation::INPUT;
 if (op_str == "OUTPUT") return Operation::OUTPUT;
 throw std::runtime_error("Unknown operation: " + op_str);
}

Dataflow::Dataflow()
  // The initializer list "links" all the components
  : node_graph_()       // Default-construct maps and queues
  , fr_rules_()
  , token_queue_()
  , matched_token_queue_()
  , execution_queue_()
  , wm_unit_(fr_rules_, matched_token_queue_)
  , if_unit_(matched_token_queue_, node_graph_, execution_queue_)
  , ex_unit_(execution_queue_, token_queue_)
{
}


// We assume your 'token.hpp' defines a 'Data' type and
// a 'Token' class with a constructor like:
// Token(const Data& data, size_t dest_ip, uint8_t dest_port);

void Dataflow::run(const std::vector<uint64_t>& initial_data_values)
{
  const size_t MAX_CYCLES = 1000000; 
  size_t current_cycle = 0;
std::vector<Destination> input_destinations_;
// In Dataflow.cpp, inside run()
std::cout << "Bootstrapping dataflow..." << std::endl;
if (initial_data_values.size() <= 0) {
    throw std::runtime_error("Input data count mismatch.");
}

for (size_t i = 0; i < initial_data_values.size(); ++i)
{
    const uint64_t& data_value = initial_data_values[i];
    const std::vector<Destination>& dests = input_destinations_[i];

    for (const Destination& dest : dests)
    {
        // We can fill out most of the token:
        
        Token t;
        t.v = data_value;            // from initial_data_values
        t.ip = dest.ip;              // from the INPUT line destination
        t.p = dest.port;             // from the INPUT line port
        t.fp = 0;                  // <--- This is the missing piece
        
        token_queue_.push(t);
    }
}
std::cout << "Dataflow simulation starting..." << std::endl;

 // --- 2. SIMULATION LOOP (Mostly unchanged) ---
  while (!is_idle() && current_cycle < MAX_CYCLES)
  {
    // --- 1. Waiting-Match Stage ---
    while (!token_queue_.empty())
    {
    Token t = token_queue_.front();
    token_queue_.pop();
    wm_unit_.tick(t);
    }

    // --- 2. Instruction-Fetch Stage ---
    while (!matched_token_queue_.empty())
    {
    if_unit_.tick();
    }

    // --- 3. Execution Stage ---
    ex_unit_.tick();

      // --- FIX ---
    current_cycle++; // You forgot to increment the cycle
 }
 
  if (current_cycle >= MAX_CYCLES) {
      std::cout << "Simulation aborted: Exceeded max cycles." << std::endl;
  }
 std::cout << "Dataflow program finished in " << current_cycle << " cycles." << std::endl;
}

Dataflow create_dataflow(std::string_view graph_file)
{
  Dataflow df;
  df.load_program(graph_file);
  return df;
}

void Dataflow::load_program(std::string_view graph_file)
{
 std::ifstream file(graph_file.data());
 if (!file.is_open()) {
  throw std::runtime_error("Could not open graph file.");
 }

 std::string line;
 while (std::getline(file, line))
 {
    // Skip empty lines or comments
    if (line.empty() || line[0] == '#') {
        continue;
    }

    std::stringstream ss(line);
    size_t ip;
    std::string op_str;
    size_t num_inputs;

    if (!(ss >> ip >> op_str >> num_inputs)) continue; // Malformed line

    // --- MODIFIED LOGIC ---
    if (op_str == "INPUT")
    {
        // INPUT nodes are "injection" rules, not real nodes.
        // We just read their destinations and store them.
        std::vector<Destination> dests;
        std::string dest_str;
        while (ss >> dest_str) {
            size_t colon_pos = dest_str.find(':');
            if (colon_pos == std::string::npos) continue; // Skip malformed
            size_t dest_ip = std::stoull(dest_str.substr(0, colon_pos));
            uint8_t dest_port = std::stoul(dest_str.substr(colon_pos + 1));
            dests.push_back({dest_ip, dest_port});
        }
        // Store this list of destinations.
        this->input_destinations_.push_back(dests);
    }
    else
    {
        // This is a NORMAL node (ADD, SUB, OUTPUT, etc.)
        Node node;
        node.tag = ip;
        node.op = operation_from_string(op_str);
        node.num_inputs = num_inputs;

        // Parse destinations
        std::string dest_str;
        while (ss >> dest_str) {
            size_t colon_pos = dest_str.find(':');
            if (colon_pos == std::string::npos) continue; // Skip malformed
            size_t dest_ip = std::stoull(dest_str.substr(0, colon_pos));
            uint8_t dest_port = std::stoul(dest_str.substr(colon_pos + 1));
            node.destinations.push_back({dest_ip, dest_port});
        }

        // --- Store the node and its firing rule ---
        this->node_graph_[ip] = node;
        this->fr_rules_[ip] = num_inputs;
    }
 }
}

bool Dataflow::is_idle() const
{
  return token_queue_.empty() &&
         matched_token_queue_.empty() &&
         execution_queue_.empty() &&
         wm_unit_.is_idle() &&
         ex_unit_.is_idle();
}