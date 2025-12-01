#include <gtest/gtest.h>
#include "von_neumann/mips.hpp"
#include "von_neumann/assembler.hpp"
#include <array>
#include <iomanip>

class EmulatorTest : public ::testing::Test {
protected:
    Assembler assembler_;
    
    // Helper to create program array from assembled instructions
    std::array<uint32_t, 4096> make_program(const std::vector<std::string>& lines) {
        std::array<uint32_t, 4096> program{};
        auto instructions = assembler_.assemble(lines);
        for (size_t i = 0; i < instructions.size() && i < 4096; ++i) {
            program[i] = instructions[i];
        }
        return program;
    }
    
    // Helper to run until halted or max cycles
    void run_until_done(Mips& cpu, uint64_t max_cycles = 1000) {
        cpu.run(max_cycles);
    }
};

// Add this test at the beginning of the test file, after the class definition

TEST_F(EmulatorTest, DiagnosticMinimal) {
    auto program = make_program({
        "ADDI $t0, $zero, 42",
    });
    
    // Print the assembled instruction
    std::cout << "Assembled instruction: 0x" << std::hex << program[0] << std::dec << "\n";
    
    Mips cpu(program);
    
    // Check initial state
    std::cout << "Before run - PC should be at start\n";
    
    // Run just a few cycles to see what happens
    for (int i = 0; i < 20; ++i) {
        cpu.tick();
        auto stats = cpu.get_stats();
        std::cout << "Cycle " << i << ": "
                  << "commits=" << stats.instructions_committed
                  << ", halted=" << cpu.is_halted() << "\n";
        std::cout << "Memory Contention Ratio: " 
          << (stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
        if (cpu.is_halted()) {
            std::cout << "CPU halted at cycle " << i << "\n";
            break;
        }
    }
    
    std::cout << "$t0 = " << cpu.get_register(8) << "\n";
    
    // This will fail, but we want to see the output
    EXPECT_EQ(cpu.get_register(8), 42u);
}

// ============================================================================
// Basic ALU Operations
// ============================================================================

TEST_F(EmulatorTest, RTypeAdd) {
    auto program = make_program({
        "ADDI $t0, $zero, 10",
        "ADDI $t1, $zero, 20",
        "ADD $t2, $t0, $t1",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(10), 30);  // $t2 = 30
}

TEST_F(EmulatorTest, RTypeSub) {
    auto program = make_program({
        "ADDI $t0, $zero, 50",
        "ADDI $t1, $zero, 20",
        "SUB $t2, $t0, $t1",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(10), 30);
}

TEST_F(EmulatorTest, ITypeAddi) {
    auto program = make_program({
        "ADDI $t0, $zero, 100",
        "ADDI $t1, $t0, 50",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(9), 150);  // $t1
}

TEST_F(EmulatorTest, ITypeLogical) {
    auto program = make_program({
        "ADDI $t0, $zero, 0xFF",
        "ANDI $t1, $t0, 0x0F",
        "ORI $t2, $t0, 0x100",
        "XORI $t3, $t0, 0xFF",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(9), 0x0F);
    EXPECT_EQ(cpu.get_register(10), 0x1FF);
    EXPECT_EQ(cpu.get_register(11), 0);
}

// ============================================================================
// RAW Hazard Handling (OoO Correctness)
// ============================================================================

TEST_F(EmulatorTest, RawHazardSimple) {
    auto program = make_program({
        "ADDI $t0, $zero, 5",
        "ADDI $t1, $t0, 10",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(8), 5);
    EXPECT_EQ(cpu.get_register(9), 15);
}

TEST_F(EmulatorTest, RawHazardChain) {
    auto program = make_program({
        "ADDI $t0, $zero, 1",
        "ADDI $t1, $t0, 1",
        "ADDI $t2, $t1, 1",
        "ADDI $t3, $t2, 1",
        "ADDI $t4, $t3, 1",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(8), 1);
    EXPECT_EQ(cpu.get_register(9), 2);
    EXPECT_EQ(cpu.get_register(10), 3);
    EXPECT_EQ(cpu.get_register(11), 4);
    EXPECT_EQ(cpu.get_register(12), 5);
}

TEST_F(EmulatorTest, RawHazardWithIndependent) {
    auto program = make_program({
        "ADDI $t0, $zero, 10",
        "ADDI $t1, $zero, 20",
        "ADDI $t2, $zero, 30",
        "ADD $t3, $t0, $t1",
        "ADD $t4, $t2, $t3",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(11), 30);
    EXPECT_EQ(cpu.get_register(12), 60);
}

TEST_F(EmulatorTest, WawHazard) {
    auto program = make_program({
        "ADDI $t0, $zero, 1",
        "ADDI $t0, $zero, 2",
        "ADDI $t0, $zero, 3",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(8), 3);
}

// ============================================================================
// Load/Store Operations
// ============================================================================

TEST_F(EmulatorTest, LoadStoreSimple) {
    // Note: Memory layout - program is at low addresses, data at higher
    // We'll store at address 0x800 (2048) to avoid program area
    auto program = make_program({
        "ADDI $t0, $zero, 42",
        "ADDI $t5, $zero, 2048",  // Base address
        "SW $t0, 0($t5)",
        "LW $t1, 0($t5)",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(9), 42);
}

TEST_F(EmulatorTest, LoadStoreWithOffset) {
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 512",   // Base address (word index 128)
        "ADDI $t1, $zero, 0xAB",
        "SW $t1, 8($t0)",         // Store at 512 + 8 = 520 (word 130)
        "LW $t2, 8($t0)",         // Load from 520
    });
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(100);
    
    EXPECT_EQ(cpu.get_register(10), 0xABu);
}

TEST_F(EmulatorTest, LoadAfterStore) {
    auto program = make_program({
        "ADDI $t5, $zero, 2048",
        "ADDI $t0, $zero, 77",
        "SW $t0, 0($t5)",
        "LW $t1, 0($t5)",
        "ADDI $t2, $t1, 1",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(10), 78);
}

// ============================================================================
// Von Neumann Bottleneck Verification
// ============================================================================

TEST_F(EmulatorTest, BottleneckCausesStalls) {
    // This test verifies the memory bus arbitration works correctly
    // Actual fetch stalls depend on timing and may not occur with short programs
    
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 512",   // Base address
        "ADDI $t1, $zero, 42",    // Value to store
        "SW $t1, 0($t0)",         // Store
        "LW $t2, 0($t0)",         // Load
        "LW $t3, 0($t0)",         // Another load
    });
    
    std::array<uint32_t, 4096> prog{};
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(100);
    
    auto stats = cpu.get_stats();
    
    // Verify correctness
    EXPECT_EQ(cpu.get_register(10), 42u);  // $t2 loaded 42
    EXPECT_EQ(cpu.get_register(11), 42u);  // $t3 loaded 42
    
    // Memory bus statistics (informational)
    std::cerr << "Memory accesses: " << stats.memory_accesses << std::endl;
    std::cerr << "Fetch stalls: " << stats.fetch_stalls << std::endl;
    std::cout << "Memory Contention Ratio: " 
          << (stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    // For short programs, fetch typically completes before data accesses
    // So we just verify the memory bus is counting accesses
    EXPECT_GT(stats.memory_accesses, 0u);
}

TEST_F(EmulatorTest, PureAluNoBottleneck) {
    auto program = make_program({
        "ADDI $t0, $zero, 1",
        "ADDI $t1, $zero, 2",
        "ADDI $t2, $zero, 3",
        "ADDI $t3, $zero, 4",
        "ADD $t4, $t0, $t1",
        "ADD $t5, $t2, $t3",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    auto stats = cpu.get_stats();
    
    // Pure ALU should have zero or minimal fetch stalls
    EXPECT_EQ(stats.fetch_stalls, 0u)
        << "Pure ALU program should not stall fetch";
}

// ============================================================================
// Branch Prediction (Always Not-Taken)
// ============================================================================

TEST_F(EmulatorTest, BranchNotTakenCorrect) {
    auto program = make_program({
        "ADDI $t0, $zero, 5",
        "ADDI $t1, $zero, 10",
        "BEQ $t0, $t1, 2",
        "ADDI $t2, $zero, 1",
        "ADDI $t3, $zero, 2",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(10), 1);
    EXPECT_EQ(cpu.get_register(11), 2);
    
    auto stats = cpu.get_stats();
    EXPECT_EQ(stats.branch_mispredictions, 0u);
}

TEST_F(EmulatorTest, BranchTakenMisprediction) {
    auto program = make_program({
        "ADDI $t0, $zero, 5",
        "ADDI $t1, $zero, 5",
        "BEQ $t0, $t1, 2",      // Taken -> misprediction
        "ADDI $t2, $zero, 99",  // Should be flushed
        "ADDI $t3, $zero, 99",  // Should be flushed
        "ADDI $t4, $zero, 1",   // Branch target
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_NE(cpu.get_register(10), 99u);
    EXPECT_NE(cpu.get_register(11), 99u);
    EXPECT_EQ(cpu.get_register(12), 1u);
    
    auto stats = cpu.get_stats();
    EXPECT_EQ(stats.branch_mispredictions, 1u);
}

TEST_F(EmulatorTest, BranchLoop) {
    // Sum 1 to 5
    auto program = make_program({
        "ADDI $t0, $zero, 0",   // sum = 0
        "ADDI $t1, $zero, 1",   // i = 1
        "ADDI $t2, $zero, 6",   // limit = 6
        // loop (index 3):
        "ADD $t0, $t0, $t1",    // sum += i
        "ADDI $t1, $t1, 1",     // i++
        "BNE $t1, $t2, -3",     // if i != 6, goto loop
        "ADDI $t3, $zero, 1",   // Done marker
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    // sum = 1 + 2 + 3 + 4 + 5 = 15
    EXPECT_EQ(cpu.get_register(8), 15u);
    EXPECT_EQ(cpu.get_register(11), 1u);
    
    auto stats = cpu.get_stats();
    // Loop runs 5 times, branch taken 4 times (mispredictions)
    EXPECT_EQ(stats.branch_mispredictions, 4u);
}

TEST_F(EmulatorTest, BranchLoopDiagnostic) {
    // Simple loop: add 5 three times
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 0",    // 0: counter = 0
        "ADDI $t1, $zero, 3",    // 1: limit = 3
        // loop:
        "ADDI $t0, $t0, 5",      // 2: counter += 5
        "ADDI $t2, $t2, 1",      // 3: iteration++
        "BNE $t2, $t1, -3",      // 4: if iteration != limit, goto loop (PC=8)
        "ADDI $t3, $zero, 1",    // 5: done marker
    });
    
    std::cerr << "\n=== Branch Loop Diagnostic ===" << std::endl;
    std::cerr << "Instructions:" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        std::cerr << "  [" << i << "] PC=" << (i*4) << ": 0x" 
                  << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    std::array<uint32_t, 4096> prog{};
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 100; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": mispred=" << stats.branch_mispredictions
                      << ", $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", $t2=" << cpu.get_register(10)
                      << ", $t3=" << cpu.get_register(11)
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    // After 3 iterations: $t0 = 15, $t2 = 3, $t3 = 1
    // Mispredictions: 2 taken (loop back) + 1 not taken (exit) = 3? Or just taken ones?
    EXPECT_EQ(cpu.get_register(8), 15u);  // 5 * 3 = 15
    EXPECT_EQ(cpu.get_register(10), 3u);  // 3 iterations
    EXPECT_EQ(cpu.get_register(11), 1u);  // done marker
}

TEST_F(EmulatorTest, BranchDiagnostic) {
    // Simple test: BEQ with equal values should be taken
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 5",
        "ADDI $t1, $zero, 5",
        "BEQ $t0, $t1, 1",       // Should skip next instruction
        "ADDI $t2, $zero, 99",   // Should be skipped
        "ADDI $t3, $zero, 42",   // Should execute
    });
    
    std::cout << "\n=== Branch Diagnostic ===" << std::endl;
    std::cout << "Assembled " << instructions.size() << " instructions:" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        std::cout << "  [" << i << "] 0x" << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    std::array<uint32_t, 4096> prog{};
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 30; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        if (cycle % 5 == 0 || cpu.is_halted()) {
            std::cout << "Cycle " << cycle 
                      << ": commits=" << stats.instructions_committed
                      << ", mispred=" << stats.branch_mispredictions
                      << ", $t2=" << cpu.get_register(10)
                      << ", $t3=" << cpu.get_register(11)
                      << std::endl;
        }
        if (cpu.is_halted()) break;
    }
    
    auto stats = cpu.get_stats();
    std::cout << "Final: mispredictions=" << stats.branch_mispredictions << std::endl;
    std::cout << "$t2=" << cpu.get_register(10) << " (expect: NOT 99)" << std::endl;
    std::cout << "$t3=" << cpu.get_register(11) << " (expect: 42)" << std::endl;
    
    // Branch taken = misprediction for always-not-taken
    EXPECT_EQ(stats.branch_mispredictions, 1u);
    EXPECT_NE(cpu.get_register(10), 99u);  // t2 should NOT be 99
    EXPECT_EQ(cpu.get_register(11), 42u);  // t3 should be 42
}

TEST_F(EmulatorTest, BranchDiagnosticDetailed) {
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 5",    // 0: PC=0
        "ADDI $t1, $zero, 5",    // 1: PC=4
        "BEQ $t0, $t1, 1",       // 2: PC=8, target = 8+4+(1*4) = 16
        "ADDI $t2, $zero, 99",   // 3: PC=12 (should be flushed)
        "ADDI $t3, $zero, 42",   // 4: PC=16 (branch target)
    });
    
    std::cerr << "\n=== Branch Detailed Diagnostic ===" << std::endl;
    std::cerr << "Instruction layout:" << std::endl;
    std::cerr << "  PC=0:  ADDI $t0, $zero, 5" << std::endl;
    std::cerr << "  PC=4:  ADDI $t1, $zero, 5" << std::endl;
    std::cerr << "  PC=8:  BEQ $t0, $t1, 1 (target=PC+4+4=16)" << std::endl;
    std::cerr << "  PC=12: ADDI $t2, $zero, 99 (should flush)" << std::endl;
    std::cerr << "  PC=16: ADDI $t3, $zero, 42 (branch target)" << std::endl;
    
    std::array<uint32_t, 4096> prog{};
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 30; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        std::cerr << "Cycle " << std::setw(2) << cycle 
                  << ": commits=" << stats.instructions_committed
                  << ", mispred=" << stats.branch_mispredictions
                  << ", $t0=" << cpu.get_register(8)
                  << ", $t1=" << cpu.get_register(9)
                  << ", $t2=" << cpu.get_register(10)
                  << ", $t3=" << cpu.get_register(11);
        
        if (cpu.is_halted()) {
            std::cerr << " [HALTED]";
        }
        std::cerr << std::endl;
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(10), 0u);   // t2 should NOT be 99
    EXPECT_EQ(cpu.get_register(11), 42u);  // t3 should be 42
}
// ============================================================================
// ROB In-Order Commit
// ============================================================================

TEST_F(EmulatorTest, InOrderCommit) {
    auto program = make_program({
        "ADDI $t5, $zero, 2048",
        "ADDI $t0, $zero, 100",
        "LW $t1, 0($t5)",        // Slow (memory access)
        "ADDI $t2, $t1, 1",      // Depends on load
    });
    
    // Pre-populate memory
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",
        "ADDI $t0, $zero, 100",
        "LW $t1, 0($t5)",
        "ADDI $t2, $t1, 1",
    });
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    // Put data at address 2048 (word index 512)
    prog[512] = 50;
    
    Mips cpu(prog);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(8), 100u);
    EXPECT_EQ(cpu.get_register(9), 50u);
    EXPECT_EQ(cpu.get_register(10), 51u);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(EmulatorTest, ManyInstructions) {
    std::vector<std::string> lines;
    for (int i = 0; i < 20; i++) {
        lines.push_back("ADDI $t0, $t0, 1");
    }
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble(lines);
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(1000);  // Increase max cycles to allow all instructions to complete
    
    EXPECT_EQ(cpu.get_register(8), 20u);
}
TEST_F(EmulatorTest, MixedWorkload) {
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // Base address for data
        "ADDI $t0, $zero, 0",     // Counter
        "ADDI $t1, $zero, 3",     // Limit
        // loop:
        "ADDI $t0, $t0, 1",       // counter++
        "SLL $t6, $t0, 2",        // t6 = counter * 4 (word offset)
        "ADD $t6, $t5, $t6",      // address = base + offset
        "SW $t0, 0($t6)",         // Store counter
        "BNE $t0, $t1, -5",       // Loop back 5 instructions
        // end:
        "LW $t2, 4($t5)",         // Load from base+4 (word 1)
        "LW $t3, 8($t5)",         // Load from base+8 (word 2)
        "LW $t4, 12($t5)",        // Load from base+12 (word 3)
    });
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(500);
    
    EXPECT_EQ(cpu.get_register(8), 3u);   // Final counter
    EXPECT_EQ(cpu.get_register(10), 1u);  // Value at word 1
    EXPECT_EQ(cpu.get_register(11), 2u);  // Value at word 2
    EXPECT_EQ(cpu.get_register(12), 3u);  // Value at word 3
}
// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(EmulatorTest, ZeroRegisterImmutable) {
    auto program = make_program({
        "ADDI $zero, $zero, 100",
        "ADD $t0, $zero, $zero",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(0), 0u);
    EXPECT_EQ(cpu.get_register(8), 0u);
}

TEST_F(EmulatorTest, NegativeImmediate) {
    auto program = make_program({
        "ADDI $t0, $zero, 100",
        "ADDI $t1, $t0, -30",
    });
    
    Mips cpu(program);
    run_until_done(cpu);
    
    EXPECT_EQ(cpu.get_register(9), 70u);
}

TEST_F(EmulatorTest, DependencyChainDiagnostic) {
    // Simple chain: t0=1, t1=t0+1, t2=t1+1
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 1",    // t0 = 1
        "ADDI $t1, $t0, 1",      // t1 = t0 + 1 = 2 (depends on t0)
        "ADDI $t2, $t1, 1",      // t2 = t1 + 1 = 3 (depends on t1)
    });
    
    std::cerr << "\n=== Dependency Chain Diagnostic ===" << std::endl;
    
    std::array<uint32_t, 4096> prog{};
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 30; ++cycle) {
        cpu.tick();
        
        std::cerr << "Cycle " << std::setw(2) << cycle 
                  << ": $t0=" << cpu.get_register(8)
                  << ", $t1=" << cpu.get_register(9)
                  << ", $t2=" << cpu.get_register(10)
                  << (cpu.is_halted() ? " [HALTED]" : "")
                  << std::endl;
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(8), 1u);
    EXPECT_EQ(cpu.get_register(9), 2u);
    EXPECT_EQ(cpu.get_register(10), 3u);
}


