#ifndef EXECUTE_STAGE_HPP
#define EXECUTE_STAGE_HPP

#include "issuer.hpp"
#include "reservation_station.hpp"
#include "alu.hpp"
#include <vector>

struct ExecutionUnit
{
  bool is_busy;
  int remaining_cycles;
  uint8_t dest_tag;
  uint32_t result;
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
  explicit ExecutionStage(ExecutionConfig ex_config, ReservationStation& reservation_station);

  void tick();
private:
  const ExecutionConfig ex_config_;
  std::vector<ExecutionUnit> alus_;
  std::vector<ExecutionUnit> mem_units_;
  ReservationStation& reservation_station_;
};
#endif