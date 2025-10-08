#ifndef ISSUER_HPP
#define ISSUER_HPP

#include "alu.hpp"
#include "instruction.hpp"
#include <vector>

struct IssueEntry {
    bool is_valid;
    AluOperation op;
    uint32_t     dest_p_reg;

    bool     src1_is_ready;
    uint32_t src1_p_reg_or_val;

    bool     src2_is_ready;
    uint32_t src2_p_reg_or_val;
};

const size_t MAX_ISSUE_BUFFER_SIZE = 10;
using IssueQueue = std::vector<IssueEntry>;

class Issuer{
public:
  explicit Issuer(FetchDecodeQueue& input_queue, IssueQueue& issue_queue);
  void tick();

private:
  FetchDecodeQueue& input_queue_;
  IssueQueue& issue_queue_;
};

uint32_t alu(AluOperation op, uint32_t rs, uint32_t rt);

#endif