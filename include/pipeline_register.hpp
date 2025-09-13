#ifndef PIPELINE_REGISTER_H
#define PIPELINE_REGISTER_H

#include "alu_operation.hpp"
#include <cstdint>

struct ControlSignals
{
  // EX Controls
  bool   reg_dst;
  bool   alu_src;
  AluOperation alu_op;

  // MEM Controls
  bool    branch;
  bool  mem_read;
  bool mem_write;

  // WB Controls
  bool  reg_write;
  bool mem_to_reg;
};

struct IF_ID{
  uint32_t instruction;
  uint32_t     next_pc;
};

struct ID_EX{
  ControlSignals control;
  uint32_t     next_pc;
  uint32_t read_data_1;
  uint32_t read_data_2;
  int32_t sign_extended_immediate;
  uint8_t rt;
  uint8_t rd;
};

struct EX_MEM{
  ControlSignals control;  
  uint32_t     alu_result;
  uint32_t read_data_2;
  uint8_t write_register_addr;
  bool zero_flag;
};

struct MEM_WB{
  ControlSignals control;
  uint32_t    alu_result;
  uint32_t mem_read_data;
  uint8_t write_register_addr;
};

#endif
