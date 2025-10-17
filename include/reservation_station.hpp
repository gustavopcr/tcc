#ifndef RESERVATION_STATION_HPP
#define RESERVATION_STATION_HPP

#include "alu.hpp"
#include <cstdint>
#include <vector>
#include <optional>

/* if invalid, has a tag
   if valid, has a value
*/

struct ReservationStationEntry {
  bool is_busy = false; // Is this entry in use?
  AluOperation op;

  // Source Operand 1
  bool src1_is_ready = false;
  uint32_t Vj; // Use if src1_is_ready is true
  uint8_t Qj;  // Use if src1_is_ready is false

  // Source Operand 2
  bool src2_is_ready = false;
  uint32_t Vk;             // Use if src2_is_ready is true
  uint8_t Qk; // Use if src2_is_ready is false
  
  uint8_t arch_reg;
  uint32_t immediate = 0;
};

struct CommonDataBusMessage {
    uint8_t tag;
    uint32_t value;
};

class ReservationStation {
public:
  explicit ReservationStation(size_t size);

  bool is_full() const;

  void update_with_cdb_message(uint8_t tag, uint32_t value);
  void add(const ReservationStationEntry& entry_to_add);

  std::vector<ReservationStationEntry>& get_entries();

private:
  std::vector<ReservationStationEntry> entries_;
};
#endif