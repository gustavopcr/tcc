#ifndef OPERATION_HPP
#define OPERATION_HPP

enum class Operation
{
  // --- Computational Nodes ---
  ADD,
  SUB,
  MULT,
  DIV,

  // --- Control/Routing Nodes ---
  FORK,
  SWITCH,
  MERGE

  // --- Host Interface Nodes ---
  INPUT,
  OUTPUT
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
  INPUT = 1,
  OUTPUT = 1,
};

#endif