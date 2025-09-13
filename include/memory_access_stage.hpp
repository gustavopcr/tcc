#ifndef MEMORY_ACCESS_STAGE_H
#define MEMORY_ACCESS_STAGE_H

#include "pipeline_register.hpp"
#include "memory.hpp"

class MemoryAccessStage{
public:
  explicit MemoryAccessStage(uint32_t& pc, Memory& memory, EX_MEM& ex_mem, MEM_WB& mem_wb);
  void run();
private:
  uint32_t& pc_;
  Memory& memory_;
  EX_MEM& ex_mem_;
  MEM_WB& mem_wb_;
};
#endif