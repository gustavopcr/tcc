#include "von_neumann/mips.hpp"

#include <iostream>
#include <string>

int main()
{
  std::cout << "MIPS Superscalar Out-of-Order Simulator" << "\n";

  /*
    0b00100000000010010000000001100100 - addi $t1, $zero, 100
    0b00100000000010100000000011001000 - addi $t2, $zero, 200
    0b00000001001010100100000000100000 - add $t0, $t1, $t2
  */
  std::array<uint32_t, 4096> program{};
  program[0] = 0b00100000000010010000000001100100;  // addi $t1, $zero, 100
  program[1] = 0b00100000000010100000000011001000;  // addi $t2, $zero, 200
  program[2] = 0b00000001001010100100000000100000;  // add $t0, $t1, $t2

  std::cout << "Registers before execution:" << "\n";
  
  // Create MIPS simulator with program
  // In main.cpp
  MipsConfig config{};
  config.data_priority = true;  // Data access blocks fetch

  Mips mips(program, config);
  mips.run(1000);

  const auto& stats = mips.get_stats();
  std::cout << "Fetch stalls due to data access: " << stats.fetch_stalls << "\n";
  std::cout << "Total memory accesses: " << stats.memory_accesses << "\n";
  std::cout << "IPC: " << stats.get_ipc() << "\n";
    std::cout << "Memory Contention Ratio: " 
          << (stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    return 0;
}