#ifndef PIPELINE_STAGES_HPP
#define PIPELINE_STAGES_HPP

#include "von_neumann/uop.hpp"
#include "von_neumann/memory.hpp"
#include "von_neumann/register.hpp"
#include "von_neumann/rat.hpp"
#include "von_neumann/rob.hpp"
#include "von_neumann/reservation_station.hpp"
#include "von_neumann/execution_unit.hpp"
#include <vector>

class FetchStage {
public:
    FetchStage(uint32_t& pc, Memory& imem, FetchBuffer& out_buffer);
    void tick();
    void stall();
    void unstall();

private:
    uint32_t& pc_;
    Memory& imem_;
    FetchBuffer& out_buffer_;
    bool stall_ = false;
};

class DecodeStage {
public:
    DecodeStage(FetchBuffer& in_buffer, DecodeBuffer& out_buffer);
    void tick(uint32_t current_pc);

private:
    FetchBuffer& in_buffer_;
    DecodeBuffer& out_buffer_;
};

class DispatchStage {
public:
    DispatchStage(DecodeBuffer& in_buffer,
                  RegisterAliasTable& rat,
                  ReorderBuffer& rob,
                  ReservationStation& rs,
                  const RegisterBank& arf);
    void tick();

private:
    Operand rename_source(uint8_t arch_reg);
    
    DecodeBuffer& in_buffer_;
    RegisterAliasTable& rat_;
    ReorderBuffer& rob_;
    ReservationStation& rs_;
    const RegisterBank& arf_;
};

class IssueStage {
public:
    IssueStage(ReservationStation& rs, std::vector<ExecutionUnit>& exec_units);
    void tick();

private:
    ReservationStation& rs_;
    std::vector<ExecutionUnit>& exec_units_;
};

class WritebackStage {
public:
    WritebackStage(std::vector<ExecutionUnit>& exec_units,
                   ReservationStation& rs,
                   ReorderBuffer& rob);
    void tick();

private:
    std::vector<ExecutionUnit>& exec_units_;
    ReservationStation& rs_;
    ReorderBuffer& rob_;
};

class CommitStage {
public:
    CommitStage(ReorderBuffer& rob,
                RegisterAliasTable& rat,
                RegisterBank& arf,
                Memory& dmem);
    void tick();

private:
    ReorderBuffer& rob_;
    RegisterAliasTable& rat_;
    RegisterBank& arf_;
    Memory& dmem_;
};

#endif