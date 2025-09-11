#ifndef MIPS_H
#define MIPS_H
#include "register.hpp"
#include "pipeline_register.hpp"
#include "fetch_stage.hpp"
#include "decode_stage.hpp"
#include "execute_stage.hpp"
#include "memory_access_stage.hpp"
#include "write_back_stage.hpp"
#include <cstdint>
#include <array>

class Mips{
private:
  std::array<uint32_t, 32> registers_;
  uint32_t pc_;

  FetchStage fetch_;
  DecodeStage decode_;
  ExecuteStage execute_;
  MemoryAccessStage memory_access_;
  WriteBackStage write_back_;

  IF_ID   if_id_;
  ID_EX   id_ex_;
  EX_MEM ex_mem_;
  MEM_WB mem_wb_;
};

#endif
