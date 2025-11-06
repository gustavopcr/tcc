#ifndef NODES_HPP
#define NODES_HPP

// FORK -> DISTRIBUTES ONE DATA TO MANY NODES
// PRIMITIVE OPS -> PERFORMS BASIC OPERATIONS
// SWITCH -> ROUTES DATA BASED ON CONDITIONS
// MERGE  -> MERGES VALUE BASED ON BOOLEAN INPUT

#include <cstdint>
#include <cstddef>

struct Node
{
  size_t tag; // id for the node
  uint64_t result;
};
#endif