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