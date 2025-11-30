#ifndef EXECUTION_UNIT_HPP
#define EXECUTION_UNIT_HPP

#include "von_neumann/uop.hpp"

class ExecutionUnit {
public:
    explicit ExecutionUnit(int latency = 1);
    
    void start_execution(const ExecutingInst& inst);
    void tick();
    
    bool has_result() const;
    ExecutionResult get_result();
    bool is_busy() const;
    void flush();

private:
    ExecutionResult compute_result(const ExecutingInst& inst);
    
    int latency_;
    ExecutingInst current_inst_{};
    ExecutionResult result_{};
    bool busy_ = false;
    bool has_result_ = false;
};

#endif