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
    MipsConfig config{};
    config.num_alus = 2;
    config.num_mem_units = 1;
    config.alu_latency = 1;
    config.mem_latency = 3;
    
    Mips mips(program, config);
    
    // Run simulation
    mips.run(100);
    
    std::cout << "Simulation completed in " << mips.get_cycle_count() << " cycles\n";
    std::cout << "Final PC: " << mips.get_pc() << "\n";
    
    std::cout << "\nRegisters after execution:" << "\n";
    const auto& registers = mips.get_registers();
    for (size_t i = 0; i < 32; ++i) {
        if (registers[i] != 0) {
            std::cout << "  $" << i << ": " << registers[i] << "\n";
        }
    }
    
    // Expected output:
    // $8 (t0) = 300
    // $9 (t1) = 100
    // $10 (t2) = 200
    
    return 0;
}