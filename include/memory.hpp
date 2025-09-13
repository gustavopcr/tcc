#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <array>
#include <stdexcept>

class Memory{
public:
  uint32_t read_word(uint32_t address) const;
  void store_word(uint32_t address, uint32_t data);

private:
  std::array<uint32_t, 4096> memory_; 
};

#endif