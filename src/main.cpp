#include "mips.hpp"
#include <iostream>
#include <string>
int main(){
  std::string alo{};
  std::cout << "ola mundo" << "\n";

  std::array<uint32_t, 32> registers{0};
  std::array<uint32_t, 4096> program_data{0b00100010011010100000000000000100};

  uint32_t pc{0};
  IF_ID if_id{};
  ID_EX id_ex{};
  EX_MEM ex_mem{};
  MEM_WB mem_wb{};

  Memory memory{program_data};
  FetchStage fetch(pc, memory, if_id);
  DecodeStage decode(pc, registers, if_id, id_ex);
  ExecuteStage execute(id_ex, ex_mem);
  MemoryAccessStage memory_access(pc, memory, ex_mem, mem_wb);
  WriteBackStage write_back(registers, mem_wb);

  Mips mips(memory, fetch, decode, execute, memory_access, write_back);
  mips.run();
}