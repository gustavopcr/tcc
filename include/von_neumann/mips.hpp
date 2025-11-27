#ifndef MIPS_HPP
#define MIPS_HPP

#include "von_neumann/memory.hpp"
#include "von_neumann/register.hpp"
#include "von_neumann/rat.hpp"
#include "von_neumann/rob.hpp"
#include "von_neumann/reservation_station.hpp"
#include "von_neumann/execution_unit.hpp"
#include "von_neumann/pipeline_stages.hpp"
#include <array>
#include <memory>
#include <vector>

struct MipsConfig {
    size_t num_alus = 2;
    size_t num_mem_units = 1;
    int alu_latency = 1;
    int mem_latency = 3;
};

class Mips {
public:
    explicit Mips(std::array<uint32_t, 4096> program, MipsConfig config = {});
    
    void tick();
    void run(size_t max_cycles = 100000);
    
    const RegisterBank& get_registers() const;
    uint32_t get_pc() const;
    size_t get_cycle_count() const;
    bool is_halted() const;

private:
    uint32_t pc_ = 0;
    RegisterBank arf_{};
    Memory imem_;
    Memory dmem_;
    
    RegisterAliasTable rat_;
    ReorderBuffer rob_;
    ReservationStation alu_rs_;
    ReservationStation mem_rs_;
    
    std::vector<ExecutionUnit> alu_units_;
    std::vector<ExecutionUnit> mem_units_;
    
    FetchBuffer fetch_buffer_;
    DecodeBuffer decode_buffer_;
    
    std::unique_ptr<FetchStage> fetch_stage_;
    std::unique_ptr<DecodeStage> decode_stage_;
    std::unique_ptr<DispatchStage> dispatch_stage_;
    std::unique_ptr<IssueStage> alu_issue_stage_;
    std::unique_ptr<IssueStage> mem_issue_stage_;
    std::unique_ptr<WritebackStage> alu_wb_stage_;
    std::unique_ptr<WritebackStage> mem_wb_stage_;
    std::unique_ptr<CommitStage> commit_stage_;
    
    size_t cycle_count_ = 0;
    bool halted_ = false;
};

#endif