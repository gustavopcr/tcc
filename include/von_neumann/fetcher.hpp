#ifndef FETCHER_HPP
#define FETCHER_HPP

#include "von_neumann/memory.hpp"
#include "von_neumann/instruction.hpp"
#include <cstdint>

class Fetcher{
public:
  explicit Fetcher(uint32_t& pc, Memory& memory, FetchDecodeQueue& output_queue);
  void tick();

private:
  uint32_t&   pc_;
  Memory& memory_;
  FetchDecodeQueue& output_queue_;
};
#endif