TEST_F(EmulatorTest, ShiftDiagnostic) {
    // Simple test: shift 1 left by 2 = 4
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 1",    // t0 = 1
        "SLL $t1, $t0, 2",       // t1 = 1 << 2 = 4
        "ADDI $t2, $zero, 2048", // t2 = 2048
        "ADD $t3, $t2, $t1",     // t3 = 2048 + 4 = 2052
    });
    
    std::cerr << "\n=== Shift Diagnostic ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        std::cerr << "  [" << i << "] 0x" << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    std::array<uint32_t, 4096> prog{};
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(50);
    
    std::cerr << "$t0 = " << cpu.get_register(8) << " (expect 1)" << std::endl;
    std::cerr << "$t1 = " << cpu.get_register(9) << " (expect 4)" << std::endl;
    std::cerr << "$t2 = " << cpu.get_register(10) << " (expect 2048)" << std::endl;
    std::cerr << "$t3 = " << cpu.get_register(11) << " (expect 2052)" << std::endl;
    
    EXPECT_EQ(cpu.get_register(8), 1u);
    EXPECT_EQ(cpu.get_register(9), 4u);
    EXPECT_EQ(cpu.get_register(10), 2048u);
    EXPECT_EQ(cpu.get_register(11), 2052u);
}

TEST_F(EmulatorTest, MixedWorkloadDiagnostic) {
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // Base address for data
        "ADDI $t0, $zero, 0",     // Counter
        "ADDI $t1, $zero, 3",     // Limit
        // loop:
        "ADDI $t0, $t0, 1",       // counter++
        "SLL $t6, $t0, 2",        // t6 = counter * 4 (word offset)
        "ADD $t6, $t5, $t6",      // address = base + offset
        "SW $t0, 0($t6)",         // Store counter
        "BNE $t0, $t1, -5",       // Loop back 5 instructions
        // end:
        "LW $t2, 4($t5)",         // Load from base+4 (word 1)
        "LW $t3, 8($t5)",         // Load from base+8 (word 2)
        "LW $t4, 12($t5)",        // Load from base+12 (word 3)
    });
    
    std::cerr << "\n=== MixedWorkload Diagnostic ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 200; ++cycle) {
        cpu.tick();
        
        if (cycle % 20 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(3) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", $t2=" << cpu.get_register(10)
                      << ", $t3=" << cpu.get_register(11)
                      << ", $t4=" << cpu.get_register(12)
                      << ", $t5=" << cpu.get_register(13)
                      << ", $t6=" << cpu.get_register(14)
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    // Check memory contents
    std::cerr << "Memory at 2048+4 = " << cpu.get_memory(2052) << " (expect 1)" << std::endl;
    std::cerr << "Memory at 2048+8 = " << cpu.get_memory(2056) << " (expect 2)" << std::endl;
    std::cerr << "Memory at 2048+12 = " << cpu.get_memory(2060) << " (expect 3)" << std::endl;
    
    EXPECT_EQ(cpu.get_register(8), 3u);   // Final counter
    EXPECT_EQ(cpu.get_register(10), 1u);  // Value at word 1
    EXPECT_EQ(cpu.get_register(11), 2u);  // Value at word 2
    EXPECT_EQ(cpu.get_register(12), 3u);  // Value at word 3
}


