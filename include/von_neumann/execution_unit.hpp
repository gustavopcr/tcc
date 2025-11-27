#ifndef EXECUTION_UNIT_HPP
#define EXECUTION_UNIT_HPP

#include "von_neumann/uop.hpp"
#include <optional>

class ExecutionUnit {
public:
    explicit ExecutionUnit(int latency);
    
    bool accept(const RsEntry& entry);
    std::optional<ExecutionResult> tick();
    bool is_busy() const;

private:
    ExecutionResult compute_result(const ExecutingInst& inst);
    
    int latency_;
    std::optional<ExecutingInst> current_inst_;
};

#endif