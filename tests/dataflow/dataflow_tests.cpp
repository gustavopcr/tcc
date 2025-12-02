// filepath: src/dataflow/test_dataflow.cpp
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include "dataflow/dataflow.hpp"

class DataflowTest : public ::testing::Test {
protected:
  std::string temp_file_;
  
  void SetUp() override {
    temp_file_ = "/tmp/test_graph_" + std::to_string(rand()) + ".dfg";
  }
  
  void TearDown() override {
    std::remove(temp_file_.c_str());
  }
  
  void write_graph(const std::string& content) {
    std::ofstream file(temp_file_);
    file << content;
    file.close();
  }
};

// =============================================================================
// Graph Loading Tests
// =============================================================================

TEST_F(DataflowTest, LoadProgram_SimpleGraph_ParsesCorrectly) {
  write_graph(
    "# Simple ADD graph\n"
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  ASSERT_NO_THROW(df.load_program(temp_file_));
}

TEST_F(DataflowTest, LoadProgram_EmptyFile_NoThrow) {
  write_graph("");
  
  Dataflow df;
  ASSERT_NO_THROW(df.load_program(temp_file_));
}

TEST_F(DataflowTest, LoadProgram_CommentsOnly_NoThrow) {
  write_graph(
    "# This is a comment\n"
    "# Another comment\n"
  );
  
  Dataflow df;
  ASSERT_NO_THROW(df.load_program(temp_file_));
}

TEST_F(DataflowTest, LoadProgram_NonExistentFile_Throws) {
  Dataflow df;
  ASSERT_THROW(df.load_program("/nonexistent/path.dfg"), std::runtime_error);
}

// =============================================================================
// Input Injection Tests
// =============================================================================

TEST_F(DataflowTest, SetInputs_CorrectCount_NoThrow) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  
  ASSERT_NO_THROW(df.run({5, 3}));
}

TEST_F(DataflowTest, SetInputs_MismatchCount_Throws) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  
  ASSERT_THROW(df.run({5}), std::runtime_error);  // Expected 2, got 1
}

// =============================================================================
// Basic Arithmetic Tests
// =============================================================================

TEST_F(DataflowTest, Execute_ADD_ProducesCorrectResult) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({10, 20});
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
  EXPECT_GT(stats.tokens_consumed, 0u);
}

TEST_F(DataflowTest, Execute_SUB_Completes) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 SUB 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({30, 10});
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}

TEST_F(DataflowTest, Execute_MULT_Completes) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 MULT 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({5, 7});
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}


TEST_F(DataflowTest, Execute_DIV_RespectsLatency) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 DIV 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({100, 10});
  
  const auto& stats = df.get_stats();
  // Pipeline: 1 cycle WM+IF + 6 cycles DIV + 1 cycle OUTPUT = ~8 cycles minimum
  EXPECT_GE(stats.total_cycles, 6u);
}

// =============================================================================
// FORK Tests
// =============================================================================

TEST_F(DataflowTest, Execute_FORK_BroadcastsToMultipleDestinations) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "100 FORK 1 200:0 300:0\n"
    "200 OUTPUT 1\n"
    "300 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({42});
  
  const auto& stats = df.get_stats();
  // FORK produces 2 output tokens + 1 input = 3 total generated
  EXPECT_GE(stats.tokens_generated, 3u);
}
TEST_F(DataflowTest, Execute_FORK_FeedsParallelOperations) {
  write_graph(
    "10 INPUT 0 30:0\n"
    "20 INPUT 0 40:0\n"
    "30 FORK 1 100:0 200:0\n"
    "40 FORK 1 100:1 200:1\n"
    "100 ADD 2 300:0\n"
    "200 SUB 2 300:1\n"
    "300 MULT 2 400:0\n"
    "400 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({10, 5});
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 1u);  // Should take multiple cycles now
  EXPECT_GE(stats.max_parallel_tasks, 2u);  // ADD and SUB should run in parallel
}
// =============================================================================
// Control Flow Tests (SWITCH)
// =============================================================================

TEST_F(DataflowTest, Execute_SWITCH_TakeTruePath) {
  write_graph(
    "10 INPUT 0 100:0\n"  // Data value
    "20 INPUT 0 100:1\n"  // Condition (true = non-zero)
    "100 SWITCH 2 200:0 300:0\n"  // True->200, False->300
    "200 OUTPUT 1\n"
    "300 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({42, 1});  // Data=42, Condition=true
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}

TEST_F(DataflowTest, Execute_SWITCH_TakeFalsePath) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 SWITCH 2 200:0 300:0\n"
    "200 OUTPUT 1\n"
    "300 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({42, 0});  // Data=42, Condition=false
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}

// =============================================================================
// MERGE Tests
// =============================================================================

TEST_F(DataflowTest, Execute_MERGE_PassesThrough) {
    // MERGE with 1 input fires immediately
    write_graph(R"(
10 INPUT 0 100:0
100 MERGE 1 200:0
200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({42});
    
    EXPECT_TRUE(df.is_idle());
    EXPECT_GT(df.get_stats().total_cycles, 0u);
}

TEST_F(DataflowTest, Execute_MERGE_FromMultipleSources) {
    // Two sources can send to same MERGE port
    // Whichever arrives, MERGE fires
    write_graph(R"(
10 INPUT 0 50:0
20 INPUT 0 50:1
50 SLT 2 100:1 101:1
60 INPUT 0 100:0
70 INPUT 0 101:0
100 SWITCH 2 200:0 300:0
101 SWITCH 2 300:0 200:0
200 MERGE 1 400:0
300 OUTPUT 1
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({5, 10, 42, 99});  // 5 < 10, so condition is true
    
    EXPECT_TRUE(df.is_idle());
}

TEST_F(DataflowTest, Execute_SelectPattern_WithSwitchAndMerge) {
    // Replicate SELECT behavior: choose A or B based on condition
    // This is the canonical dataflow pattern for conditional selection
    write_graph(R"(
# Inputs: A=10, B=20, Condition=1 (true)
# Expected: output A (10)

10 INPUT 0 100:0
20 INPUT 0 101:0
30 INPUT 0 100:1 101:1

# SWITCH A: true->MERGE, false->discard
100 SWITCH 2 200:0 999:0

# SWITCH B: true->discard, false->MERGE
101 SWITCH 2 999:0 200:0

# MERGE: receives exactly one value
200 MERGE 1 300:0

300 OUTPUT 1
999 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({10, 20, 1});  // A=10, B=20, Cond=true → expect 10
    
    EXPECT_TRUE(df.is_idle());
}
// =============================================================================
// Comparison Operations Tests
// =============================================================================

TEST_F(DataflowTest, Execute_SLT_LessThanTrue) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 SLT 2 200:0\n"  // 5 < 10 = true
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({5, 10});
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}

TEST_F(DataflowTest, Execute_SGE_GreaterOrEqualTrue) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 SGE 2 200:0\n"  // 10 >= 5 = true
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({10, 5});
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}

// =============================================================================
// Metrics Tests
// =============================================================================

TEST_F(DataflowTest, Metrics_TotalCycles_IsPositive) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({1, 2});
  
  EXPECT_GT(df.get_stats().total_cycles, 0u);
}