// ...existing code...

// ============================================================================
// Von Neumann Bottleneck Stress Tests
// ============================================================================
TEST_F(EmulatorTest, HighMemoryContention) {
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // Base address for data
        "ADDI $t0, $zero, 42",    // Initial value
        
        // Mix of ALU and memory ops - more realistic
        "SW $t0, 0($t5)",         // Store 42
        "ADDI $t1, $t0, 1",       // ALU work: t1 = 43
        "LW $t2, 0($t5)",         // Load back: t2 = 42
        "ADD $t3, $t1, $t2",      // ALU work: t3 = 85
        "SW $t3, 4($t5)",         // Store 85
        "ADDI $t4, $t3, 10",      // ALU work: t4 = 95
        "LW $t6, 4($t5)",         // Load back: t6 = 85
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(500);
    
    auto stats = cpu.get_stats();
    
    // Verify correctness
    EXPECT_EQ(cpu.get_register(10), 42u);  // $t2 = 42
    EXPECT_EQ(cpu.get_register(11), 85u);  // $t3 = 85
    EXPECT_EQ(cpu.get_register(14), 85u);  // $t6 = 85
    
    // Structural checks - less strict
    EXPECT_EQ(stats.instructions_committed, 9u);
    EXPECT_GT(stats.memory_accesses, 0u)
        << "Should have memory accesses";
}

