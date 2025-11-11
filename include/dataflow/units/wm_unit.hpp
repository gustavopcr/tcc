#ifndef WM_UNIT_HPP
#define WM_UNIT_HPP

#include "dataflow/primitives/token.hpp"
#include <unordered_map>
#include <vector>
#include <queue>

// WaitMatch Unit
using FiringRuleMap = std::unordered_map<size_t, size_t>; // maps node tag to how many num inputs it needs. avoids having NodeMap as a dependency
class WmUnit
{
public:
  WmUnit(FiringRuleMap& fr, std::queue<MatchedToken>& matched_tokens_);
  void tick(Token token);
  std::vector<size_t> get_work_nodes(); // returns id of nodes that can go to instruction fetch

private:
  MatchedToken match_token(const std::vector<Token>& tokens);

  std::unordered_map<TokenTag, std::vector<Token>> waiting_token_mem_;
  std::queue<MatchedToken>& matched_tokens_;
  FiringRuleMap fr_map_;
};
#endif