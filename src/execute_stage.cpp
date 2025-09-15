#include "execute_stage.hpp"

ExecuteStage::ExecuteStage(ID_EX& id_ex, EX_MEM& ex_mem, MEM_WB& mem_wb)
: id_ex_{id_ex}
, ex_mem_{ex_mem}
, mem_wb_{mem_wb}
{
}


void ExecuteStage::run()
{
  EX_MEM next_ex_mem = {};
  const AluOperation alu_op = id_ex_.control.alu_op;
  uint32_t data_from_rt = get_data_for_rt();
  int32_t alu_input_2;
  if(id_ex_.control.alu_src)
  {
    alu_input_2 = id_ex_.sign_extended_immediate;
  }else
  {
    alu_input_2 = data_from_rt;
  }
  next_ex_mem.alu_result = alu(id_ex_.control.alu_op, get_data_for_rs(), alu_input_2);
  next_ex_mem.read_data_2 = data_from_rt; // Needed for 'sw' in MEM stage
  next_ex_mem.zero_flag = (next_ex_mem.alu_result == 0); // For 'beq' in MEM stage
  if (id_ex_.control.reg_dst) { // True for R-type
    next_ex_mem.write_register_addr = id_ex_.rd;
  } else { // False for I-type (like lw or addi)
    next_ex_mem.write_register_addr = id_ex_.rt;
  }

  // Pass the control signals to the next stage
  next_ex_mem.control = id_ex_.control;
  ex_mem_ = next_ex_mem;
}


uint32_t alu(AluOperation op, uint32_t rs, uint32_t rt)
{
  switch (op)
  {
    case AluOperation::ADD:
      return rs + rt;

    case AluOperation::SUB:
      return rs - rt;

    case AluOperation::AND:
      return rs & rt;

    case AluOperation::OR:
      return rs | rt;

    case AluOperation::SLT:
      return (static_cast<int32_t>(rs) < static_cast<int32_t>(rt)) ? 1 : 0;
    
    case AluOperation::NOR:
      return ~(rs | rt);
    
    default:
      return 0; 
  }
}


uint32_t ExecuteStage::get_data_for_rs() {
  // MEM->EX hazard check
  if (ex_mem_.control.reg_write && ex_mem_.write_register_addr != 0 && ex_mem_.write_register_addr == id_ex_.rs) {
    return ex_mem_.alu_result;
  }
  // WB->EX hazard check
  if (mem_wb_.control.reg_write && mem_wb_.write_register_addr != 0 && mem_wb_.write_register_addr == id_ex_.rs) {
    return mem_wb_.control.mem_to_reg ? mem_wb_.mem_read_data : mem_wb_.alu_result;
  }
  return id_ex_.read_data_1;
}

uint32_t ExecuteStage::get_data_for_rt() {
  // MEM->EX hazard check
  if (ex_mem_.control.reg_write && ex_mem_.write_register_addr != 0 && ex_mem_.write_register_addr == id_ex_.rt) {
    return ex_mem_.alu_result;
  }
  // WB->EX hazard check
  if (mem_wb_.control.reg_write && mem_wb_.write_register_addr != 0 && mem_wb_.write_register_addr == id_ex_.rt) {
    return mem_wb_.control.mem_to_reg ? mem_wb_.mem_read_data : mem_wb_.alu_result;
  }
  return id_ex_.read_data_2;
}