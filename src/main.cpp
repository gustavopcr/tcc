#include "mips.hpp"

#include <iostream>
#include <string>
int main(){
  std::string alo{};
  std::cout << "ola mundo" << "\n";

  std::array<uint32_t, 32> registers{0};
  // registers[static_cast<size_t>(Register::t1)] = 100;
  // registers[static_cast<size_t>(Register::t2)] = 200;
  /*
    0b00100000000010010000000001100100 - addi $t1, $zero, 100
    0b00100000000010100000000011001000 - addi $t2, $zero, 200
    0b00000001001010100100000000100000 - add $t0, $t1, $t2
  */
  std::array<uint32_t, 4096> program_data{0b00100000000010010000000001100100, 0b00100000000010100000000011001000, 0b00000001001010100100000000100000};

  uint32_t pc{0};
  IF_ID if_id{};
  ID_EX id_ex{};
  EX_MEM ex_mem{};
  MEM_WB mem_wb{};

  Memory memory{program_data};
  FetchStage fetch(pc, memory, if_id);
  DecodeStage decode(pc, registers, if_id, id_ex);
  ExecuteStage execute(id_ex, ex_mem, mem_wb);
  MemoryAccessStage memory_access(pc, memory, ex_mem, mem_wb);
  WriteBackStage write_back(registers, mem_wb);

  std::cout << "registers before: " << "\n";
  for(size_t i=0; i<32; ++i)
  {
    std::cout << "registers[" << std::to_string(i)<< "]: " << registers[i] << "\n";
  }
  Mips mips(memory, fetch, decode, execute, memory_access, write_back);
  mips.run();

  std::cout << "registers after: " << "\n";
  for(size_t i=0; i<32; ++i)
  {
    std::cout << "registers[" << std::to_string(i)<< "]: " << registers[i] << "\n";
  }
}