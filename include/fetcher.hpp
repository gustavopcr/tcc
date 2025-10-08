#ifndef FETCHER
#define FETCHER

#include "memory.hpp"
#include <cstdint>

class Fetcher{
public:
  explicit Fetcher(uint32_t& pc, Memory& memory);
  void tick();

private:
  uint32_t   pc_;
  Memory& memory_;
};
#endif