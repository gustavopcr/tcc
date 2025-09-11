#ifndef FETCH_STAGE_H
#define FETCH_STAGE_H

#include "pipeline_register.hpp"
#include "memory.hpp"
#include <cstdint>

class FetchStage{
public:
  FetchStage(uint32_t& pc, Memory& memory, IF_ID& if_id);
  void run();

private:
  uint32_t&   pc_;
  Memory& memory_;
  IF_ID&   if_id_;
};
#endif