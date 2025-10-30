#include "von_neumann/execute_stage.hpp"

ExecutionStage::ExecutionStage(ExecutionConfig ex_config, ReservationStation& reservation_station, ReorderBuffer& rob, Memory& memory)
  : ex_config_{ex_config}
  , alus_(ex_config.alu_amount)
  , mem_units_(ex_config.mem_unit_amount)
  , reservation_station_{reservation_station}
  , rob_{rob}
  , memory_{memory}
  {
  };


// This is the main loop. The order is CRITICAL.
void ExecutionStage::tick()
{
    process_completed_instructions();
    issue_ready_instructions();
}


void ExecutionStage::issue_ready_instructions() {
    auto& rs_entries = reservation_station_.get_entries();

    for (auto& entry : rs_entries) {
      if (entry.is_busy && entry.src1_is_ready && entry.src2_is_ready) 
      {
        if(entry.op == AluOperation::LOAD || entry.op == AluOperation::STORE)
        {
          for (auto& mem_unit : mem_units_) 
          {
            if (!mem_unit.is_busy) 
            {
              mem_unit.is_busy = true;
              mem_unit.remaining_cycles = ex_config_.mem_unit_cycles;
              
              // **FIX:** Store the *whole* instruction
              mem_unit.current_instruction = entry;
              
              // **FIX:** Pre-calculate the address (this is OK)
              mem_unit.address = entry.Vj + entry.immediate; 
              
              entry.is_busy = false; 
              break; 
              }
            }
          continue; // Move to the next RS entry
        }
        else // This is an ALU operation
        {
          for (auto& alu : alus_) 
          {
            if (!alu.is_busy) 
            {
              alu.is_busy = true;
              alu.remaining_cycles = ex_config_.alu_cycles;
              
              // **FIX:** Store the *whole* instruction
              alu.current_instruction = entry; 
              
              // **DO NOT** do the calculation here!
              
              entry.is_busy = false; 
              break; 
            }
          }
        }
      }
    }
}

void ExecutionStage::process_completed_instructions() {
  for (auto& alu : alus_) 
  {
    if (alu.is_busy) {
      alu.remaining_cycles--;
      if (alu.remaining_cycles == 0) 
      {
        auto& inst = alu.current_instruction;
        uint8_t tag = inst.tag;
        uint32_t result = 0;

        switch (inst.op) {
          case AluOperation::ADD:
              result = inst.Vj + inst.Vk;
              break;
          case AluOperation::SUB:
              result = inst.Vj - inst.Vk;
              break;
          default:
              result = 0;
        }

        reservation_station_.update_with_cdb_message(tag, result);
        rob_.update_entry(tag, result); 
        alu.is_busy = false;
      }
    }
  }

  for (auto& mem_unit : mem_units_) 
  {
    if (mem_unit.is_busy) {
      mem_unit.remaining_cycles--;
      if (mem_unit.remaining_cycles == 0) 
      {
        auto& inst = mem_unit.current_instruction;
        uint8_t tag = inst.tag;

        if (inst.op == AluOperation::LOAD) 
        {
            uint32_t loaded_data = memory_.read_word(mem_unit.address);
            
            reservation_station_.update_with_cdb_message(tag, loaded_data);
            rob_.update_entry(tag, loaded_data);

        } else if (inst.op == AluOperation::STORE) 
        {
            uint32_t data_to_store = inst.Vk; 
            memory_.store_word(mem_unit.address, data_to_store);
            rob_.update_entry(tag, 0); 
        }
        
        mem_unit.is_busy = false;
      }
    }
  }
}