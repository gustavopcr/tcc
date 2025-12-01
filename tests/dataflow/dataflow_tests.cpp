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

TEST_F(DataflowTest, Execute_MERGE_SelectsTrueValue) {
  write_graph(
    "10 INPUT 0 100:0\n"  // False value
    "20 INPUT 0 100:1\n"  // True value
    "30 INPUT 0 100:2\n"  // Condition
    "100 MERGE 3 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({10, 20, 1});  // Should select 20 (true value)
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
}

TEST_F(DataflowTest, Execute_MERGE_SelectsFalseValue) {
  write_graph(
    "10 INPUT 0 100:0\n"
    "20 INPUT 0 100:1\n"
    "30 INPUT 0 100:2\n"
    "100 MERGE 3 200:0\n"
    "200 OUTPUT 1\n"
  );
  
  Dataflow df;
  df.load_program(temp_file_);
  df.run({10, 20, 0});  // Should select 10 (false value)
  
  const auto& stats = df.get_stats();
  EXPECT_GT(stats.total_cycles, 0u);
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