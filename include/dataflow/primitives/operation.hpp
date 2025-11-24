#ifndef OPERATION_HPP
#define OPERATION_HPP

#include <string>

enum class Operation
{
  // --- Computational Nodes ---
  ADD,
  SUB,
  MULT,
  DIV,

    // --- Comparison Nodes ---
  SLT, // Set Less Than (<)
  SGT, // Set Greater Than (>)
  SLE, // Set Less Equal (<=)
  SGE, // Set Greater Equal (>=)
  EQ,  // Equal (==)

  // --- Control/Routing Nodes ---
  FORK,
  SWITCH,
  MERGE,
  
  // --- Recursion Support ---
  CALL,
  RETURN,

  // --- Memory 
  LOAD,
  STORE,
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
  MERGE = 5,
  CALL = 10,   // Context switch overhead
  RETURN = 10, // Context switch overhead
  LOAD = 20,
  STORE = 20,
  INPUT = 1,
  OUTPUT = 1,
};

Operation operation_from_string(std::string_view op_str)
{
    if (op_str == "ADD")    return Operation::ADD;
    if (op_str == "SUB")    return Operation::SUB;
    if (op_str == "MULT")   return Operation::MULT;
    if (op_str == "DIV")    return Operation::DIV;
    if (op_str == "SLT")    return Operation::SLT;
    if (op_str == "SGT")    return Operation::SGT;
    if (op_str == "SLE")    return Operation::SLE;
    if (op_str == "SGE")    return Operation::SGE;
    if (op_str == "EQ")     return Operation::EQ;
    if (op_str == "FORK")   return Operation::FORK;
    if (op_str == "SWITCH") return Operation::SWITCH;
    if (op_str == "MERGE")  return Operation::MERGE;
    if (op_str == "LOAD")   return Operation::LOAD;
    if (op_str == "STORE")  return Operation::STORE;
    if (op_str == "INPUT")  return Operation::INPUT;
    if (op_str == "OUTPUT") return Operation::OUTPUT;
    throw std::runtime_error("Unknown operation: " + std::string(op_str));
}

OperationCycles get_op_cycle(Operation op) {
    switch(op) {
        case Operation::ADD: return OperationCycles::ADD;
        case Operation::SUB: return OperationCycles::SUB;
        case Operation::MULT: return OperationCycles::MULT;
        case Operation::DIV: return OperationCycles::DIV;
        case Operation::FORK: return OperationCycles::FORK;
        case Operation::SWITCH: return OperationCycles::SWITCH;
        case Operation::MERGE: return OperationCycles::MERGE;
        case Operation::CALL: return OperationCycles::CALL;
        case Operation::RETURN: return OperationCycles::RETURN;
        case Operation::LOAD: return OperationCycles::LOAD;
        case Operation::STORE: return OperationCycles::STORE;
        case Operation::INPUT: return OperationCycles::INPUT;
        case Operation::OUTPUT: return OperationCycles::OUTPUT;
        default: return OperationCycles::ADD;
    }
}
#endif