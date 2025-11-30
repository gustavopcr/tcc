#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

class Assembler {
public:
    Assembler();
    
    // Assemble a list of assembly lines into binary instructions
    std::vector<uint32_t> assemble(const std::vector<std::string>& lines);
    
    // Assemble a single instruction
    uint32_t assemble_instruction(const std::string& line);

private:
    // Register name to number mapping
    std::unordered_map<std::string, uint8_t> reg_map_;
    
    // Helper functions
    uint8_t parse_register(const std::string& reg);
    int32_t parse_immediate(const std::string& imm);
    
    // Instruction encoders
    uint32_t encode_r_type(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t shamt, uint8_t funct);
    uint32_t encode_i_type(uint8_t opcode, uint8_t rs, uint8_t rt, int16_t immediate);
    uint32_t encode_j_type(uint8_t opcode, uint32_t address);
    
    // Parse memory operand like "offset($reg)"
    std::pair<int16_t, uint8_t> parse_memory_operand(const std::string& operand);
    
    // Tokenize instruction line
    std::vector<std::string> tokenize(const std::string& line);
    
    // Remove leading/trailing whitespace
    std::string trim(const std::string& s);
};

#endif