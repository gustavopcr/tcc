#ifndef WRITE_BACK_STAGE_H
#define WRITE_BACK_STAGE_H

#include "pipeline_register.hpp"
#include <array>

class WriteBackStage{
public:
  explicit WriteBackStage(std::array<uint32_t, 32>&registers, MEM_WB& mem_wb);
  void run();
private:
  std::array<uint32_t, 32>& registers_;
  MEM_WB& mem_wb_;
};

#endif