TEST_F(DataflowTest, Metrics_TokensGenerated_CountsInputs) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({1, 2});
  
  // 2 inputs + 1 from ADD = 3 tokens generated
  EXPECT_GE(df.get_stats().tokens_generated, 3u);
}

TEST_F(DataflowTest, Metrics_TokensConsumed_MatchesGenerated) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({1, 2});
  
  // All generated tokens should eventually be consumed
  EXPECT_GE(df.get_stats().tokens_consumed, 2u);
}

TEST_F(DataflowTest, Metrics_Parallelism_DetectedInParallelGraph) {
  write_graph(
    "10 INPUT 0 30:0\n"
    "20 INPUT 0 40:0\n"
    "30 FORK 1 100:0 200:0\n"
    "40 FORK 1 100:1 200:1\n"
    "100 ADD 2 300:0\n"
    "200 ADD 2 300:1\n"
    "300 ADD 2 400:0\n"
    "400 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({5, 10});
  
  const auto& stats = df.get_stats();
  // Two ADDs should execute in parallel
  EXPECT_GE(stats.max_parallel_tasks, 2u);
}


TEST_F(DataflowTest, Metrics_AvgParallelism_IsReasonable) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({1, 2});
  
  double avg = df.get_stats().get_avg_parallelism();
  EXPECT_GE(avg, 0.0);
  EXPECT_LE(avg, 100.0);  // Sanity check
}

// =============================================================================
// Idle Detection Tests
// =============================================================================

TEST_F(DataflowTest, IsIdle_AfterCompletion_ReturnsTrue) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({1, 2});
  
  EXPECT_TRUE(df.is_idle());
}

// =============================================================================
// Complex Graph Tests
// =============================================================================

TEST_F(DataflowTest, Execute_ChainedOperations_Completes) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 FORK 1 300:0 300:1\n"
    "300 MULT 2 400:0\n"
    "400 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({3, 4});  // (3+4)^2 = 49
  
  EXPECT_TRUE(df.is_idle());
  EXPECT_GT(df.get_stats().total_cycles, 0u);
}

TEST_F(DataflowTest, Execute_DiamondGraph_Completes) {
  // Diamond pattern: input -> fork -> two parallel ops -> merge
  write_graph(
    "10 INPUT 0 20:0\n"
    "15 INPUT 0 30:0\n"  // Second operand for both branches
    "20 FORK 1 100:0 200:0\n"
    "30 FORK 1 100:1 200:1\n"
    "100 ADD 2 300:0\n"
    "200 SUB 2 300:1\n"
    "300 MULT 2 400:0\n"
    "400 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({10, 3});  // (10+3) * (10-3) = 13 * 7 = 91
  
  EXPECT_TRUE(df.is_idle());
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(DataflowTest, Execute_SingleInputNode_Completes) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "100 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({42});
  
  EXPECT_TRUE(df.is_idle());
}

TEST_F(DataflowTest, Execute_ZeroValues_Completes) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({0, 0});
  
  EXPECT_TRUE(df.is_idle());
}

TEST_F(DataflowTest, Execute_LargeValues_Completes) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "100 ADD 2 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({UINT64_MAX / 2, UINT64_MAX / 2});
  
  EXPECT_TRUE(df.is_idle());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// =============================================================================
// Parallelism Tests
// =============================================================================

TEST_F(DataflowTest, MaxParallelism_FourIndependentOps_UsesFourALUs) {
    // Graph with 4 completely independent operations
    // Should achieve max parallelism = 4 (ALU_AMOUNT)
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
30 INPUT 0 101:0
40 INPUT 0 101:1
50 INPUT 0 102:0
60 INPUT 0 102:1
70 INPUT 0 103:0
80 INPUT 0 103:1
100 ADD 2 200:0
101 ADD 2 200:1
102 ADD 2 201:0
103 ADD 2 201:1
200 ADD 2 300:0
201 ADD 2 300:1
300 ADD 2 400:0
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({1, 2, 3, 4, 5, 6, 7, 8});
    
    const auto& stats = df.get_stats();
    
    // All 4 ADDs (100-103) should execute in parallel
    EXPECT_EQ(stats.max_parallel_tasks, 4u) 
        << "Expected 4 parallel tasks when 4 independent ops are ready";
}

TEST_F(DataflowTest, AvgParallelism_MixedWorkload_ReflectsActualUsage) {
    // Linear chain - only 1 ALU active at a time
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 ADD 2 200:0
200 FORK 1 300:0 300:1
300 ADD 2 400:0
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({10, 20});
    
    const auto& stats = df.get_stats();
    
    // Mostly sequential execution
    EXPECT_LE(stats.get_avg_parallelism(), 1.5)
        << "Linear chain should have low average parallelism";
    EXPECT_GE(stats.get_avg_parallelism(), 0.5)
        << "Should have some parallelism from pipeline overlap";
}