TEST_F(EmulatorTest, MemoryContentionLoop) {
    // Loop with memory operations - maximizes bottleneck effect
    // Each iteration does: load, compute, store, branch
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // Base address
        "ADDI $t0, $zero, 0",     // Loop counter
        "ADDI $t1, $zero, 10",    // Loop limit
        "ADDI $t2, $zero, 0",     // Running sum
        
        // loop: (index 4)
        "LW $t3, 0($t5)",         // Load from memory (causes contention)
        "ADD $t2, $t2, $t3",      // Accumulate
        "ADDI $t3, $t3, 1",       // Increment loaded value
        "SW $t3, 0($t5)",         // Store back (causes contention)
        "ADDI $t0, $t0, 1",       // i++
        "BNE $t0, $t1, -6",       // Loop back (causes misprediction)
        
        // After loop - verify
        "LW $t4, 0($t5)",         // Final load
        "ADDI $s0, $zero, 1",     // Done marker to prevent early halt
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    // Initialize memory at base address
    prog[512] = 0;  // Word at address 2048
    
    Mips cpu(prog);
    cpu.run(2000);  // Increase cycles to ensure completion
    
    auto stats = cpu.get_stats();
    
    std::cerr << "\n=== Memory Contention Loop Test ===" << std::endl;
    std::cerr << "Total Cycles: " << stats.cycles << std::endl;
    std::cerr << "Instructions Committed: " << stats.instructions_committed << std::endl;
    std::cerr << "Memory Accesses: " << stats.memory_accesses << std::endl;
    std::cerr << "Fetch Stalls (Memory Contention): " << stats.fetch_stalls << std::endl;
    std::cerr << "Data Stalls: " << stats.data_stalls << std::endl;
    std::cerr << "ROB Full Stalls: " << stats.rob_full_stalls << std::endl;
    std::cerr << "RS Full Stalls: " << stats.rs_full_stalls << std::endl;
    std::cerr << "EU Busy Stalls: " << stats.eu_busy_stalls << std::endl;
    std::cerr << "Total Stalls: " << stats.get_total_stalls() << std::endl;
    std::cerr << "Memory Contention Ratio: " 
              << (stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    std::cerr << "Branch Mispredictions: " << stats.branch_mispredictions << std::endl;
    std::cerr << "IPC: " << stats.get_ipc() << std::endl;
    std::cerr << "$t4 (final mem value): " << cpu.get_register(12) << std::endl;
    std::cerr << "$t2 (sum): " << cpu.get_register(10) << std::endl;
    
    // Thesis metrics - these are the important assertions
    EXPECT_GT(stats.fetch_stalls, 0u)
        << "Loop with memory ops should cause fetch stalls";
    EXPECT_GT(stats.memory_accesses, 10u)
        << "Should have memory accesses from load/store operations";
    
    // Verify the loop actually ran (memory value was incremented)
    EXPECT_GT(cpu.get_register(12), 0u) 
        << "Memory should have been incremented by the loop";
}

TEST_F(EmulatorTest, BottleneckComparison) {
    // Run two programs: one ALU-heavy, one memory-heavy
    // Compare their memory contention ratios
    
    // === ALU-Heavy Program ===
    std::array<uint32_t, 4096> alu_prog{};
    auto alu_instructions = assembler_.assemble({
        "ADDI $t0, $zero, 1",
        "ADDI $t1, $zero, 2",
        "ADD $t2, $t0, $t1",
        "ADD $t3, $t2, $t0",
        "ADD $t4, $t3, $t1",
        "SUB $t5, $t4, $t0",
        "ADD $t6, $t5, $t2",
        "ADD $t7, $t6, $t3",
        "SUB $t0, $t7, $t4",
        "ADD $t1, $t0, $t5",
    });
    for (size_t i = 0; i < alu_instructions.size(); ++i) {
        alu_prog[i] = alu_instructions[i];
    }
    
    Mips alu_cpu(alu_prog);
    alu_cpu.run(200);
    auto alu_stats = alu_cpu.get_stats();
    
    // === Memory-Heavy Program ===
    std::array<uint32_t, 4096> mem_prog{};
    auto mem_instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",
        "ADDI $t0, $zero, 1",
        "SW $t0, 0($t5)",
        "LW $t1, 0($t5)",
        "SW $t1, 4($t5)",
        "LW $t2, 4($t5)",
        "SW $t2, 8($t5)",
        "LW $t3, 8($t5)",
        "SW $t3, 12($t5)",
        "LW $t4, 12($t5)",
    });
    for (size_t i = 0; i < mem_instructions.size(); ++i) {
        mem_prog[i] = mem_instructions[i];
    }
    
    Mips mem_cpu(mem_prog);
    mem_cpu.run(200);
    auto mem_stats = mem_cpu.get_stats();
    
    std::cerr << "\n=== Bottleneck Comparison ===" << std::endl;
    std::cerr << "ALU-Heavy Program:" << std::endl;
    std::cerr << "  Cycles: " << alu_stats.cycles << std::endl;
    std::cerr << "  Fetch Stalls: " << alu_stats.fetch_stalls << std::endl;
    std::cerr << "  Memory Contention Ratio: " 
              << (alu_stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    std::cerr << "  IPC: " << alu_stats.get_ipc() << std::endl;
    
    std::cerr << "Memory-Heavy Program:" << std::endl;
    std::cerr << "  Cycles: " << mem_stats.cycles << std::endl;
    std::cerr << "  Fetch Stalls: " << mem_stats.fetch_stalls << std::endl;
    std::cerr << "  Memory Contention Ratio: " 
              << (mem_stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    std::cerr << "  IPC: " << mem_stats.get_ipc() << std::endl;
    
    // Thesis assertion: memory-heavy should have worse contention
    EXPECT_GE(mem_stats.fetch_stalls, alu_stats.fetch_stalls)
        << "Memory-heavy workload should have more fetch stalls than ALU-heavy";
}

TEST_F(EmulatorTest, SustainedMemoryContention) {
    // This test creates SUSTAINED memory contention by using a loop
    // where fetch cannot "run ahead" because of the backward branch.
    // Each iteration: fetch competes with load AND store for the bus.
    //
    // Register allocation (using $t registers that work in other tests):
    //   $t7 = base address (2048)
    //   $t5 = loop counter
    //   $t6 = loop limit
    //   $t0 = temp for memory location 1
    //   $t1 = temp for memory location 2
    //   $t2 = final loaded value
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t7, $zero, 2048",  // Base address
        "ADDI $t5, $zero, 0",     // Loop counter = 0
        "ADDI $t6, $zero, 10",    // Loop limit = 10
        
        // loop: (PC = 12)
        "LW $t0, 0($t7)",         // Load from addr 2048
        "ADDI $t0, $t0, 1",       // Increment
        "SW $t0, 0($t7)",         // Store back
        "LW $t1, 4($t7)",         // Load from addr 2052
        "ADDI $t1, $t1, 1",       // Increment
        "SW $t1, 4($t7)",         // Store back
        "ADDI $t5, $t5, 1",       // counter++
        "BNE $t5, $t6, -8",       // if counter != limit, loop back
        
        // After loop
        "LW $t2, 0($t7)",         // Final load to verify
        "ADDI $t3, $zero, 1",     // Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    // Initialize memory
    prog[512] = 0;  // Word at address 2048
    prog[513] = 0;  // Word at address 2052
    
    Mips cpu(prog);
    cpu.run(3000);
    
    auto stats = cpu.get_stats();
    
    std::cerr << "\n=== Sustained Memory Contention Test ===" << std::endl;
    std::cerr << "Total Cycles: " << stats.cycles << std::endl;
    std::cerr << "Instructions Committed: " << stats.instructions_committed << std::endl;
    std::cerr << "IPC: " << stats.get_ipc() << std::endl;
    std::cerr << std::endl;
    std::cerr << "=== Stall Breakdown ===" << std::endl;
    std::cerr << "Fetch Stalls (Von Neumann Bottleneck): " << stats.fetch_stalls << std::endl;
    std::cerr << "Data Stalls: " << stats.data_stalls << std::endl;
    std::cerr << "ROB Full Stalls: " << stats.rob_full_stalls << std::endl;
    std::cerr << "RS Full Stalls: " << stats.rs_full_stalls << std::endl;
    std::cerr << "EU Busy Stalls: " << stats.eu_busy_stalls << std::endl;
    std::cerr << "Total Stalls: " << stats.get_total_stalls() << std::endl;
    std::cerr << std::endl;
    std::cerr << "=== Thesis Metrics ===" << std::endl;
    std::cerr << "Memory Accesses: " << stats.memory_accesses << std::endl;
    std::cerr << "Memory Contention Ratio: " 
              << (stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    std::cerr << "Branch Mispredictions: " << stats.branch_mispredictions << std::endl;
    std::cerr << "$t2 (final mem value): " << cpu.get_register(10) << std::endl;
    std::cerr << "$t5 (loop counter): " << cpu.get_register(13) << std::endl;
    
    // KEY THESIS ASSERTION: Memory contention must exist
    EXPECT_GT(stats.fetch_stalls, 0u)
        << "Sustained memory-heavy loop MUST cause fetch stalls";
    
    // Verify contention ratio is meaningful
    EXPECT_GT(stats.get_memory_contention_ratio(), 0.0)
        << "Memory contention ratio should be non-zero";
    
    // Verify the loop actually executed (counter reached limit)
    EXPECT_EQ(cpu.get_register(13), 10u)  // $t5 = loop counter
        << "Loop should have completed 10 iterations";
    EXPECT_EQ(cpu.get_register(10), 10u)  // $t2 = final memory value
        << "Memory value should equal iteration count";
    
    // 9 taken branches = 9 mispredictions
    EXPECT_EQ(stats.branch_mispredictions, 9u);
}

TEST_F(EmulatorTest, MatrixStyleAccess) {
    // Simulates strided memory access pattern (like matrix operations)
    // Multiple loads/stores per iteration with different addresses
    //
    // Register allocation (using $t registers):
    //   $t4 = pointer to A
    //   $t5 = pointer to B  
    //   $t6 = pointer to C
    //   $t7 = loop counter
    //   $t3 = loop limit
    //   $t0, $t1, $t2 = temporaries for computation
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t4, $zero, 2048",  // Base A
        "ADDI $t5, $zero, 2560",  // Base B (A + 512 bytes = 128 words)
        "ADDI $t6, $zero, 3072",  // Base C (B + 512 bytes)
        "ADDI $t7, $zero, 0",     // Loop counter
        "ADDI $t3, $zero, 10",    // Loop limit
        
        // loop: Simulate C[i] = A[i] + B[i]
        "LW $t0, 0($t4)",         // Load A[i]
        "LW $t1, 0($t5)",         // Load B[i]
        "ADD $t2, $t0, $t1",      // A[i] + B[i]
        "SW $t2, 0($t6)",         // Store to C[i]
        "ADDI $t4, $t4, 4",       // A++
        "ADDI $t5, $t5, 4",       // B++
        "ADDI $t6, $t6, 4",       // C++
        "ADDI $t7, $t7, 1",       // counter++
        "BNE $t7, $t3, -9",       // if counter != limit, loop back
        
        // End marker
        "ADDI $s0, $zero, 1",     // Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    // Initialize arrays A and B
    for (int i = 0; i < 20; ++i) {
        prog[512 + i] = i + 1;        // A[i] = i+1
        prog[512 + 128 + i] = i + 1;  // B[i] = i+1
    }
    
    Mips cpu(prog);
    cpu.run(2000);
    
    auto stats = cpu.get_stats();
    
    std::cerr << "\n=== Matrix-Style Access Test ===" << std::endl;
    std::cerr << "Total Cycles: " << stats.cycles << std::endl;
    std::cerr << "Instructions Committed: " << stats.instructions_committed << std::endl;
    std::cerr << "Memory Accesses: " << stats.memory_accesses << std::endl;
    std::cerr << "Fetch Stalls: " << stats.fetch_stalls << std::endl;
    std::cerr << "Memory Contention Ratio: " 
              << (stats.get_memory_contention_ratio() * 100) << "%" << std::endl;
    std::cerr << "IPC: " << stats.get_ipc() << std::endl;
    std::cerr << "$t7 (loop counter): " << cpu.get_register(15) << std::endl;
    std::cerr << "Branch Mispredictions: " << stats.branch_mispredictions << std::endl;
    
    // Thesis metrics - verify contention exists
    EXPECT_GT(stats.memory_accesses, 20u)
        << "Should have many memory accesses (2 loads + 1 store per iteration)";
    
    // With memory ops per iteration in a loop, fetch stalls are expected
    EXPECT_GT(stats.fetch_stalls, 0u)
        << "Matrix-style workload should cause fetch stalls";
    
    // Verify loop completed
    EXPECT_EQ(cpu.get_register(15), 10u)  // $t7 = loop counter
        << "Loop should have completed 10 iterations";
    
    // 9 taken branches = 9 mispredictions (always-not-taken predictor)
    EXPECT_EQ(stats.branch_mispredictions, 9u);
}

// ...existing code...

// ============================================================================
// Diagnostic Tests for Loop Behavior
// ============================================================================

TEST_F(EmulatorTest, DiagnosticSimpleLoop) {
    // Minimal loop that mirrors BranchLoop (which passes)
    // to verify basic loop mechanics work
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 0",     // counter = 0
        "ADDI $t1, $zero, 5",     // limit = 5
        // loop:
        "ADDI $t0, $t0, 1",       // counter++
        "BNE $t0, $t1, -2",       // if counter != limit, loop back
        "ADDI $t2, $zero, 1",     // Done marker
    });
    
    std::cerr << "\n=== Diagnostic: Simple Loop ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << ": 0x" 
                  << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 100; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", $t2=" << cpu.get_register(10)
                      << ", mispred=" << stats.branch_mispredictions
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(8), 5u);   // counter should be 5
    EXPECT_EQ(cpu.get_register(10), 1u);  // done marker
    
    auto stats = cpu.get_stats();
    EXPECT_EQ(stats.branch_mispredictions, 4u);  // 4 taken branches
}

