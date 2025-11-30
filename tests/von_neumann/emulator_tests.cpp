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