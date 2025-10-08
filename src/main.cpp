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
  std::cout << "registers before: " << "\n";
  for(size_t i=0; i<32; ++i)
  {
    std::cout << "registers[" << std::to_string(i)<< "]: " << registers[i] << "\n";
  }
  Mips mips();
}