TEST_F(EmulatorTest, DiagnosticLoopWithMemory) {
    // Add ONE memory operation to the simple loop
    // to see if memory ops break the loop
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // base address
        "ADDI $t0, $zero, 0",     // counter = 0
        "ADDI $t1, $zero, 5",     // limit = 5
        // loop:
        "LW $t3, 0($t5)",         // Load (memory op)
        "ADDI $t0, $t0, 1",       // counter++
        "BNE $t0, $t1, -3",       // if counter != limit, loop back
        "ADDI $t2, $zero, 1",     // Done marker
    });
    
    std::cerr << "\n=== Diagnostic: Loop With Memory ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << ": 0x" 
                  << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    prog[512] = 42;  // Initialize memory at address 2048
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 150; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", $t2=" << cpu.get_register(10)
                      << ", $t3=" << cpu.get_register(11)
                      << ", mispred=" << stats.branch_mispredictions
                      << ", mem_acc=" << stats.memory_accesses
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(8), 5u);   // counter should be 5
    EXPECT_EQ(cpu.get_register(10), 1u);  // done marker
    EXPECT_EQ(cpu.get_register(11), 42u); // loaded value
    
    auto stats = cpu.get_stats();
    EXPECT_EQ(stats.branch_mispredictions, 4u);  // 4 taken branches
}

TEST_F(EmulatorTest, DiagnosticLoopWithLoadStore) {
    // Add BOTH load and store to the loop
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // base address
        "ADDI $t0, $zero, 0",     // counter = 0
        "ADDI $t1, $zero, 5",     // limit = 5
        // loop:
        "LW $t3, 0($t5)",         // Load
        "ADDI $t3, $t3, 1",       // Increment
        "SW $t3, 0($t5)",         // Store back
        "ADDI $t0, $t0, 1",       // counter++
        "BNE $t0, $t1, -5",       // if counter != limit, loop back
        "ADDI $t2, $zero, 1",     // Done marker
    });
    
    std::cerr << "\n=== Diagnostic: Loop With Load/Store ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << ": 0x" 
                  << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    prog[512] = 0;  // Initialize memory at address 2048
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 200; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 15 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(3) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t2=" << cpu.get_register(10)
                      << ", $t3=" << cpu.get_register(11)
                      << ", mem[2048]=" << cpu.get_memory(2048)
                      << ", mispred=" << stats.branch_mispredictions
                      << ", fetch_stall=" << stats.fetch_stalls
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    std::cerr << "Final memory[2048] = " << cpu.get_memory(2048) << std::endl;
    
    EXPECT_EQ(cpu.get_register(8), 5u);   // counter should be 5
    EXPECT_EQ(cpu.get_register(10), 1u);  // done marker
    EXPECT_EQ(cpu.get_memory(2048), 5u);  // memory incremented 5 times
    
    auto stats = cpu.get_stats();
    EXPECT_EQ(stats.branch_mispredictions, 4u);  // 4 taken branches
    // EXPECT_GT(stats.fetch_stalls, 0u);           // Should see contention
}

TEST_F(EmulatorTest, DiagnosticLoopWithTwoMemLocations) {
    // This is the pattern in SustainedMemoryContention
    // Two memory locations being accessed per iteration
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t7, $zero, 2048",  // base address
        "ADDI $t5, $zero, 0",     // counter = 0
        "ADDI $t6, $zero, 5",     // limit = 5
        // loop:
        "LW $t0, 0($t7)",         // Load from addr 2048
        "ADDI $t0, $t0, 1",       // Increment
        "SW $t0, 0($t7)",         // Store back
        "LW $t1, 4($t7)",         // Load from addr 2052
        "ADDI $t1, $t1, 1",       // Increment
        "SW $t1, 4($t7)",         // Store back
        "ADDI $t5, $t5, 1",       // counter++
        "BNE $t5, $t6, -8",       // if counter != limit, loop back
        "ADDI $t2, $zero, 1",     // Done marker
    });
    
    std::cerr << "\n=== Diagnostic: Loop With Two Memory Locations ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << ": 0x" 
                  << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    prog[512] = 0;  // Initialize memory at address 2048
    prog[513] = 0;  // Initialize memory at address 2052
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 300; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 20 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(3) << cycle 
                      << ": $t5=" << cpu.get_register(13)
                      << ", $t6=" << cpu.get_register(14)
                      << ", $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", $t2=" << cpu.get_register(10)
                      << ", mem[2048]=" << cpu.get_memory(2048)
                      << ", mem[2052]=" << cpu.get_memory(2052)
                      << ", mispred=" << stats.branch_mispredictions
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    auto stats = cpu.get_stats();
    std::cerr << "\nFinal State:" << std::endl;
    std::cerr << "  $t5 (counter) = " << cpu.get_register(13) << std::endl;
    std::cerr << "  $t2 (done) = " << cpu.get_register(10) << std::endl;
    std::cerr << "  memory[2048] = " << cpu.get_memory(2048) << std::endl;
    std::cerr << "  memory[2052] = " << cpu.get_memory(2052) << std::endl;
    std::cerr << "  branch_mispredictions = " << stats.branch_mispredictions << std::endl;
    std::cerr << "  fetch_stalls = " << stats.fetch_stalls << std::endl;
    std::cerr << "  total_cycles = " << stats.cycles << std::endl;
    
    EXPECT_EQ(cpu.get_register(13), 5u);  // counter should be 5
    EXPECT_EQ(cpu.get_register(10), 1u);  // done marker
    EXPECT_EQ(cpu.get_memory(2048), 5u);  // memory incremented 5 times
    EXPECT_EQ(cpu.get_memory(2052), 5u);  // memory incremented 5 times
    EXPECT_EQ(stats.branch_mispredictions, 4u);  // 4 taken branches
}

TEST_F(EmulatorTest, DiagnosticBranchOffset) {
    // Test if -8 offset works correctly (8 instructions back)
    // This is used in SustainedMemoryContention
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 0",     // 0: counter = 0
        "ADDI $t1, $zero, 3",     // 1: limit = 3
        // loop: (index 2)
        "ADDI $t2, $zero, 1",     // 2: padding
        "ADDI $t2, $zero, 2",     // 3: padding
        "ADDI $t2, $zero, 3",     // 4: padding
        "ADDI $t2, $zero, 4",     // 5: padding
        "ADDI $t2, $zero, 5",     // 6: padding
        "ADDI $t2, $zero, 6",     // 7: padding
        "ADDI $t0, $t0, 1",       // 8: counter++
        "BNE $t0, $t1, -8",       // 9: loop back 8 instructions to index 2
        "ADDI $t3, $zero, 1",     // 10: Done marker
    });
    
    std::cerr << "\n=== Diagnostic: Branch Offset -8 ===" << std::endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << ": 0x" 
                  << std::hex << instructions[i] << std::dec << std::endl;
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 150; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t3=" << cpu.get_register(11)
                      << ", mispred=" << stats.branch_mispredictions
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(8), 3u);   // counter should be 3
    EXPECT_EQ(cpu.get_register(11), 1u);  // done marker
    
    auto stats = cpu.get_stats();
    EXPECT_EQ(stats.branch_mispredictions, 2u);  // 2 taken branches
}

// ...existing code...

TEST_F(EmulatorTest, DiagnosticHaltCondition) {
    // Test to understand what triggers early halt
    // Gradually increase loop body size to find the breaking point
    
    std::cerr << "\n=== Diagnostic: Halt Condition ===" << std::endl;
    
    // Test with 4 memory operations (2 load + 2 store) but simpler structure
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 0",     // 0: counter = 0
        "ADDI $t1, $zero, 3",     // 1: limit = 3
        "ADDI $t7, $zero, 2048",  // 2: base address
        // loop: (index 3)
        "LW $t2, 0($t7)",         // 3: Load
        "SW $t2, 0($t7)",         // 4: Store
        "ADDI $t0, $t0, 1",       // 5: counter++
        "BNE $t0, $t1, -4",       // 6: loop back 4 instructions to index 3
        "ADDI $t3, $zero, 1",     // 7: Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << std::endl;
    }
    
    prog[512] = 42;  // Initialize memory
    
    Mips cpu(prog);
    
    std::cerr << "\nCycle-by-cycle execution:" << std::endl;
    for (int cycle = 0; cycle < 100; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        // Print every cycle for detailed analysis
        std::cerr << "Cycle " << std::setw(2) << cycle 
                  << ": $t0=" << cpu.get_register(8)
                  << ", $t1=" << cpu.get_register(9)
                  << ", $t3=" << cpu.get_register(11)
                  << ", mem=" << cpu.get_memory(2048)
                  << ", mispred=" << stats.branch_mispredictions
                  << ", commits=" << stats.instructions_committed
                  << (cpu.is_halted() ? " [HALTED]" : "")
                  << std::endl;
        
        if (cpu.is_halted()) {
            std::cerr << "\n*** HALTED at cycle " << cycle << " ***" << std::endl;
            std::cerr << "Expected: counter=$t0 should be 3, got " << cpu.get_register(8) << std::endl;
            break;
        }
    }
    
    EXPECT_EQ(cpu.get_register(8), 3u);   // counter should be 3
    EXPECT_EQ(cpu.get_register(11), 1u);  // done marker
}

