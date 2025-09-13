#include "mips.hpp"

Mips::Mips(Memory memory, 
                FetchStage fetch, 
                DecodeStage decode, 
                ExecuteStage execute,
                MemoryAccessStage memory_access,
                WriteBackStage write_back
)
: memory_{memory}
, fetch_{fetch}
, decode_{decode}
, execute_{execute}
, memory_access_{memory_access}
, write_back_{write_back}
{
}


void Mips::run()
{
  clock_tick();
}

void Mips::clock_tick()
{
  write_back_.run();
  memory_access_.run();
  execute_.run();
  decode_.run();
  fetch_.run();
}