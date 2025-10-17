#include "execute_stage.hpp"

ExecutionStage::ExecutionStage(ExecutionConfig ex_config, ReservationStation& reservation_station)
  : ex_config_{ex_config}
  , alus_(ex_config.alu_amount)
  , mem_units_(ex_config.mem_unit_amount)
  , reservation_station_{reservation_station}
  {
  };


void ExecutionStage::tick()
{
  for(auto& alu : alus_)
  {
    --alu.remaining_cycles;
    if(alu.remaining_cycles == 0)
    {
      // be done with instruction
      // maybe add to writer queue?
      alu.is_busy = false;
      alu.current_instruction = {};
    }
  }

  for(auto& mem_unit : mem_units_)
  {
    --mem_unit.remaining_cycles;
    if(mem_unit.remaining_cycles == 0)
    {
      // be done with instruction
      // maybe add to writer queue?
      mem_unit.is_busy = false;
      mem_unit.current_instruction = {};
    }
  }
}