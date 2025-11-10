
#ifndef EXECUTION_PACKAGE_HPP
#define EXECUTION_PACKAGE_HPP

#include "dataflow/primitives/token.hpp"
#include <cstddef>
#include <vector>

struct ExecutionPackage
{
  size_t fp;
  Operation op;
  std::vector<size_t> operands;
  std::vector<Destination> destinations;
};

#endif