#ifndef NODES_HPP
#define NODES_HPP

// FORK -> DISTRIBUTES ONE DATA TO MANY NODES
// PRIMITIVE OPS -> PERFORMS BASIC OPERATIONS
// SWITCH -> ROUTES DATA BASED ON CONDITIONS
// MERGE  -> MERGES VALUE BASED ON BOOLEAN INPUT

#include "operation.hpp"
#include <cstdint>
#include <cstddef>
#include <unordered_map>


struct Destination
{
  uint64_t ip;
  uint8_t port; // index 
};

struct Node
{
  size_t tag; // id for the node
  size_t num_inputs;
  Operation op;
  std::vector<Destination> destinations;
};

using NodeGraph = std::unordered_map<size_t, Node>; // maps instruction pointer to corresponding node
#endif