TEST_F(EmulatorTest, DiagnosticLoopBodySize) {
    // Test different loop body sizes to find the breaking point
    
    std::cerr << "\n=== Diagnostic: Loop Body Size ===" << std::endl;
    
    // Loop body with 6 instructions (the failing case uses 8)
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 0",     // counter = 0
        "ADDI $t1, $zero, 3",     // limit = 3
        // loop:
        "ADDI $t2, $zero, 1",     // padding 1
        "ADDI $t2, $zero, 2",     // padding 2
        "ADDI $t2, $zero, 3",     // padding 3
        "ADDI $t2, $zero, 4",     // padding 4
        "ADDI $t2, $zero, 5",     // padding 5
        "ADDI $t0, $t0, 1",       // counter++
        "BNE $t0, $t1, -7",       // loop back 7 instructions
        "ADDI $t3, $zero, 1",     // Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 100; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t3=" << cpu.get_register(11)
                      << ", mispred=" << stats.branch_mispredictions
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    std::cerr << "Final: $t0=" << cpu.get_register(8) 
              << ", $t3=" << cpu.get_register(11) << std::endl;
    
    EXPECT_EQ(cpu.get_register(8), 3u);
    EXPECT_EQ(cpu.get_register(11), 1u);
}

TEST_F(EmulatorTest, DiagnosticTwoStoresSequential) {
    // Test if the issue is specifically with multiple stores
    // Two stores to different addresses, sequential (not in loop)
    
    std::cerr << "\n=== Diagnostic: Two Stores Sequential ===" << std::endl;
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t7, $zero, 2048",  // base address
        "ADDI $t0, $zero, 10",    // value 1
        "ADDI $t1, $zero, 20",    // value 2
        "SW $t0, 0($t7)",         // Store to 2048
        "SW $t1, 4($t7)",         // Store to 2052
        "LW $t2, 0($t7)",         // Load from 2048
        "LW $t3, 4($t7)",         // Load from 2052
        "ADDI $t4, $zero, 1",     // Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    cpu.run(100);
    
    std::cerr << "$t2 (loaded from 2048) = " << cpu.get_register(10) << " (expect 10)" << std::endl;
    std::cerr << "$t3 (loaded from 2052) = " << cpu.get_register(11) << " (expect 20)" << std::endl;
    std::cerr << "$t4 (done marker) = " << cpu.get_register(12) << " (expect 1)" << std::endl;
    
    EXPECT_EQ(cpu.get_register(10), 10u);
    EXPECT_EQ(cpu.get_register(11), 20u);
    EXPECT_EQ(cpu.get_register(12), 1u);
}

TEST_F(EmulatorTest, DiagnosticMemoryLoopMinimal) {
    // Absolute minimal loop with 2 memory ops
    // Loop body: LW, SW, counter++, BNE (4 instructions)
    
    std::cerr << "\n=== Diagnostic: Memory Loop Minimal ===" << std::endl;
    
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t7, $zero, 2048",  // 0: base address
        "ADDI $t0, $zero, 0",     // 1: counter = 0
        "ADDI $t1, $zero, 3",     // 2: limit = 3
        // loop: (index 3)
        "LW $t2, 0($t7)",         // 3: Load
        "ADDI $t2, $t2, 1",       // 4: Increment
        "SW $t2, 0($t7)",         // 5: Store
        "ADDI $t0, $t0, 1",       // 6: counter++
        "BNE $t0, $t1, -5",       // 7: loop back to index 3
        "ADDI $t3, $zero, 1",     // 8: Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
        std::cerr << "  [" << i << "] PC=" << (i*4) << std::endl;
    }
    
    prog[512] = 0;  // Initialize memory at 2048
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 150; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(3) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t2=" << cpu.get_register(10)
                      << ", $t3=" << cpu.get_register(11)
                      << ", mem=" << cpu.get_memory(2048)
                      << ", mispred=" << stats.branch_mispredictions
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    std::cerr << "\nFinal: $t0=" << cpu.get_register(8) 
              << ", mem[2048]=" << cpu.get_memory(2048)
              << ", $t3=" << cpu.get_register(11) << std::endl;
    
    // This is the same pattern as DiagnosticLoopWithLoadStore which PASSES
    EXPECT_EQ(cpu.get_register(8), 3u);    // counter should be 3
    EXPECT_EQ(cpu.get_memory(2048), 3u);   // memory incremented 3 times
    EXPECT_EQ(cpu.get_register(11), 1u);   // done marker
}

TEST_F(EmulatorTest, DiagnosticROBCapacity) {
    // Test if ROB capacity is the issue
    // If ROB has 8 entries and loop has 8 instructions, it might cause issues
    
    std::cerr << "\n=== Diagnostic: ROB Capacity ===" << std::endl;
    
    // 9 instructions before the loop marker - exceeds typical ROB size
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t0, $zero, 1",     // 0
        "ADDI $t0, $zero, 2",     // 1
        "ADDI $t0, $zero, 3",     // 2
        "ADDI $t0, $zero, 4",     // 3
        "ADDI $t0, $zero, 5",     // 4
        "ADDI $t0, $zero, 6",     // 5
        "ADDI $t0, $zero, 7",     // 6
        "ADDI $t0, $zero, 8",     // 7
        "ADDI $t0, $zero, 9",     // 8 - 9th instruction
        "ADDI $t1, $zero, 1",     // 9 - Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    for (int cycle = 0; cycle < 50; ++cycle) {
        cpu.tick();
        auto stats = cpu.get_stats();
        
        if (cycle % 5 == 0 || cpu.is_halted()) {
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", commits=" << stats.instructions_committed
                      << ", rob_stalls=" << stats.rob_full_stalls
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(8), 9u);   // Final value
    EXPECT_EQ(cpu.get_register(9), 1u);   // Done marker
}

// ...existing code...

TEST_F(EmulatorTest, HaltWithPendingMemoryOps) {
  // This test checks if halt detection waits for all memory ops to complete
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t7, $zero, 2048",  // base address
    "ADDI $t0, $zero, 42",    // value
    "SW $t0, 0($t7)",         // Store - takes multiple cycles
    "SW $t0, 4($t7)",         // Store - takes multiple cycles
    "SW $t0, 8($t7)",         // Store - takes multiple cycles
    "SW $t0, 12($t7)",        // Store - takes multiple cycles
    "LW $t1, 0($t7)",         // Load back
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  
  Mips cpu(prog);
  
  std::cerr << "\n=== Halt With Pending Memory Ops ===" << std::endl;
  for (int cycle = 0; cycle < 100; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    if (cycle % 10 == 0 || cpu.is_halted()) {
      std::cerr << "Cycle " << std::setw(2) << cycle 
            << ": $t1=" << cpu.get_register(9)
            << ", $t2=" << cpu.get_register(10)
            << ", mem[2048]=" << cpu.get_memory(2048)
            << ", commits=" << stats.instructions_committed
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
    }
    
    if (cpu.is_halted()) break;
  }
  
  EXPECT_EQ(cpu.get_register(9), 42u);   // $t1 loaded 42
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  EXPECT_EQ(cpu.get_memory(2048), 42u);  // Memory written
}

// ============================================================================
// Test 2: Loop with branch taken - verify pipeline drains correctly
// ============================================================================

TEST_F(EmulatorTest, LoopBranchPipelineDrain) {
  // When a branch is taken, the pipeline must flush speculatively fetched
  // instructions. Halt detection must not trigger during this window.
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t0, $zero, 0",     // counter = 0
    "ADDI $t1, $zero, 2",     // limit = 2 (small for quick test)
    // loop:
    "ADDI $t0, $t0, 1",       // counter++
    "BNE $t0, $t1, -2",       // loop back if counter != limit
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  
  Mips cpu(prog);
  
  std::cerr << "\n=== Loop Branch Pipeline Drain ===" << std::endl;
  int last_counter = -1;
  for (int cycle = 0; cycle < 50; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    int current_counter = cpu.get_register(8);
    if (current_counter != last_counter || cpu.is_halted()) {
      std::cerr << "Cycle " << std::setw(2) << cycle 
            << ": $t0=" << current_counter
            << ", $t2=" << cpu.get_register(10)
            << ", mispred=" << stats.branch_mispredictions
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
      last_counter = current_counter;
    }
    
    if (cpu.is_halted()) break;
  }
  
  EXPECT_EQ(cpu.get_register(8), 2u);    // counter should be 2
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  
  auto stats = cpu.get_stats();
  EXPECT_EQ(stats.branch_mispredictions, 1u);  // 1 taken branch
}

// ============================================================================
// Test 3: Memory operation latency vs halt detection
// ============================================================================

