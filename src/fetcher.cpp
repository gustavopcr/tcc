#include "fetcher.hpp"

Fetcher::Fetcher(uint32_t& pc, Memory& memory)
: pc_{pc}
, memory_{memory}
{ 
}

void Fetcher::tick()
{
  pc_ += 4;
}