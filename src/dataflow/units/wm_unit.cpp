#include "dataflow/units/wm_unit.hpp"
#include <iostream>

WmUnit::WmUnit(FiringRuleMap& fr, std::queue<MatchedToken>& matched_tokens)
: waiting_token_mem_{}
, fr_map_{fr}
, matched_tokens_{matched_tokens}
{
}

void WmUnit::tick(Token token)
{
  TokenTag token_tag{token.ip, token.fp};
  
  // 1. Get or Create the token list for this tag
  auto it = waiting_token_mem_.find(token_tag);
  if (it == waiting_token_mem_.end()) {
      // Create new entry
      it = waiting_token_mem_.insert({token_tag, std::vector<Token>{}}).first;
  }
  
  auto& token_list = it->second;

  // 2. Insert Token (Handle Duplicate Ports)
  bool port_found = false;
  for(auto& t : token_list) {
      if(t.p == token.p) {
          t.v = token.v; // Update value (overwrite)
          port_found = true;
          break;
      }
  }
  if(!port_found) {
      token_list.push_back(token);
  }

  // 3. Check Firing Rule
  auto fr_rule = fr_map_.find(token.ip);
  if(fr_rule != fr_map_.end() && token_list.size() == fr_rule->second)
  {
    // Fire!
    MatchedToken mt = match_token(token_list);
    matched_tokens_.push(mt);
    
    // Remove from memory
    waiting_token_mem_.erase(it);
  }
}

MatchedToken WmUnit::match_token(const std::vector<Token>& tokens)
{
  MatchedToken mt;
  mt.ip = tokens[0].ip;
  mt.fp = tokens[0].fp;
  
  size_t max_port = 0;
  for(const auto& t : tokens) {
      if (t.p > max_port) max_port = t.p;
  }
  
  mt.operands.resize(max_port + 1, 0);

  for(const auto& token : tokens)
  {
    mt.operands[token.p] = token.v;
  }
  return mt;
}


std::vector<size_t> WmUnit::get_work_nodes()
{
  std::vector<size_t> work_nodes;
  return work_nodes;
}

bool WmUnit::is_idle() const
{
  return waiting_token_mem_.empty();
}

uint64_t WmUnit::get_current_occupancy() const {
    uint64_t total = 0;
    for (const auto& [tag, tokens] : waiting_token_mem_) {
        total += tokens.size();
    }
    return total;
}