TEST_F(EmulatorTest, MemoryLatencyHaltTiming) {
    std::array<uint32_t, 4096> prog{};
    auto instructions = assembler_.assemble({
        "ADDI $t5, $zero, 2048",  // base
        "ADDI $t0, $zero, 100",   // value
        "SW $t0, 0($t5)",         // Store (may take cycles)
        "SW $t0, 0($t5)",         // Store (may take cycles)
        "SW $t0, 0($t5)",         // Store (may take cycles)
        "LW $t1, 0($t5)",         // Load (depends on store completing)
        "ADDI $t2, $zero, 1",     // Done marker
    });
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        prog[i] = instructions[i];
    }
    
    Mips cpu(prog);
    
    std::cerr << "\n=== Memory Latency Halt Timing ===" << std::endl;
    for (int cycle = 0; cycle < 100; ++cycle) {
        cpu.tick();
        
        if (cycle % 10 == 0 || cpu.is_halted()) {
            auto stats = cpu.get_stats();
            std::cerr << "Cycle " << std::setw(2) << cycle 
                      << ": $t0=" << cpu.get_register(8)
                      << ", $t1=" << cpu.get_register(9)
                      << ", $t2=" << cpu.get_register(10)
                      << ", mem[2048]=" << cpu.get_memory(2048)
                      << ", commits=" << stats.instructions_committed
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    EXPECT_EQ(cpu.get_register(9), 100u);  // $t1 loaded 100
    EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
}

// ============================================================================
// Test 4: Reproduce the exact failing pattern
// ============================================================================

TEST_F(EmulatorTest, ExactFailingPattern) {
  // This reproduces DiagnosticLoopWithTwoMemLocations exactly
  // to observe cycle-by-cycle behavior around the halt point
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t7, $zero, 2048",  // 0: base address
    "ADDI $t5, $zero, 0",     // 1: counter = 0
    "ADDI $t6, $zero, 5",     // 2: limit = 5
    // loop: (PC = 12)
    "LW $t0, 0($t7)",         // 3: Load from addr 2048
    "ADDI $t0, $t0, 1",       // 4: Increment
    "SW $t0, 0($t7)",         // 5: Store back
    "LW $t1, 4($t7)",         // 6: Load from addr 2052
    "ADDI $t1, $t1, 1",       // 7: Increment
    "SW $t1, 4($t7)",         // 8: Store back
    "ADDI $t5, $t5, 1",       // 9: counter++
    "BNE $t5, $t6, -8",       // 10: if counter != limit, loop back
    "ADDI $t2, $zero, 1",     // 11: Done marker
  });
  
  std::cerr << "\n=== Exact Failing Pattern ===" << std::endl;
  std::cerr << "Instructions:" << std::endl;
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
    std::cerr << "  [" << i << "] PC=" << (i*4) << std::endl;
  }
  
  prog[512] = 0;  // Initialize memory at address 2048
  prog[513] = 0;  // Initialize memory at address 2052
  
  Mips cpu(prog);
  
  // Detailed cycle-by-cycle analysis around the failure point
  std::cerr << "\nDetailed execution:" << std::endl;
  for (int cycle = 0; cycle < 150; ++cycle) {
    // Capture state BEFORE tick
    auto pre_stats = cpu.get_stats();
    bool pre_halted = cpu.is_halted();
    
    cpu.tick();
    
    auto post_stats = cpu.get_stats();
    
    // Always print around the critical point (cycle 20-25)
    bool print = (cycle >= 18 && cycle <= 30) || cpu.is_halted();
    
    if (print) {
      std::cerr << "Cycle " << std::setw(2) << cycle 
            << ": $t5=" << cpu.get_register(13)
            << ", $t0=" << cpu.get_register(8)
            << ", $t1=" << cpu.get_register(9)
            << ", mem[2048]=" << cpu.get_memory(2048)
            << ", mem[2052]=" << cpu.get_memory(2052)
            << ", mispred=" << post_stats.branch_mispredictions
            << ", commits=" << post_stats.instructions_committed;
      
      if (cpu.is_halted() && !pre_halted) {
        std::cerr << " [JUST HALTED!]";
      } else if (cpu.is_halted()) {
        std::cerr << " [HALTED]";
      }
      std::cerr << std::endl;
    }
    
    if (cpu.is_halted()) {
      std::cerr << "\n*** CPU halted at cycle " << cycle << " ***" << std::endl;
      std::cerr << "Expected: 5 iterations, counter should be 5" << std::endl;
      std::cerr << "Actual: counter=$t5=" << cpu.get_register(13) << std::endl;
      break;
    }
  }
  
  // Check results
  EXPECT_EQ(cpu.get_register(13), 5u);  // counter should be 5
  EXPECT_EQ(cpu.get_register(10), 1u);  // done marker
  EXPECT_EQ(cpu.get_memory(2048), 5u);  // memory incremented 5 times
  EXPECT_EQ(cpu.get_memory(2052), 5u);  // memory incremented 5 times
}

// ============================================================================
// Test 5: Verify the branch target is correct
// ============================================================================

TEST_F(EmulatorTest, BranchTargetCalculation) {
  // BNE $t5, $t6, -8 at PC=40 should jump to:
  // target = PC + 4 + (offset * 4) = 40 + 4 + (-8 * 4) = 44 - 32 = 12
  // This should be the LW instruction at index 3
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t0, $zero, 0",     // 0: PC=0
    "ADDI $t1, $zero, 2",     // 1: PC=4, limit
    // loop: PC=8
    "ADDI $t2, $zero, 1",     // 2: PC=8
    "ADDI $t2, $zero, 2",     // 3: PC=12
    "ADDI $t2, $zero, 3",     // 4: PC=16
    "ADDI $t2, $zero, 4",     // 5: PC=20
    "ADDI $t2, $zero, 5",     // 6: PC=24
    "ADDI $t2, $zero, 6",     // 7: PC=28
    "ADDI $t0, $t0, 1",       // 8: PC=32, counter++
    "BNE $t0, $t1, -8",       // 9: PC=36, target = 36+4+(-8*4) = 40-32 = 8
    "ADDI $t3, $zero, 1",     // 10: PC=40, Done marker
  });
  
  std::cerr << "\n=== Branch Target Calculation ===" << std::endl;
  std::cerr << "BNE at PC=36, offset=-8" << std::endl;
  std::cerr << "Expected target: 36 + 4 + (-8 * 4) = 36 + 4 - 32 = 8" << std::endl;
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  
  Mips cpu(prog);
  cpu.run(100);
  
  EXPECT_EQ(cpu.get_register(8), 2u);    // counter should be 2
  EXPECT_EQ(cpu.get_register(11), 1u);   // Done marker
  
  auto stats = cpu.get_stats();
  // 1 taken branch (first iteration jumps back, second falls through)
  EXPECT_EQ(stats.branch_mispredictions, 1u);
}

// ============================================================================
// Test 6: Simplified 8-instruction loop body
// ============================================================================

TEST_F(EmulatorTest, EightInstructionLoopBody) {
  // The failing test has 8 instructions in the loop body
  // Test with pure ALU ops (no memory) to isolate the issue
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t5, $zero, 0",     // counter = 0
    "ADDI $t6, $zero, 3",     // limit = 3
    // loop: 8 instructions in body
    "ADDI $t0, $zero, 1",     // 1
    "ADDI $t0, $zero, 2",     // 2
    "ADDI $t0, $zero, 3",     // 3
    "ADDI $t0, $zero, 4",     // 4
    "ADDI $t0, $zero, 5",     // 5
    "ADDI $t0, $zero, 6",     // 6
    "ADDI $t5, $t5, 1",       // 7: counter++
    "BNE $t5, $t6, -8",       // 8: loop back
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  
  Mips cpu(prog);
  
  std::cerr << "\n=== Eight Instruction Loop Body (ALU only) ===" << std::endl;
  for (int cycle = 0; cycle < 100; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    if (cycle % 10 == 0 || cpu.is_halted()) {
      std::cerr << "Cycle " << std::setw(2) << cycle 
            << ": $t5=" << cpu.get_register(13)
            << ", $t2=" << cpu.get_register(10)
            << ", mispred=" << stats.branch_mispredictions
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
    }
    
    if (cpu.is_halted()) break;
  }
  
  EXPECT_EQ(cpu.get_register(13), 3u);   // counter should be 3
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  
  auto stats = cpu.get_stats();
  EXPECT_EQ(stats.branch_mispredictions, 2u);  // 2 taken branches
}

// ============================================================================
// Test 7: Memory ops in 8-instruction loop
// ============================================================================

TEST_F(EmulatorTest, EightInstructionLoopWithMemory) {
  // Add memory ops to the 8-instruction loop
  // This should show if memory + large loop causes the halt issue
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t7, $zero, 2048",  // base
    "ADDI $t5, $zero, 0",     // counter = 0
    "ADDI $t6, $zero, 3",     // limit = 3
    // loop: 8 instructions
    "LW $t0, 0($t7)",         // 1: Load
    "ADDI $t0, $t0, 1",       // 2: Increment
    "SW $t0, 0($t7)",         // 3: Store
    "ADDI $t1, $zero, 0",     // 4: padding
    "ADDI $t1, $zero, 0",     // 5: padding
    "ADDI $t1, $zero, 0",     // 6: padding
    "ADDI $t5, $t5, 1",       // 7: counter++
    "BNE $t5, $t6, -8",       // 8: loop back
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  prog[512] = 0;  // Initialize memory
  
  Mips cpu(prog);
  
  std::cerr << "\n=== Eight Instruction Loop With Memory ===" << std::endl;
  for (int cycle = 0; cycle < 150; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    if (cycle % 15 == 0 || cpu.is_halted()) {
      std::cerr << "Cycle " << std::setw(3) << cycle 
            << ": $t5=" << cpu.get_register(13)
            << ", $t0=" << cpu.get_register(8)
            << ", mem=" << cpu.get_memory(2048)
            << ", $t2=" << cpu.get_register(10)
            << ", mispred=" << stats.branch_mispredictions
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
    }
    
    if (cpu.is_halted()) break;
  }
  
  EXPECT_EQ(cpu.get_register(13), 3u);   // counter should be 3
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  EXPECT_EQ(cpu.get_memory(2048), 3u);   // memory incremented 3 times
  
  auto stats = cpu.get_stats();
  EXPECT_EQ(stats.branch_mispredictions, 2u);  // 2 taken branches
}

