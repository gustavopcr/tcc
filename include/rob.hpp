#ifndef ROB_HPP
#define ROB_HPP

#include "alu.hpp"
#include "register.hpp"
#include "rat.hpp"
#include <cstdint>
#include <vector>

enum class RobState
{
  Waiting,
  Executing,
  Writeback
};

struct RobEntry{
  bool is_busy;
  AluOperation operation;
  RobState state;
  uint8_t arch_dest_reg;
  uint8_t physical_dest_reg;
  uint32_t result_value;
};

class ReorderBuffer
{
public:
  explicit ReorderBuffer(size_t size);
  uint8_t add(RobEntry entry); // returns tag
  bool is_full() const;
  void update_entry(uint8_t tag, uint32_t value);

  void commit(RegisterBank& registers, RegisterAliasTable& rat);


private:
  std::vector<RobEntry> entries_;
  size_t head_;
  size_t tail_;
  size_t size_;
};

#endif