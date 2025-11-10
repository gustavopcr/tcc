#ifndef OPERATION_HPP
#define OPERATION_HPP

enum class Operation
{
  ADD,
  SUB,
  MULT,
  DIV,
  FORK,
  SWITCH,
  MERGE
};

enum class OperationCycles
{
  ADD = 2,
  SUB = 2,
  MULT = 4,
  DIV = 6,
  FORK = 3,
  SWITCH = 4,
  MERGE = 5
};

#endif