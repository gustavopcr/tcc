#ifndef EXECUTE_STAGE_HPP
#define EXECUTE_STAGE_HPP

#include "reservation_station.hpp"
#include "alu.hpp"
#include "rob.hpp"
#include "memory.hpp"
#include <vector>

struct CdbMessage {
    uint8_t tag;
    uint32_t result;
};

struct ExecutionUnit {
    bool is_busy = false;
    int remaining_cycles = 0;
    ReservationStationEntry current_instruction; 
    uint32_t address = 0;
};

struct ExecutionConfig
{
  int alu_cycles;
  int mem_unit_cycles;
  size_t alu_amount;
  size_t mem_unit_amount;
};

class ExecutionStage{
public:
  explicit ExecutionStage(ExecutionConfig ex_config, ReservationStation& reservation_station, ReorderBuffer& rob, Memory& Memory);

  void tick();
private:
  void issue_ready_instructions();
  void process_completed_instructions();
  
  const ExecutionConfig ex_config_;
  std::vector<ExecutionUnit> alus_;
  std::vector<ExecutionUnit> mem_units_;
  ReservationStation& reservation_station_;
  ReorderBuffer& rob_;
  Memory& memory_;
};
#endif