#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "dataflow/primitives/node.hpp"
struct Token
{
  uint64_t fp; // fp -> frame pointer -> used for function calls and loop management
  uint64_t ip; // ip -> instruction pointer
  uint8_t p; // p -> port(L or R) or indexes
  uint64_t v; // v -> value
};
#endif