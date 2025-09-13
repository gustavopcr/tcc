#include "execute_stage.hpp"

ExecuteStage::ExecuteStage(ID_EX& id_ex, EX_MEM& ex_mem)
: id_ex_{id_ex}
, ex_mem_{ex_mem}
{
}


void ExecuteStage::run()
{
  const AluOperation alu_op = id_ex_.control.alu_op;
  int32_t alu_input_2;
  if(id_ex_.control.alu_src)
  {
    alu_input_2 = id_ex_.sign_extended_immediate;
  }else
  {
    alu_input_2 = id_ex_.read_data_2;
  }
  ex_mem_.alu_result = alu(id_ex_.control.alu_op, id_ex_.read_data_1, alu_input_2);
  ex_mem_.read_data_2 = id_ex_.read_data_2; // Needed for 'sw' in MEM stage
  ex_mem_.zero_flag = (ex_mem_.alu_result == 0); // For 'beq' in MEM stage
  if (id_ex_.control.reg_dst) { // True for R-type
    ex_mem_.write_register_addr = id_ex_.rd;
  } else { // False for I-type (like lw or addi)
    ex_mem_.write_register_addr = id_ex_.rt;
  }

  // Pass the control signals to the next stage
  ex_mem_.control = id_ex_.control;
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