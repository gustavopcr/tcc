#ifndef RAT_HPP
#define RAT_HPP

#include <cstdint>
#include <array>

struct RegisterAliasTableEntry {
  bool is_busy;
  uint8_t tag;
  uint32_t value;
};


class RegisterAliasTable {
public:
  RegisterAliasTable() = default;


  bool is_busy(uint8_t reg_index) const;
  uint8_t get_tag(uint8_t reg_index) const;
  void set_busy(uint8_t reg_index, uint8_t tag);
  void update_from_cdb(uint8_t tag);

private:
  std::array<RegisterAliasTableEntry, 32> entries_;
};

#endif