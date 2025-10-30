#include "von_neumann/fetcher.hpp"

Fetcher::Fetcher(uint32_t& pc, Memory& memory, FetchDecodeQueue& output_queue)
: pc_{pc}
, memory_{memory}
, output_queue_{output_queue}
{ 
}

void Fetcher::tick()
{
  if(output_queue_.size() >= MAX_FETCH_DECODE_QUEUE_SIZE)
  {
    return;
  }

  uint32_t instruction_data = memory_.read_word(pc_);
  output_queue_.push(instruction_data);
  pc_ += 4;
}