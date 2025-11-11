#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "dataflow/primitives/node.hpp"

struct TokenTag
{
  uint64_t ip;
  uint64_t fp;

  bool operator==(const TokenTag& other) const
  {
    return fp == other.fp && ip == other.ip;
  }
};

struct Token
{
  uint64_t fp; // fp -> frame pointer -> used for function calls and loop management
  uint64_t ip; // ip -> instruction pointer
  uint8_t p; // p -> port(L or R) or indexes
  uint64_t v; // v -> value
};

struct MatchedToken
{
  uint64_t ip;
  uint64_t fp;
  std::vector<uint64_t> operands;
};

namespace std
{
  template<>
  struct hash<TokenTag>
  {
    std::size_t operator()(const TokenTag& tag) const
    {
      // 1. Get the hashes of the individual members.
      std::size_t hash_ip = std::hash<uint64_t>{}(tag.ip);
      std::size_t hash_fp = std::hash<uint64_t>{}(tag.fp);

      // 2. Combine them. (This is a common formula).
      return hash_ip ^ (hash_fp + 0x9e3779b9 + (hash_ip << 6) + (hash_ip >> 2));
    }
  };
}
#endif