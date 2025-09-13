#include "memory.hpp"


Memory::Memory(std::array<uint32_t, 4096> program_data)
: memory_{program_data}
{
}

uint32_t Memory::read_word(uint32_t address) const
{
  if((address % 4) != 0)
  {
    throw std::runtime_error("Unaligned memory read at address: " + std::to_string(address));
  }

  uint32_t index = address/4;

  if(index >= memory_.size())
  {
    throw std::runtime_error("Out-of-bounds memory read at address: " + std::to_string(address));
  }

  return memory_[index];
}

void Memory::store_word(uint32_t address, uint32_t data)
{
  if((address % 4) != 0)
  {
    throw std::runtime_error("Unaligned memory read at address: " + std::to_string(address));
  }

  uint32_t index = address/4;

  if(index >= memory_.size())
  {
    throw std::runtime_error("Out-of-bounds memory read at address: " + std::to_string(address));
  }

  memory_[index] = data;
}
