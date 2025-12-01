#include "dataflow/dataflow.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>


Dataflow::Dataflow()
    : node_graph_()
    , fr_rules_()
    , input_destinations_()
    , token_queue_()
    , matched_token_queue_()
    , execution_queue_()
    , mem_unit_(1024) // Initialize 1024 words of RAM
    , wm_unit_(fr_rules_, matched_token_queue_)
    , if_unit_(matched_token_queue_, node_graph_, execution_queue_)
    , ex_unit_(execution_queue_, token_queue_, mem_unit_) // Pass MemUnit
{
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
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        size_t ip;
        std::string op_str;
        size_t num_inputs;

        if (!(ss >> ip >> op_str >> num_inputs)) continue; 

        if (op_str == "INPUT")
        {
            // --- Input Node Logic (Not a real node, just destinations) ---
            std::vector<Destination> dests;
            std::string dest_str;
            while (ss >> dest_str) {
                size_t colon_pos = dest_str.find(':');
                if (colon_pos == std::string::npos) continue;
                
                size_t dest_ip = std::stoull(dest_str.substr(0, colon_pos));
                uint8_t dest_port = std::stoul(dest_str.substr(colon_pos + 1));
                dests.push_back({dest_ip, dest_port});
            }
            input_destinations_.push_back(dests);
        }
        else
        {
            // --- Standard Node Logic ---
            Node node;
            node.tag = ip;
            node.op = operation_from_string(op_str);
            node.num_inputs = num_inputs;

            std::string dest_str;
            while (ss >> dest_str) {
                size_t colon_pos = dest_str.find(':');
                if (colon_pos == std::string::npos) continue;
                size_t dest_ip = std::stoull(dest_str.substr(0, colon_pos));
                uint8_t dest_port = std::stoul(dest_str.substr(colon_pos + 1));
                node.destinations.push_back({dest_ip, dest_port});
            }

            this->node_graph_[ip] = node;
            this->fr_rules_[ip] = num_inputs;
        }
    }
}

void Dataflow::set_inputs(const std::vector<uint64_t>& input_data)
{
    std::cout << "Injecting inputs..." << std::endl;

    if (input_data.size() != input_destinations_.size()) {
        throw std::runtime_error("Input count mismatch: Graph expects " + 
            std::to_string(input_destinations_.size()) + ", got " + 
            std::to_string(input_data.size()));
    }

    for (size_t i = 0; i < input_data.size(); ++i)
    {
        const uint64_t& data_value = input_data[i];
        const std::vector<Destination>& dests = input_destinations_[i];

        for (const Destination& dest : dests)
        {
            Token t;
            t.v = data_value;       
            t.ip = dest.ip;         
            t.p = dest.port;        
            t.fp = 0; 
            
            token_queue_.push(t);
        }
    }
}

// ...existing code...

void Dataflow::run(const std::vector<uint64_t>& initial_data_values)
{
    const size_t MAX_CYCLES = 100000; 
    size_t current_cycle = 0;

    std::cout << "Bootstrapping dataflow..." << std::endl;

    set_inputs(initial_data_values);
    stats_.tokens_generated += initial_data_values.size();  // NEW: Count input tokens

    std::cout << "Dataflow simulation starting..." << std::endl;

    // --- Main Loop ---
    while (!is_idle() && current_cycle < MAX_CYCLES)
    {
        ex_unit_.reset_cycle_counters();  // NEW
        
        // 1. Waiting-Match (Consume tokens)
        while (!token_queue_.empty())
        {
            Token t = token_queue_.front();
            token_queue_.pop();
            stats_.tokens_consumed++;  // NEW
            wm_unit_.tick(t);
        }

        // 2. Instruction Fetch (Consume matched sets)
        while (!matched_token_queue_.empty())
        {
            if_unit_.tick();
        }

        // 3. Execution (Process ALUs)
        ex_unit_.tick();
        
        // NEW: Collect per-cycle metrics
        uint64_t active_alus = ex_unit_.get_active_alu_count();
        if (active_alus > 0) {
            stats_.cycles_with_activity++;
            stats_.total_active_alus += active_alus;
            stats_.max_parallel_tasks = std::max(stats_.max_parallel_tasks, active_alus);
        }
        
        stats_.tokens_generated += ex_unit_.get_tokens_produced_this_cycle();
        
        uint64_t ms_occupancy = wm_unit_.get_current_occupancy();
        stats_.max_matching_store_size = std::max(stats_.max_matching_store_size, ms_occupancy);

        current_cycle++; 
    }
    
    stats_.total_cycles = current_cycle;  // NEW

    if (current_cycle >= MAX_CYCLES) {
        std::cout << "Simulation aborted: Exceeded max cycles." << std::endl;
    } else {
        std::cout << "Finished in " << current_cycle << " cycles." << std::endl;
        
        // NEW: Print metrics
        std::cout << "\n=== Dataflow Performance Metrics ===" << std::endl;
        std::cout << "Total Cycles: " << stats_.total_cycles << std::endl;
        std::cout << "Avg Parallelism: " << stats_.get_avg_parallelism() << std::endl;
        std::cout << "Max Parallelism: " << stats_.max_parallel_tasks << std::endl;
        std::cout << "Total Tokens: " << stats_.get_total_tokens() << std::endl;
        std::cout << "Max Matching Store Occupancy: " << stats_.max_matching_store_size << std::endl;
    }
}