#include "write_back_stage.hpp"

WriteBackStage::WriteBackStage(std::array<uint32_t, 32>&registers, MEM_WB& mem_wb)
: registers_{registers}
, mem_wb_{mem_wb}
{
}


void WriteBackStage::run()
{
  if(!mem_wb_.control.reg_write)
  {
    return;
  }

  int32_t data_to_write;
  if(mem_wb_.control.mem_to_reg)
  { //load
    data_to_write = mem_wb_.mem_read_data;
  }else
  {
    data_to_write = mem_wb_.alu_result;
  }
  
  if(mem_wb_.write_register_addr != 0)
  {
    registers_[mem_wb_.write_register_addr] = data_to_write;
  }
}