#ifndef WM_UNIT_HPP
#define WM_UNIT_HPP

#include "dataflow/primitives/token.hpp"
#include <unordered_map>
#include <vector>

// WaitMatch Unit
class WmUnit
{
public:
  void tick(Token token);
  std::vector<size_t> get_work_nodes(); // returns id of nodes that can go to instruction fetch

private:
  std::unordered_map<size_t, Token> waiting_token_mem_;
};
#endif