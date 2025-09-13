#ifndef EXECUTE_STAGE_H
#define EXECUTE_STAGE_H

#include "pipeline_register.hpp"
#include "alu_operation.hpp"

enum InstructionType : uint8_t
{
  R_TYPE,
  J_TYPE,
  I_TYPE
};

class ExecuteStage{
public:
  explicit ExecuteStage(ID_EX& id_ex, EX_MEM& ex_mem);
  void run();

private:
  ID_EX& id_ex_;
  EX_MEM& ex_mem_;
};

uint32_t alu(AluOperation op, uint32_t rs, uint32_t rt);

#endif