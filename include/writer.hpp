#ifndef WRITER_H
#define WRITER_H

#include <array>
#include <cstdint>

class Writer{
public:
  explicit Writer(std::array<uint32_t, 32>&registers);
  void tick();
private:
  std::array<uint32_t, 32>& registers_;
};

#endif