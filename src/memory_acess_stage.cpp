#include "memory_access_stage.hpp"

MemoryAccessStage::MemoryAccessStage(uint32_t& pc, Memory& memory, EX_MEM& ex_mem, MEM_WB& mem_wb)
: pc_{pc}
, memory_{memory}
, ex_mem_{ex_mem}
, mem_wb_{mem_wb}
{
}

void MemoryAccessStage::run()
{
  mem_wb_ = {};
  mem_wb_.alu_result = ex_mem_.alu_result;
  mem_wb_.write_register_addr = ex_mem_.write_register_addr;
  mem_wb_.control = ex_mem_.control;
  if(ex_mem_.control.mem_read)
  {
    mem_wb_.mem_read_data = memory_.read_word(ex_mem_.alu_result);
  }else if(ex_mem_.control.mem_write)
  {
    memory_.store_word(ex_mem_.alu_result, ex_mem_.read_data_2);
  }else if(ex_mem_.control.branch && ex_mem_.zero_flag)
  {
    pc_ = ex_mem_.alu_result;
  }
}
