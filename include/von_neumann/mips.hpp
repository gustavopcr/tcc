#ifndef MIPS_HPP
#define MIPS_HPP

#include "von_neumann/register.hpp"
#include <cstdint>
#include <array>

class Mips{
public:
  explicit Mips();
  
  void tick();
private:
  void clock_tick();
  
  std::array<uint32_t, 32> registers_;
  uint32_t pc_;
};

#endif