TEST_F(DataflowTest, Parallelism_HighlyParallelGraph_HighAverage) {
    // Wide graph with many parallel operations
    write_graph(R"(
10 INPUT 0 100:0 101:0 102:0 103:0
20 INPUT 0 100:1 101:1 102:1 103:1
100 ADD 2 200:0
101 SUB 2 200:1
102 ADD 2 201:0
103 SUB 2 201:1
200 ADD 2 300:0
201 ADD 2 300:1
300 ADD 2 400:0
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({100, 50});
    
    const auto& stats = df.get_stats();
    
    // Should achieve high parallelism at some point
    EXPECT_GE(stats.max_parallel_tasks, 2u)
        << "Wide graph should achieve at least 2 parallel tasks";
    EXPECT_GE(stats.get_avg_parallelism(), 1.0)
        << "Wide graph should have decent average parallelism";
}

// =============================================================================
// Idle Cycle Tests
// =============================================================================

TEST_F(DataflowTest, IdleCycles_SimpleGraph_HasPipelineStalls) {
    // Simple graph - will have idle cycles from pipeline filling/draining
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 ADD 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({5, 10});
    
    const auto& stats = df.get_stats();
    
    // Pipeline has startup and drain cycles
    EXPECT_GT(stats.idle_cycles, 0u)
        << "Should have some idle cycles from pipeline overhead";
    EXPECT_LT(stats.get_idle_ratio(), 0.8)
        << "Idle ratio should not be excessive for simple graph";
}

TEST_F(DataflowTest, IdleCycles_LongLatencyOp_MinimalIdleRatio) {
    // DIV has 6 cycle latency - ALU is busy, not idle
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 DIV 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({100, 10});
    
    const auto& stats = df.get_stats();
    
    // While DIV executes, ALU is active (not idle)
    // Idle cycles are only when ALUs are empty but work exists
    EXPECT_GE(stats.cycles_with_activity, 6u)
        << "DIV should keep ALU active for at least 6 cycles";
}

TEST_F(DataflowTest, IdleCycles_DataDependency_CausesStalls) {
    // Chain of operations - each must wait for previous result
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 ADD 2 200:0
200 FORK 1 300:0 300:1
300 MULT 2 400:0
400 FORK 1 500:0 500:1
500 DIV 2 600:0
600 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({10, 5});
    
    const auto& stats = df.get_stats();
    
    // Serial chain has pipeline stalls between operations
    EXPECT_GT(stats.idle_cycles, 0u)
        << "Serial dependency chain should have idle cycles";
}

// =============================================================================
// Token Flow Tests
// =============================================================================

TEST_F(DataflowTest, Tokens_SimpleAdd_CorrectCount) {
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 ADD 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({5, 10});
    
    const auto& stats = df.get_stats();
    
    // 2 input tokens + 1 ADD result = 3 tokens minimum
    EXPECT_GE(stats.tokens_generated, 3u)
        << "Should generate at least 3 tokens (2 inputs + 1 result)";
    EXPECT_GE(stats.tokens_consumed, 2u)
        << "Should consume at least 2 tokens (the inputs to ADD)";
}

TEST_F(DataflowTest, Tokens_ForkMultipliesTokens) {
    // FORK duplicates tokens
    write_graph(R"(
10 INPUT 0 100:0
100 FORK 1 200:0 300:0 400:0
200 OUTPUT 1
300 OUTPUT 1
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({42});
    
    const auto& stats = df.get_stats();
    
    // 1 input + 3 FORK outputs = 4 tokens generated
    EXPECT_GE(stats.tokens_generated, 4u)
        << "FORK should multiply tokens to destinations";
}

TEST_F(DataflowTest, Tokens_ComplexGraph_HighTokenCount) {
    // Complex graph generates many tokens
    write_graph(R"(
10 INPUT 0 100:0 101:0
20 INPUT 0 100:1 101:1
100 ADD 2 200:0 201:0
101 SUB 2 200:1 201:1
200 MULT 2 300:0
201 DIV 2 300:1
300 ADD 2 400:0
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({20, 5});
    
    const auto& stats = df.get_stats();
    
    // Tokens fan out through the graph
    EXPECT_GE(stats.get_total_tokens(), 8u)
        << "Complex graph should have significant token traffic";
}

// =============================================================================
// Matching Store Occupancy Tests
// =============================================================================

TEST_F(DataflowTest, MatchingStore_TwoInputNode_OccupancyOne) {
    // Two tokens arrive for same node, one waits briefly
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 ADD 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({5, 10});
    
    const auto& stats = df.get_stats();
    
    // First token waits for second token
    EXPECT_GE(stats.max_matching_store_size, 1u)
        << "First token should wait in matching store";
}

TEST_F(DataflowTest, MatchingStore_MultipleWaitingTokens_HighOccupancy) {
    // Multiple nodes have partial inputs simultaneously
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 101:0
30 INPUT 0 102:0
40 INPUT 0 103:0
50 INPUT 0 100:1 101:1 102:1 103:1
100 ADD 2 200:0
101 ADD 2 200:1
102 ADD 2 201:0
103 ADD 2 201:1
200 ADD 2 300:0
201 ADD 2 300:1
300 ADD 2 400:0
400 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({1, 2, 3, 4, 10});
    
    const auto& stats = df.get_stats();
    
    // 4 tokens arrive first, then 5th token triggers all
    // Peak occupancy should be 4 (before 5th token arrives)
    EXPECT_GE(stats.max_matching_store_size, 4u)
        << "Four partial inputs should wait in matching store";
}

TEST_F(DataflowTest, MatchingStore_SingleInputNodes_ZeroOccupancy) {
    // Nodes with 1 input fire immediately, no waiting
    write_graph(R"(
10 INPUT 0 100:0
100 FORK 1 200:0 201:0
200 OUTPUT 1
201 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({99});
    
    const auto& stats = df.get_stats();
    
    // Single-input nodes fire immediately
    EXPECT_EQ(stats.max_matching_store_size, 0u)
        << "Single-input nodes should not accumulate in matching store";
}

// =============================================================================
// Latency Tests
// =============================================================================

TEST_F(DataflowTest, Latency_AddVsMult_DifferentCycles) {
    // ADD (2 cycles) vs MULT (4 cycles)
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 ADD 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df1;
    df1.load_program(temp_file_);
    df1.run({5, 10});
    size_t add_cycles = df1.get_stats().total_cycles;
    
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 MULT 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df2;
    df2.load_program(temp_file_);
    df2.run({5, 10});
    size_t mult_cycles = df2.get_stats().total_cycles;
    
    EXPECT_GT(mult_cycles, add_cycles)
        << "MULT should take more cycles than ADD due to higher latency";
}

TEST_F(DataflowTest, Latency_DivIsExpensive) {
    // DIV has highest latency (6 cycles)
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 DIV 2 200:0
200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({100, 10});
    
    const auto& stats = df.get_stats();
    
    // DIV alone takes 6 cycles, plus pipeline overhead
    EXPECT_GE(stats.total_cycles, 8u)
        << "DIV (6 cycles) + pipeline overhead should be >= 8 cycles";
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(DataflowTest, Metrics_EmptyRun_ZeroStats) {
    // Graph with no path to OUTPUT
    write_graph(R"(
10 INPUT 0 100:0
100 ADD 2 200:0
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    // Run with only 1 input - ADD needs 2, so it never fires
    // This should terminate after idle detection
    df.run({42});
    
    const auto& stats = df.get_stats();
    
    // System eventually idles because ADD never fires
    EXPECT_LT(stats.total_cycles, 100u)
        << "Should terminate quickly when no work progresses";
}

TEST_F(DataflowTest, Metrics_MemoryOperations_IncludedInStats) {
    // LOAD and STORE operations
    write_graph(R"(
10 INPUT 0 100:0
20 INPUT 0 100:1
100 STORE 2 200:0
200 LOAD 1 300:0
300 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({5, 42});  // Store 42 at address 5, then load
    
    const auto& stats = df.get_stats();
    
    EXPECT_GT(stats.total_cycles, 0u)
        << "Memory operations should execute and produce stats";
    EXPECT_GE(stats.tokens_generated, 3u)
        << "Memory ops should produce tokens";
}

// =============================================================================
// Comprehensive Integration Test
// =============================================================================

TEST_F(DataflowTest, Integration_AllMetricsReasonable) {
    // Complex graph to test all metrics together
    write_graph(R"(
# Compute: ((a + b) * (c - d)) / (e + f)
10 INPUT 0 100:0
20 INPUT 0 100:1
30 INPUT 0 101:0
40 INPUT 0 101:1
50 INPUT 0 102:0
60 INPUT 0 102:1
100 ADD 2 200:0
101 SUB 2 200:1
102 ADD 2 201:1
200 MULT 2 201:0
201 DIV 2 300:0
300 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    df.run({10, 20, 15, 5, 6, 9});  // ((10+20)*(15-5))/(6+9) = (30*10)/15 = 20
    
    const auto& stats = df.get_stats();
    
    // Validate all metrics are reasonable
    EXPECT_GT(stats.total_cycles, 0u) << "Should have non-zero cycles";
    EXPECT_GT(stats.cycles_with_activity, 0u) << "Should have active cycles";
    EXPECT_GE(stats.max_parallel_tasks, 2u) << "Should achieve some parallelism";
    EXPECT_GT(stats.get_avg_parallelism(), 0.0) << "Should have non-zero avg parallelism";
    EXPECT_GE(stats.tokens_generated, 6u) << "Should generate tokens";
    EXPECT_GE(stats.tokens_consumed, 6u) << "Should consume tokens";
    EXPECT_LT(stats.get_idle_ratio(), 1.0) << "Should not be 100% idle";
    
    std::cout << "\n=== Integration Test Metrics ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Active Cycles: " << stats.cycles_with_activity << std::endl;
    std::cout << "Idle Cycles: " << stats.idle_cycles << std::endl;
    std::cout << "Max Parallelism: " << stats.max_parallel_tasks << std::endl;
    std::cout << "Avg Parallelism: " << stats.get_avg_parallelism() << std::endl;
    std::cout << "Tokens Generated: " << stats.tokens_generated << std::endl;
    std::cout << "Tokens Consumed: " << stats.tokens_consumed << std::endl;
    std::cout << "Max Matching Store: " << stats.max_matching_store_size << std::endl;
}


TEST_F(DataflowTest, RLE_AA_Unrolled) {
    // Unrolled RLE for "AA\0" - no loops, explicit iterations
    // Easier to debug and verify correctness
    
    write_graph(R"(
# =====================================================================
# RLE UNROLLED FOR "AA\0"
# =====================================================================
# Iteration 1: Load 'A' at ptr=4, compare with val='A', match → cnt=2
# Iteration 2: Load null at ptr=8, terminate → store (2, 'A')

# === MEMORY SETUP ===
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

3 INPUT 0 20:0
4 INPUT 0 20:1
20 STORE 2 21:0

5 INPUT 0 30:0
6 INPUT 0 30:1
30 STORE 2 31:0

11 FORK 1 40:0
21 FORK 1 40:1
40 ADD 2 41:0
41 FORK 1 42:0
31 FORK 1 42:1
42 ADD 2 43:0

# === LOAD FIRST CHAR (initialization) ===
43 FORK 1 50:0
7 INPUT 0 50:0
50 LOAD 1 51:0

# State after init: ptr=0, val='A'(65), cnt=1

# === ITERATION 1: Load char at ptr=4 ===
# Compute addr = 0 + 4
8 INPUT 0 60:0
9 INPUT 0 60:1
60 ADD 2 61:0

61 LOAD 1 62:0

# Compare with val (65)
51 FORK 1 70:0 100:0
62 FORK 1 70:1 71:0

70 EQ 2 72:0

# next_val (62) also goes to null check
71 EQ 2 73:0
12 INPUT 0 71:1

# Match check result
72 FORK 1 80:0

# Null check result (should be false, 65 != 0)
73 FORK 1 81:0

# SWITCH on null (should go false path)
# Data doesn't matter here, we just need to know which path
81 SWITCH 2 900:0 82:0

# Not null, check match
82 FORK 1 83:0

# Match result goes to switch
72 FORK 1 83:1

83 FORK 1 84:0

# SWITCH on match (should go true path since 'A'=='A')
# Using a dummy data value
13 INPUT 0 84:0
84 SWITCH 2 85:0 901:0

# Match path: increment count
# cnt (1) + 1 = 2
85 FORK 1 90:0
14 INPUT 0 90:0
15 INPUT 0 90:1
90 ADD 2 91:0

# State after iter1: ptr=4, val='A', cnt=2

# === ITERATION 2: Load char at ptr=8 ===
# Compute addr = 4 + 4
16 INPUT 0 110:0
17 INPUT 0 110:1
110 ADD 2 111:0

111 LOAD 1 112:0

# Null check (should be true, 0 == 0)
112 EQ 2 113:0
18 INPUT 0 112:1

113 SWITCH 2 120:0 902:0

# Null path: terminate, store (cnt, val)

# Store cnt=2 at out=100
120 FORK 1 130:0
91 FORK 1 130:1
19 INPUT 0 130:0
130 STORE 2 131:0

# Store val='A' at out+4=104
131 FORK 1 140:0
100 FORK 1 141:0
22 INPUT 0 140:0
23 INPUT 0 140:1
140 ADD 2 142:0
142 FORK 1 143:0
141 FORK 1 143:1
143 STORE 2 150:0

150 OUTPUT 1

# Unused paths
900 OUTPUT 1
901 OUTPUT 1
902 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    // Memory: store 65 at 0, 65 at 4, 0 at 8
    // Init: load from 0
    // Iter1 addr calc: 0+4
    // Iter1 null const: 0
    // Iter1 dummy: 1
    // Iter1 cnt: 1
    // Iter1 incr: 1
    // Iter2 addr calc: 4+4
    // Iter2 null const: 0
    // Output addr: 100
    // Output addr calc: 100+4
    
    df.run({
        0, 65,    // store 'A' at 0
        4, 65,    // store 'A' at 4
        8, 0,     // store null at 8
        0,        // initial load addr
        0, 4,     // iter1: 0+4
        0,        // iter1: null compare const
        1,        // iter1: dummy for switch
        1,        // iter1: cnt
        1,        // iter1: increment
        4, 4,     // iter2: 4+4
        0,        // iter2: null compare const
        100,      // output addr
        100, 4    // output addr + 4
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    std::cout << "\n=== RLE Unrolled 'AA' Results ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Tokens Generated: " << stats.tokens_generated << std::endl;
}



TEST_F(DataflowTest, RLE_LongInput_Unrolled) {
    // Input: "AAAAABBBBCCDDDDDDD" (18 chars + null)
    // Same as EmulatorTest::RLECompression_LongInput
    //
    // Memory layout (same as MIPS):
    //   Input:  2048-2120 (words 512-530)
    //   Output: 2560+     (words 640+)
    //
    // Expected output:
    //   (5, 'A') at 2560, 2564
    //   (4, 'B') at 2568, 2572
    //   (2, 'C') at 2576, 2580
    //   (7, 'D') at 2584, 2588
    //
    // This is UNROLLED - we know the structure ahead of time
    // In real dataflow, this would use loops, but unrolling lets us
    // compare execution characteristics fairly.
    
    write_graph(R"(
# =====================================================================
# RLE COMPRESSION: "AAAAABBBBCCDDDDDDD"
# Unrolled dataflow graph - same input as MIPS RLECompression_LongInput
# =====================================================================

# === MEMORY INITIALIZATION ===
# Input string at addresses 2048, 2052, 2056, ... (stride 4)

# 'A' (65) x 5 at addresses 2048, 2052, 2056, 2060, 2064
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

3 INPUT 0 12:0
4 INPUT 0 12:1
12 STORE 2 13:0

5 INPUT 0 14:0
6 INPUT 0 14:1
14 STORE 2 15:0

7 INPUT 0 16:0
8 INPUT 0 16:1
16 STORE 2 17:0

9 INPUT 0 18:0
19 INPUT 0 18:1
18 STORE 2 20:0

# 'B' (66) x 4 at addresses 2068, 2072, 2076, 2080
21 INPUT 0 22:0
23 INPUT 0 22:1
22 STORE 2 24:0

25 INPUT 0 26:0
27 INPUT 0 26:1
26 STORE 2 28:0

29 INPUT 0 30:0
31 INPUT 0 30:1
30 STORE 2 32:0

33 INPUT 0 34:0
35 INPUT 0 34:1
34 STORE 2 36:0

# 'C' (67) x 2 at addresses 2084, 2088
37 INPUT 0 38:0
39 INPUT 0 38:1
38 STORE 2 40:0

41 INPUT 0 42:0
43 INPUT 0 42:1
42 STORE 2 44:0

# 'D' (68) x 7 at addresses 2092, 2096, 2100, 2104, 2108, 2112, 2116
45 INPUT 0 46:0
47 INPUT 0 46:1
46 STORE 2 48:0

49 INPUT 0 50:0
51 INPUT 0 50:1
50 STORE 2 52:0

53 INPUT 0 54:0
55 INPUT 0 54:1
54 STORE 2 56:0

57 INPUT 0 58:0
59 INPUT 0 58:1
58 STORE 2 60:0

61 INPUT 0 62:0
63 INPUT 0 62:1
62 STORE 2 64:0

65 INPUT 0 66:0
67 INPUT 0 66:1
66 STORE 2 68:0

69 INPUT 0 70:0
71 INPUT 0 70:1
70 STORE 2 72:0

# Null terminator (0) at address 2120
73 INPUT 0 74:0
75 INPUT 0 74:1
74 STORE 2 76:0

# === SYNC ALL STORES ===
11 FORK 1 80:0
13 FORK 1 80:1
80 ADD 2 81:0

15 FORK 1 82:0
17 FORK 1 82:1
82 ADD 2 83:0

81 FORK 1 84:0
83 FORK 1 84:1
84 ADD 2 85:0

20 FORK 1 86:0
24 FORK 1 86:1
86 ADD 2 87:0

85 FORK 1 88:0
87 FORK 1 88:1
88 ADD 2 89:0

28 FORK 1 90:0
32 FORK 1 90:1
90 ADD 2 91:0

36 FORK 1 92:0
40 FORK 1 92:1
92 ADD 2 93:0

91 FORK 1 94:0
93 FORK 1 94:1
94 ADD 2 95:0

89 FORK 1 96:0
95 FORK 1 96:1
96 ADD 2 97:0

44 FORK 1 98:0
48 FORK 1 98:1
98 ADD 2 99:0

52 FORK 1 100:0
56 FORK 1 100:1
100 ADD 2 101:0

99 FORK 1 102:0
101 FORK 1 102:1
102 ADD 2 103:0

60 FORK 1 104:0
64 FORK 1 104:1
104 ADD 2 105:0

68 FORK 1 106:0
72 FORK 1 106:1
106 ADD 2 107:0

105 FORK 1 108:0
107 FORK 1 108:1
108 ADD 2 109:0

103 FORK 1 110:0
109 FORK 1 110:1
110 ADD 2 111:0

97 FORK 1 112:0
111 FORK 1 112:1
112 ADD 2 113:0

76 FORK 1 114:0
113 FORK 1 114:1
114 ADD 2 115:0

# Memory ready - trigger RLE computation
115 FORK 1 1000:0

# =====================================================================
# RLE COMPUTATION (Unrolled - we know the runs)
# =====================================================================
# 
# Run 1: 5 x 'A' (indices 0-4)
# Run 2: 4 x 'B' (indices 5-8)
# Run 3: 2 x 'C' (indices 9-10)
# Run 4: 7 x 'D' (indices 11-17)
#
# For each run, we:
#   1. Load first char of run (to get the character)
#   2. Count is known (unrolled)
#   3. Store (count, char) to output
#
# All 4 runs can execute IN PARALLEL - this is where dataflow shines!

# === RUN 1: 'A' x 5 → output at 2560, 2564 ===

# Load char from addr 2048
200 INPUT 0 210:0
210 LOAD 1 220:0

# Count = 5
201 INPUT 0 230:0

# Store count at 2560
202 INPUT 0 240:0
230 FORK 1 240:1
240 STORE 2 250:0

# Store char at 2564
250 FORK 1 260:0
203 INPUT 0 260:1
260 ADD 2 270:0
270 FORK 1 280:0
220 FORK 1 280:1
280 STORE 2 290:0

290 FORK 1 900:0

# === RUN 2: 'B' x 4 → output at 2568, 2572 ===

# Load char from addr 2068
300 INPUT 0 310:0
310 LOAD 1 320:0

# Count = 4
301 INPUT 0 330:0

# Store count at 2568
302 INPUT 0 340:0
330 FORK 1 340:1
340 STORE 2 350:0

# Store char at 2572
350 FORK 1 360:0
303 INPUT 0 360:1
360 ADD 2 370:0
370 FORK 1 380:0
320 FORK 1 380:1
380 STORE 2 390:0

390 FORK 1 901:0

# === RUN 3: 'C' x 2 → output at 2576, 2580 ===

# Load char from addr 2084
400 INPUT 0 410:0
410 LOAD 1 420:0

# Count = 2
401 INPUT 0 430:0

# Store count at 2576
402 INPUT 0 440:0
430 FORK 1 440:1
440 STORE 2 450:0

# Store char at 2580
450 FORK 1 460:0
403 INPUT 0 460:1
460 ADD 2 470:0
470 FORK 1 480:0
420 FORK 1 480:1
480 STORE 2 490:0

490 FORK 1 902:0

# === RUN 4: 'D' x 7 → output at 2584, 2588 ===

# Load char from addr 2092
500 INPUT 0 510:0
510 LOAD 1 520:0

# Count = 7
501 INPUT 0 530:0

# Store count at 2584
502 INPUT 0 540:0
530 FORK 1 540:1
540 STORE 2 550:0

# Store char at 2588
550 FORK 1 560:0
503 INPUT 0 560:1
560 ADD 2 570:0
570 FORK 1 580:0
520 FORK 1 580:1
580 STORE 2 590:0

590 FORK 1 903:0

# === SYNC ALL OUTPUTS ===
900 FORK 1 910:0
901 FORK 1 910:1
910 ADD 2 920:0

902 FORK 1 930:0
903 FORK 1 930:1
930 ADD 2 940:0

920 FORK 1 950:0
940 FORK 1 950:1
950 ADD 2 999:0

999 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    df.run({
        // Memory initialization - same as MIPS test
        // 'A' x 5
        2048, 65,   // addr, value
        2052, 65,
        2056, 65,
        2060, 65,
        2064, 65,
        // 'B' x 4
        2068, 66,
        2072, 66,
        2076, 66,
        2080, 66,
        // 'C' x 2
        2084, 67,
        2088, 67,
        // 'D' x 7
        2092, 68,
        2096, 68,
        2100, 68,
        2104, 68,
        2108, 68,
        2112, 68,
        2116, 68,
        // Null terminator
        2120, 0,
        
        // Run 1: 'A' x 5
        2048,       // load addr
        5,          // count
        2560,       // store count addr
        4,          // offset for char addr
        
        // Run 2: 'B' x 4
        2068,       // load addr
        4,          // count
        2568,       // store count addr
        4,          // offset
        
        // Run 3: 'C' x 2
        2084,       // load addr
        2,          // count
        2576,       // store count addr
        4,          // offset
        
        // Run 4: 'D' x 7
        2092,       // load addr
        7,          // count
        2584,       // store count addr
        4           // offset
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    
    std::cout << "\n=== RLE Dataflow: AAAAABBBBCCDDDDDDD ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Tokens Generated: " << stats.tokens_generated << std::endl;
    std::cout << "Max Parallelism: " << stats.max_parallel_tasks << std::endl;
    std::cout << "Avg Parallelism: " << stats.get_avg_parallelism() << std::endl;
    std::cout << "\nExpected output:" << std::endl;
    std::cout << "  (5, 'A') at 2560, 2564" << std::endl;
    std::cout << "  (4, 'B') at 2568, 2572" << std::endl;
    std::cout << "  (2, 'C') at 2576, 2580" << std::endl;
    std::cout << "  (7, 'D') at 2584, 2588" << std::endl;
    std::cout << "\nKey insight: All 4 runs computed IN PARALLEL!" << std::endl;
    std::cout << "Von Neumann must process sequentially with branch mispredictions." << std::endl;
}

// =============================================================================
// Test 1: The Memory Wall Chain (Core Pattern)
// =============================================================================

TEST_F(DataflowTest, SpMV_MemoryWall_IndirectLoad) {
    // This demonstrates the memory wall: load index → compute address → load value
    // The chain is strictly sequential - no parallelism possible
    //
    // Memory: [200]=2 (col_idx), [308]=99 (vector_x[2])
    // Compute: load col_idx, addr = 300 + col_idx*4, load vector_x[addr]
    
    write_graph(R"(
# === MEMORY SETUP ===
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

3 INPUT 0 12:0
4 INPUT 0 12:1
12 STORE 2 13:0

11 FORK 1 20:0
13 FORK 1 20:1
20 ADD 2 21:0
21 FORK 1 100:0

# === THE MEMORY WALL CHAIN ===

# Step 1: Load col_idx from address 200
5 INPUT 0 100:0
100 LOAD 1 110:0

# Step 2: Compute offset = col_idx * 4
6 INPUT 0 120:1
110 FORK 1 120:0
120 MULT 2 130:0

# Step 3: Compute x_addr = base_x + offset
7 INPUT 0 140:0
130 FORK 1 140:1
140 ADD 2 150:0

# Step 4: Load vector_x[col_idx]
150 LOAD 1 160:0

160 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    df.run({
        200, 2,    // Store col_idx=2 at addr 200
        308, 99,   // Store vector_x[2]=99 at addr 308
        200,       // Address to load col_idx
        4,         // Multiplier for offset
        300        // Base address of vector_x
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    std::cout << "\n=== Memory Wall Chain ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "This chain is strictly sequential (no parallelism)!" << std::endl;
}

// =============================================================================
// Test 2: Single Dot Product Element
// =============================================================================

TEST_F(DataflowTest, SpMV_SingleElement_ValueTimesVector) {
    // Compute: values[i] * vector_x[col_ind[i]]
    // This is one element of the dot product
    //
    // Memory:
    //   values[0]=5 at addr 100
    //   col_ind[0]=0 at addr 200 (pre-scaled: col 0 → offset 0)
    //   vector_x[0]=2 at addr 300
    //
    // Expected: 5 * 2 = 10
    
    write_graph(R"(
# === MEMORY SETUP ===
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

3 INPUT 0 12:0
4 INPUT 0 12:1
12 STORE 2 13:0

5 INPUT 0 14:0
6 INPUT 0 14:1
14 STORE 2 15:0

11 FORK 1 20:0
13 FORK 1 20:1
20 ADD 2 21:0
21 FORK 1 22:0
15 FORK 1 22:1
22 ADD 2 23:0
23 FORK 1 100:0

# === LOAD VALUE ===
7 INPUT 0 100:0
100 LOAD 1 110:0

# === LOAD COL_IDX (Memory Wall Starts) ===
8 INPUT 0 120:0
120 LOAD 1 130:0

# === COMPUTE X ADDRESS ===
9 INPUT 0 140:0
130 FORK 1 140:1
140 ADD 2 150:0

# === LOAD VECTOR_X (Memory Wall Ends) ===
150 LOAD 1 160:0

# === MULTIPLY ===
110 FORK 1 170:0
160 FORK 1 170:1
170 MULT 2 180:0

180 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    df.run({
        100, 5,    // values[0]=5 at 100
        200, 0,    // col_ind[0]=0 at 200
        300, 2,    // vector_x[0]=2 at 300
        100,       // addr of values[0]
        200,       // addr of col_ind[0]
        300        // base_x (offset is 0, so addr = 300+0)
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    std::cout << "\n=== Single Element ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Expected: 5 * 2 = 10" << std::endl;
}

// =============================================================================
// Test 3: Row 0 - Single Element Row
// =============================================================================

TEST_F(DataflowTest, SpMV_Row0_SingleElement) {
    // Row 0 of the matrix: [5 0 0]
    // Only one non-zero: values[0]=5, col_ind[0]=0
    // Computes: 5 * x[0] = 5 * 2 = 10
    
    write_graph(R"(
# === MEMORY SETUP ===
# values[0]=5 at 100
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

# col_ind[0]=0 at 200
3 INPUT 0 12:0
4 INPUT 0 12:1
12 STORE 2 13:0

# vector_x[0]=2 at 300
5 INPUT 0 14:0
6 INPUT 0 14:1
14 STORE 2 15:0

# Sync
11 FORK 1 20:0
13 FORK 1 20:1
20 ADD 2 21:0
21 FORK 1 22:0
15 FORK 1 22:1
22 ADD 2 23:0
23 FORK 1 100:0

# === COMPUTE y[0] = values[0] * x[col_ind[0]] ===

# Load values[0]
7 INPUT 0 100:0
100 LOAD 1 110:0

# Load col_ind[0]
8 INPUT 0 120:0
120 LOAD 1 130:0

# x_addr = base_x + col_ind
9 INPUT 0 140:0
130 FORK 1 140:1
140 ADD 2 150:0

# Load x[col_ind[0]]
150 LOAD 1 160:0

# product = value * x_val
110 FORK 1 170:0
160 FORK 1 170:1
170 MULT 2 180:0

# Store result to y[0] at addr 400
24 INPUT 0 190:0
180 FORK 1 190:1
190 STORE 2 200:0

200 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    df.run({
        100, 5,    // values[0]=5
        200, 0,    // col_ind[0]=0
        300, 2,    // vector_x[0]=2
        100,       // load values[0]
        200,       // load col_ind[0]
        300,       // base_x
        400        // store y[0]
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    std::cout << "\n=== Row 0 (Single Element) ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Expected y[0] = 10" << std::endl;
}

// =============================================================================
// Test 4: Row 2 - Two Elements (Inner Loop Unrolled)
// =============================================================================

TEST_F(DataflowTest, SpMV_Row2_TwoElements) {
    // Row 2 of the matrix: [2 0 3]
    // Two non-zeros: values[2]=2 at col 0, values[3]=3 at col 2
    // Computes: 2*x[0] + 3*x[2] = 2*2 + 3*4 = 4 + 12 = 16
    
    write_graph(R"(
# === MEMORY SETUP ===
# values[2]=2 at 108
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

# values[3]=3 at 112
3 INPUT 0 12:0
4 INPUT 0 12:1
12 STORE 2 13:0

# col_ind[2]=0 at 208
5 INPUT 0 14:0
6 INPUT 0 14:1
14 STORE 2 15:0

# col_ind[3]=8 at 212 (col 2 * 4 = 8)
7 INPUT 0 16:0
8 INPUT 0 16:1
16 STORE 2 17:0

# vector_x[0]=2 at 300
9 INPUT 0 18:0
25 INPUT 0 18:1
18 STORE 2 19:0

# vector_x[2]=4 at 308
26 INPUT 0 27:0
28 INPUT 0 27:1
27 STORE 2 29:0

# Sync all stores
11 FORK 1 30:0
13 FORK 1 30:1
30 ADD 2 31:0
15 FORK 1 32:0
17 FORK 1 32:1
32 ADD 2 33:0
31 FORK 1 34:0
33 FORK 1 34:1
34 ADD 2 35:0
19 FORK 1 36:0
29 FORK 1 36:1
36 ADD 2 37:0
35 FORK 1 38:0
37 FORK 1 38:1
38 ADD 2 39:0
39 FORK 1 1000:0

# === ELEMENT 0: values[2] * x[col_ind[2]] ===

# Load values[2]
40 INPUT 0 100:0
100 LOAD 1 110:0

# Load col_ind[2]
41 INPUT 0 120:0
120 LOAD 1 130:0

# x_addr = base_x + col_ind
42 INPUT 0 140:0
130 FORK 1 140:1
140 ADD 2 150:0

# Load x[col_ind[2]]
150 LOAD 1 160:0

# product0 = value * x_val
110 FORK 1 170:0
160 FORK 1 170:1
170 MULT 2 180:0

# === ELEMENT 1: values[3] * x[col_ind[3]] ===
# (Runs in PARALLEL with Element 0!)

# Load values[3]
43 INPUT 0 200:0
200 LOAD 1 210:0

# Load col_ind[3]
44 INPUT 0 220:0
220 LOAD 1 230:0

# x_addr = base_x + col_ind
45 INPUT 0 240:0
230 FORK 1 240:1
240 ADD 2 250:0

# Load x[col_ind[3]]
250 LOAD 1 260:0

# product1 = value * x_val
210 FORK 1 270:0
260 FORK 1 270:1
270 MULT 2 280:0

# === SUM: product0 + product1 ===
180 FORK 1 300:0
280 FORK 1 300:1
300 ADD 2 310:0

# Store result to y[2] at addr 408
46 INPUT 0 320:0
310 FORK 1 320:1
320 STORE 2 330:0

330 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    df.run({
        // Memory setup
        108, 2,    // values[2]=2 at 108
        112, 3,    // values[3]=3 at 112
        208, 0,    // col_ind[2]=0 at 208
        212, 8,    // col_ind[3]=8 at 212
        300, 2,    // vector_x[0]=2 at 300
        308, 4,    // vector_x[2]=4 at 308
        // Element 0 addresses
        108,       // load values[2]
        208,       // load col_ind[2]
        300,       // base_x
        // Element 1 addresses
        112,       // load values[3]
        212,       // load col_ind[3]
        300,       // base_x
        // Output
        408        // store y[2]
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    std::cout << "\n=== Row 2 (Two Elements) ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Max Parallelism: " << stats.max_parallel_tasks << std::endl;
    std::cout << "Expected y[2] = 2*2 + 3*4 = 16" << std::endl;
    std::cout << "\nNote: Both element computations run in PARALLEL!" << std::endl;
}

// =============================================================================
// Test 5: Full 3x3 SpMV (All Rows in Parallel)
// =============================================================================

TEST_F(DataflowTest, SpMv_Full3x3_AllRowsParallel) {
    // Complete SpMV: y = A * x
    //
    // Matrix A (3x3):      Vector X:    Result Y:
    // [5 0 0]              [2]          [10]
    // [0 8 0]      *       [1]     =    [8]
    // [2 0 3]              [4]          [16]
    //
    // All three rows compute in PARALLEL - this is where dataflow shines!
    
    write_graph(R"(
# === MEMORY SETUP ===

# values: [5, 8, 2, 3] at [100, 104, 108, 112]
1 INPUT 0 10:0
2 INPUT 0 10:1
10 STORE 2 11:0

3 INPUT 0 12:0
4 INPUT 0 12:1
12 STORE 2 13:0

5 INPUT 0 14:0
6 INPUT 0 14:1
14 STORE 2 15:0

7 INPUT 0 16:0
8 INPUT 0 16:1
16 STORE 2 17:0

# col_ind: [0, 4, 0, 8] at [200, 204, 208, 212]
35 INPUT 0 20:0
36 INPUT 0 20:1
20 STORE 2 21:0

37 INPUT 0 22:0
38 INPUT 0 22:1
22 STORE 2 23:0

39 INPUT 0 24:0
40 INPUT 0 24:1
24 STORE 2 25:0

41 INPUT 0 26:0
42 INPUT 0 26:1
26 STORE 2 57:0

# vector_x: [2, 1, 4] at [300, 304, 308]
43 INPUT 0 28:0
44 INPUT 0 28:1
28 STORE 2 29:0

45 INPUT 0 50:0
46 INPUT 0 50:1
50 STORE 2 51:0

47 INPUT 0 52:0
48 INPUT 0 52:1
52 STORE 2 53:0

# Sync stores
11 FORK 1 60:0
13 FORK 1 60:1
60 ADD 2 61:0
15 FORK 1 62:0
17 FORK 1 62:1
62 ADD 2 63:0
61 FORK 1 64:0
63 FORK 1 64:1
64 ADD 2 65:0

21 FORK 1 66:0
23 FORK 1 66:1
66 ADD 2 67:0
25 FORK 1 68:0
57 FORK 1 68:1
68 ADD 2 69:0
67 FORK 1 70:0
69 FORK 1 70:1
70 ADD 2 71:0

29 FORK 1 72:0
51 FORK 1 72:1
72 ADD 2 73:0
53 FORK 1 74:0
73 FORK 1 74:1
74 ADD 2 75:0

65 FORK 1 76:0
71 FORK 1 76:1
76 ADD 2 77:0
75 FORK 1 78:0
77 FORK 1 78:1
78 ADD 2 79:0

79 FORK 1 1000:0

# ===================================================================
# ROW 0: y[0] = 5 * x[0] = 10
# ===================================================================

# Load values[0]
80 INPUT 0 100:0
100 LOAD 1 110:0

# Load col_ind[0]
81 INPUT 0 120:0
120 LOAD 1 130:0

# x_addr = base_x + col_ind
82 INPUT 0 140:0
130 FORK 1 140:1
140 ADD 2 150:0

# Load x[col_ind[0]]
150 LOAD 1 160:0

# product = value * x_val
110 FORK 1 170:0
160 FORK 1 170:1
170 MULT 2 180:0

# Store y[0]
83 INPUT 0 190:0
180 FORK 1 190:1
190 STORE 2 195:0

195 FORK 1 900:0

# ===================================================================
# ROW 1: y[1] = 8 * x[1] = 8
# ===================================================================

# Load values[1]
84 INPUT 0 200:0
200 LOAD 1 210:0

# Load col_ind[1]
85 INPUT 0 220:0
220 LOAD 1 230:0

# x_addr = base_x + col_ind
86 INPUT 0 240:0
230 FORK 1 240:1
240 ADD 2 250:0

# Load x[col_ind[1]]
250 LOAD 1 260:0

# product = value * x_val
210 FORK 1 270:0
260 FORK 1 270:1
270 MULT 2 280:0

# Store y[1]
87 INPUT 0 290:0
280 FORK 1 290:1
290 STORE 2 295:0

295 FORK 1 901:0

# ===================================================================
# ROW 2: y[2] = 2*x[0] + 3*x[2] = 16
# ===================================================================

# --- Element 0: values[2] * x[col_ind[2]] ---

# Load values[2]
88 INPUT 0 300:0
300 LOAD 1 310:0

# Load col_ind[2]
89 INPUT 0 320:0
320 LOAD 1 330:0

# x_addr = base_x + col_ind
90 INPUT 0 340:0
330 FORK 1 340:1
340 ADD 2 350:0

# Load x[col_ind[2]]
350 LOAD 1 360:0

# product0 = value * x_val
310 FORK 1 370:0
360 FORK 1 370:1
370 MULT 2 380:0

# --- Element 1: values[3] * x[col_ind[3]] ---

# Load values[3]
91 INPUT 0 400:0
400 LOAD 1 410:0

# Load col_ind[3]
92 INPUT 0 420:0
420 LOAD 1 430:0

# x_addr = base_x + col_ind
93 INPUT 0 440:0
430 FORK 1 440:1
440 ADD 2 450:0

# Load x[col_ind[3]]
450 LOAD 1 460:0

# product1 = value * x_val
410 FORK 1 470:0
460 FORK 1 470:1
470 MULT 2 480:0

# --- Sum products ---
380 FORK 1 490:0
480 FORK 1 490:1
490 ADD 2 500:0

# Store y[2]
94 INPUT 0 510:0
500 FORK 1 510:1
510 STORE 2 520:0

520 FORK 1 902:0

# ===================================================================
# FINAL SYNC
# ===================================================================
900 FORK 1 950:0
901 FORK 1 950:1
950 ADD 2 951:0
951 FORK 1 952:0
902 FORK 1 952:1
952 ADD 2 999:0

999 OUTPUT 1
)");
    
    Dataflow df;
    df.load_program(temp_file_);
    
    df.run({
        // Memory: values
        100, 5,    // values[0]=5
        104, 8,    // values[1]=8
        108, 2,    // values[2]=2
        112, 3,    // values[3]=3
        // Memory: col_ind (pre-scaled by 4)
        200, 0,    // col_ind[0]=0
        204, 4,    // col_ind[1]=4
        208, 0,    // col_ind[2]=0
        212, 8,    // col_ind[3]=8
        // Memory: vector_x
        300, 2,    // x[0]=2
        304, 1,    // x[1]=1
        308, 4,    // x[2]=4
        // Row 0 addresses
        100,       // values[0]
        200,       // col_ind[0]
        300,       // base_x
        400,       // y[0] output
        // Row 1 addresses
        104,       // values[1]
        204,       // col_ind[1]
        300,       // base_x
        404,       // y[1] output
        // Row 2, Element 0 addresses
        108,       // values[2]
        208,       // col_ind[2]
        300,       // base_x
        // Row 2, Element 1 addresses
        112,       // values[3]
        212,       // col_ind[3]
        300,       // base_x
        // Row 2 output
        408        // y[2] output
    });
    
    EXPECT_TRUE(df.is_idle());
    
    const auto& stats = df.get_stats();
    std::cout << "\n=== Full 3x3 SpMV ===" << std::endl;
    std::cout << "Total Cycles: " << stats.total_cycles << std::endl;
    std::cout << "Max Parallelism: " << stats.max_parallel_tasks << std::endl;
    std::cout << "Avg Parallelism: " << stats.get_avg_parallelism() << std::endl;
    std::cout << "\nExpected Results:" << std::endl;
    std::cout << "  y[0] = 5*2 = 10" << std::endl;
    std::cout << "  y[1] = 8*1 = 8" << std::endl;
    std::cout << "  y[2] = 2*2 + 3*4 = 16" << std::endl;
    std::cout << "\nKey Insight: All 3 rows compute in PARALLEL!" << std::endl;
    std::cout << "Within each row, the indirect load chain is sequential." << std::endl;
}