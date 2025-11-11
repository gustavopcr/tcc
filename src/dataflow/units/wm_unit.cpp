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
  auto tokens = waiting_token_mem_.find(token_tag);
  if(tokens != waiting_token_mem_.end())
  {
    auto& token_list = tokens->second;
    tokens->second.push_back(token);
    auto fr_rule = fr_map_.find(token.ip);
    if(fr_rule != fr_map_.end() && token_list.size() == fr_rule->second)
    {
      // criar matched token
      MatchedToken mt = match_token(token_list);
      // inserir matched token em fila compartilhada entre wm-unit e instruction fetch
      matched_tokens_.push(mt);
      waiting_token_mem_.erase(tokens);
    }
    
  }
  else
  {
    std::vector<Token> token_list;
    token_list.push_back(token);
    waiting_token_mem_.insert({token_tag, token_list});
  }
}

MatchedToken WmUnit::match_token(const std::vector<Token>& tokens)
{
  MatchedToken mt;
  mt.ip = tokens[0].ip;
  mt.fp = tokens[0].fp;
  for(const auto& token : tokens)
  {
    mt.operands.push_back(token.v);
  }
}


std::vector<size_t> WmUnit::get_work_nodes()
{
  std::vector<size_t> work_nodes;
  return work_nodes;
}