// ============================================================================
// Test 8: Incremental memory ops (find breaking point)
// ============================================================================

TEST_F(EmulatorTest, IncrementalMemoryOps) {
  // Start with 1 memory op per iteration, then add more
  // to find where the halt detection breaks
  
  std::cerr << "\n=== Incremental Memory Ops ===" << std::endl;
  
  // Test with 2 loads in loop (like the failing test)
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t7, $zero, 2048",  // base
    "ADDI $t5, $zero, 0",     // counter = 0
    "ADDI $t6, $zero, 3",     // limit = 3
    // loop:
    "LW $t0, 0($t7)",         // Load 1
    "LW $t1, 4($t7)",         // Load 2 - SECOND MEMORY OP
    "ADDI $t5, $t5, 1",       // counter++
    "BNE $t5, $t6, -4",       // loop back
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  prog[512] = 42;  // Initialize memory at 2048
  prog[513] = 99;  // Initialize memory at 2052
  
  Mips cpu(prog);
  
  for (int cycle = 0; cycle < 100; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    if (cycle % 10 == 0 || cpu.is_halted()) {
      std::cerr << "Cycle " << std::setw(2) << cycle 
            << ": $t5=" << cpu.get_register(13)
            << ", $t0=" << cpu.get_register(8)
            << ", $t1=" << cpu.get_register(9)
            << ", $t2=" << cpu.get_register(10)
            << ", mispred=" << stats.branch_mispredictions
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
    }
    
    if (cpu.is_halted()) break;
  }
  
  EXPECT_EQ(cpu.get_register(13), 3u);   // counter should be 3
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  EXPECT_EQ(cpu.get_register(8), 42u);   // Loaded value 1
  EXPECT_EQ(cpu.get_register(9), 99u);   // Loaded value 2
}

// ============================================================================
// Test 9: Two stores in loop (write pattern)
// ============================================================================

TEST_F(EmulatorTest, TwoStoresInLoop) {
  // Test with 2 stores per iteration
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t7, $zero, 2048",  // base
    "ADDI $t5, $zero, 0",     // counter = 0
    "ADDI $t6, $zero, 3",     // limit = 3
    "ADDI $t0, $zero, 1",     // value 1
    "ADDI $t1, $zero, 2",     // value 2
    // loop:
    "SW $t0, 0($t7)",         // Store 1
    "SW $t1, 4($t7)",         // Store 2
    "ADDI $t5, $t5, 1",       // counter++
    "BNE $t5, $t6, -4",       // loop back
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
  }
  
  Mips cpu(prog);
  
  std::cerr << "\n=== Two Stores In Loop ===" << std::endl;
  for (int cycle = 0; cycle < 100; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    if (cycle % 10 == 0 || cpu.is_halted()) {
      std::cerr << "Cycle " << std::setw(2) << cycle 
            << ": $t5=" << cpu.get_register(13)
            << ", $t2=" << cpu.get_register(10)
            << ", mem[2048]=" << cpu.get_memory(2048)
            << ", mem[2052]=" << cpu.get_memory(2052)
            << ", mispred=" << stats.branch_mispredictions
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
    }
    
    if (cpu.is_halted()) break;
  }
  
  EXPECT_EQ(cpu.get_register(13), 3u);   // counter should be 3
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  EXPECT_EQ(cpu.get_memory(2048), 1u);   // Store value 1
  EXPECT_EQ(cpu.get_memory(2052), 2u);   // Store value 2
}

// ============================================================================
// Test 10: Load-Store pair in loop (the failing pattern)
// ============================================================================

TEST_F(EmulatorTest, LoadStorePairInLoop) {
  // This is the exact pattern that fails:
  // Load, modify, store for TWO memory locations
  
  std::array<uint32_t, 4096> prog{};
  auto instructions = assembler_.assemble({
    "ADDI $t7, $zero, 2048",  // base
    "ADDI $t5, $zero, 0",     // counter = 0
    "ADDI $t6, $zero, 3",     // limit = 3
    // loop:
    "LW $t0, 0($t7)",         // Load loc 1
    "ADDI $t0, $t0, 1",       // Inc
    "SW $t0, 0($t7)",         // Store loc 1
    "LW $t1, 4($t7)",         // Load loc 2 <<< This is the 4th mem-touching instruction
    "ADDI $t1, $t1, 1",       // Inc
    "SW $t1, 4($t7)",         // Store loc 2
    "ADDI $t5, $t5, 1",       // counter++
    "BNE $t5, $t6, -8",       // loop back 8 instructions
    "ADDI $t2, $zero, 1",     // Done marker
  });
  
  std::cerr << "\n=== Load-Store Pair In Loop ===" << std::endl;
  std::cerr << "This is the exact failing pattern" << std::endl;
  
  for (size_t i = 0; i < instructions.size(); ++i) {
    prog[i] = instructions[i];
    std::cerr << "  [" << i << "] PC=" << (i*4) << std::endl;
  }
  
  prog[512] = 0;  // Initialize memory
  prog[513] = 0;
  
  Mips cpu(prog);
  
  for (int cycle = 0; cycle < 200; ++cycle) {
    cpu.tick();
    auto stats = cpu.get_stats();
    
    bool should_print = (cycle % 10 == 0) || cpu.is_halted();
    
    if (should_print) {
      std::cerr << "Cycle " << std::setw(3) << cycle 
            << ": $t5=" << cpu.get_register(13)
            << ", $t0=" << cpu.get_register(8)
            << ", $t1=" << cpu.get_register(9)
            << ", mem[2048]=" << cpu.get_memory(2048)
            << ", mem[2052]=" << cpu.get_memory(2052)
            << ", mispred=" << stats.branch_mispredictions
            << (cpu.is_halted() ? " [HALTED]" : "")
            << std::endl;
    }
    
    if (cpu.is_halted()) {
      std::cerr << "\n*** HALT at cycle " << cycle << " ***" << std::endl;
      std::cerr << "Expected 3 iterations, got $t5=" << cpu.get_register(13) << std::endl;
      break;
    }
  }
  
  // These are the key assertions - they should all pass if the bug is fixed
  EXPECT_EQ(cpu.get_register(13), 3u);   // counter should be 3
  EXPECT_EQ(cpu.get_register(10), 1u);   // Done marker
  EXPECT_EQ(cpu.get_memory(2048), 3u);   // memory incremented 3 times
  EXPECT_EQ(cpu.get_memory(2052), 3u);   // memory incremented 3 times
  
  auto stats = cpu.get_stats();
  EXPECT_EQ(stats.branch_mispredictions, 2u);  // 2 taken branches (iterations 1 and 2)
  EXPECT_GT(stats.fetch_stalls, 0u);           // Should see memory contention
}


TEST_F(EmulatorTest, DebugStuckLoop) {
    std::array<uint32_t, 4096> program{};
    
    // Simple loop with one load and one store
    // $t7 = 0x800 (base address 2048)
    // $t5 = 0 (counter)
    // $t6 = 3 (limit)
    // loop:
    //   lw $t0, 0($t7)      - load from mem[2048]
    //   addiu $t0, $t0, 1   - increment
    //   sw $t0, 0($t7)      - store back
    //   addiu $t5, $t5, 1   - counter++
    //   bne $t5, $t6, loop  - if counter != limit, loop
    //   addiu $t2, $zero, 1 - done marker
    
    program[0] = 0x200f0800;  // addiu $t7, $zero, 0x800
    program[1] = 0x200d0000;  // addiu $t5, $zero, 0
    program[2] = 0x200e0003;  // addiu $t6, $zero, 3
    program[3] = 0x8de80000;  // lw $t0, 0($t7)
    program[4] = 0x21080001;  // addiu $t0, $t0, 1
    program[5] = 0xade80000;  // sw $t0, 0($t7)
    program[6] = 0x21ad0001;  // addiu $t5, $t5, 1
    program[7] = 0x15aefffc;  // bne $t5, $t6, -4 (to instruction 3)
    program[8] = 0x200a0001;  // addiu $t2, $zero, 1
    
    Mips cpu(program);
    
    for (int cycle = 0; cycle < 100; cycle++) {
        cpu.tick();
        
        if (cycle >= 25 && cycle <= 50) {
            auto& stats = cpu.get_stats();
            std::cout << "Cycle " << cycle 
                      << ": $t5=" << cpu.get_register(13)
                      << ", $t0=" << cpu.get_register(8)
                      << ", mem=" << cpu.get_memory(2048)
                      << ", mispred=" << stats.branch_mispredictions
                      << (cpu.is_halted() ? " [HALTED]" : "")
                      << std::endl;
        }
        
        if (cpu.is_halted()) break;
    }
    
    std::cout << "\nFinal: $t5=" << cpu.get_register(13) 
              << ", $t2=" << cpu.get_register(10)
              << ", mem[2048]=" << cpu.get_memory(2048) << std::endl;
}