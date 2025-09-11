#include "fetch_stage.hpp"

FetchStage::FetchStage(uint32_t& pc, Memory& memory, IF_ID& if_id)
: pc_{pc}
, memory_{memory}
, if_id_{if_id}
{ 
}

void FetchStage::run()
{
  if_id_.instruction = memory_.read_word(pc_);
  if_id_.next_pc = pc_ + 4;
  pc_ += 4;
}