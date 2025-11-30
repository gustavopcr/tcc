#include "von_neumann/assembler.hpp"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cctype>

Assembler::Assembler() {
    // Standard MIPS register names
    reg_map_["$zero"] = 0;
    reg_map_["$at"] = 1;
    reg_map_["$v0"] = 2;
    reg_map_["$v1"] = 3;
    reg_map_["$a0"] = 4;
    reg_map_["$a1"] = 5;
    reg_map_["$a2"] = 6;
    reg_map_["$a3"] = 7;
    reg_map_["$t0"] = 8;
    reg_map_["$t1"] = 9;
    reg_map_["$t2"] = 10;
    reg_map_["$t3"] = 11;
    reg_map_["$t4"] = 12;
    reg_map_["$t5"] = 13;
    reg_map_["$t6"] = 14;
    reg_map_["$t7"] = 15;
    reg_map_["$s0"] = 16;
    reg_map_["$s1"] = 17;
    reg_map_["$s2"] = 18;
    reg_map_["$s3"] = 19;
    reg_map_["$s4"] = 20;
    reg_map_["$s5"] = 21;
    reg_map_["$s6"] = 22;
    reg_map_["$s7"] = 23;
    reg_map_["$t8"] = 24;
    reg_map_["$t9"] = 25;
    reg_map_["$k0"] = 26;
    reg_map_["$k1"] = 27;
    reg_map_["$gp"] = 28;
    reg_map_["$sp"] = 29;
    reg_map_["$fp"] = 30;
    reg_map_["$ra"] = 31;
    
    // Numeric register names ($0 - $31)
    for (int i = 0; i <= 31; ++i) {
        reg_map_["$" + std::to_string(i)] = static_cast<uint8_t>(i);
    }
}

std::string Assembler::trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> Assembler::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string cleaned = trim(line);
    
    // Remove comments
    auto comment_pos = cleaned.find('#');
    if (comment_pos != std::string::npos) {
        cleaned = cleaned.substr(0, comment_pos);
    }
    
    if (cleaned.empty()) return tokens;
    
    // Replace commas with spaces
    std::replace(cleaned.begin(), cleaned.end(), ',', ' ');
    
    std::istringstream iss(cleaned);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

uint8_t Assembler::parse_register(const std::string& reg) {
    std::string r = trim(reg);
    auto it = reg_map_.find(r);
    if (it == reg_map_.end()) {
        throw std::runtime_error("Unknown register: " + reg);
    }
    return it->second;
}

int32_t Assembler::parse_immediate(const std::string& imm) {
    std::string s = trim(imm);
    if (s.empty()) return 0;
    
    try {
        // Handle hex (0x prefix)
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            return static_cast<int32_t>(std::stoul(s, nullptr, 16));
        }
        // Handle negative numbers
        return std::stoi(s);
    } catch (...) {
        throw std::runtime_error("Invalid immediate: " + imm);
    }
}

std::pair<int16_t, uint8_t> Assembler::parse_memory_operand(const std::string& operand) {
    // Format: offset($reg) or ($reg)
    std::string s = trim(operand);
    
    auto paren_open = s.find('(');
    auto paren_close = s.find(')');
    
    if (paren_open == std::string::npos || paren_close == std::string::npos) {
        throw std::runtime_error("Invalid memory operand: " + operand);
    }
    
    int16_t offset = 0;
    if (paren_open > 0) {
        offset = static_cast<int16_t>(parse_immediate(s.substr(0, paren_open)));
    }
    
    std::string reg = s.substr(paren_open + 1, paren_close - paren_open - 1);
    uint8_t reg_num = parse_register(reg);
    
    return {offset, reg_num};
}

uint32_t Assembler::encode_r_type(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t shamt, uint8_t funct) {
    return (0u << 26) |              // opcode = 0 for R-type
           (static_cast<uint32_t>(rs) << 21) |
           (static_cast<uint32_t>(rt) << 16) |
           (static_cast<uint32_t>(rd) << 11) |
           (static_cast<uint32_t>(shamt) << 6) |
           static_cast<uint32_t>(funct);
}

uint32_t Assembler::encode_i_type(uint8_t opcode, uint8_t rs, uint8_t rt, int16_t immediate) {
    return (static_cast<uint32_t>(opcode) << 26) |
           (static_cast<uint32_t>(rs) << 21) |
           (static_cast<uint32_t>(rt) << 16) |
           (static_cast<uint32_t>(immediate) & 0xFFFF);
}

uint32_t Assembler::encode_j_type(uint8_t opcode, uint32_t address) {
    return (static_cast<uint32_t>(opcode) << 26) |
           (address & 0x03FFFFFF);
}

uint32_t Assembler::assemble_instruction(const std::string& line) {
    auto tokens = tokenize(line);
    if (tokens.empty()) return 0;  // NOP
    
    std::string mnemonic = tokens[0];
    // Convert to uppercase for comparison
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(), ::toupper);
    
    // R-type instructions
    if (mnemonic == "ADD") {
        // ADD rd, rs, rt
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x20);
    }
    else if (mnemonic == "SUB") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x22);
    }
    else if (mnemonic == "AND") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x24);
    }
    else if (mnemonic == "OR") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x25);
    }
    else if (mnemonic == "XOR") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x26);
    }
    else if (mnemonic == "SLT") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x2A);
    }
    else if (mnemonic == "SLL") {
        // SLL rd, rt, shamt
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rt = parse_register(tokens[2]);
        uint8_t shamt = static_cast<uint8_t>(parse_immediate(tokens[3]));
        return encode_r_type(0, rt, rd, shamt, 0x00);
    }
    else if (mnemonic == "SRL") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rt = parse_register(tokens[2]);
        uint8_t shamt = static_cast<uint8_t>(parse_immediate(tokens[3]));
        return encode_r_type(0, rt, rd, shamt, 0x02);
    }
    else if (mnemonic == "NOR") {
        uint8_t rd = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        uint8_t rt = parse_register(tokens[3]);
        return encode_r_type(rs, rt, rd, 0, 0x27);
    }
    // I-type ALU instructions
    else if (mnemonic == "ADDI") {
        // ADDI rt, rs, imm
        uint8_t rt = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x08, rs, rt, imm);
    }
    else if (mnemonic == "ADDIU") {
        uint8_t rt = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x09, rs, rt, imm);
    }
    else if (mnemonic == "ANDI") {
        uint8_t rt = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x0C, rs, rt, imm);
    }
    else if (mnemonic == "ORI") {
        uint8_t rt = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x0D, rs, rt, imm);
    }
    else if (mnemonic == "XORI") {
        uint8_t rt = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x0E, rs, rt, imm);
    }
    else if (mnemonic == "SLTI") {
        uint8_t rt = parse_register(tokens[1]);
        uint8_t rs = parse_register(tokens[2]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x0A, rs, rt, imm);
    }
    else if (mnemonic == "LUI") {
        // LUI rt, imm (load upper immediate)
        uint8_t rt = parse_register(tokens[1]);
        int16_t imm = static_cast<int16_t>(parse_immediate(tokens[2]));
        return encode_i_type(0x0F, 0, rt, imm);
    }
    // Load/Store instructions
    else if (mnemonic == "LW") {
        // LW rt, offset(rs)
        uint8_t rt = parse_register(tokens[1]);
        auto [offset, rs] = parse_memory_operand(tokens[2]);
        return encode_i_type(0x23, rs, rt, offset);
    }
    else if (mnemonic == "SW") {
        // SW rt, offset(rs)
        uint8_t rt = parse_register(tokens[1]);
        auto [offset, rs] = parse_memory_operand(tokens[2]);
        return encode_i_type(0x2B, rs, rt, offset);
    }
    // Branch instructions
    else if (mnemonic == "BEQ") {
        // BEQ rs, rt, offset
        uint8_t rs = parse_register(tokens[1]);
        uint8_t rt = parse_register(tokens[2]);
        int16_t offset = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x04, rs, rt, offset);
    }
    else if (mnemonic == "BNE") {
        // BNE rs, rt, offset
        uint8_t rs = parse_register(tokens[1]);
        uint8_t rt = parse_register(tokens[2]);
        int16_t offset = static_cast<int16_t>(parse_immediate(tokens[3]));
        return encode_i_type(0x05, rs, rt, offset);
    }
    // Jump instructions
    else if (mnemonic == "J") {
        // J target
        uint32_t target = static_cast<uint32_t>(parse_immediate(tokens[1]));
        return encode_j_type(0x02, target >> 2);  // Word-aligned
    }
    else if (mnemonic == "JAL") {
        uint32_t target = static_cast<uint32_t>(parse_immediate(tokens[1]));
        return encode_j_type(0x03, target >> 2);
    }
    else if (mnemonic == "JR") {
        // JR rs (R-type with funct 0x08)
        uint8_t rs = parse_register(tokens[1]);
        return encode_r_type(rs, 0, 0, 0, 0x08);
    }
    else if (mnemonic == "NOP") {
        return 0;  // SLL $zero, $zero, 0
    }
    
    throw std::runtime_error("Unknown instruction: " + mnemonic);
}

std::vector<uint32_t> Assembler::assemble(const std::vector<std::string>& lines) {
    std::vector<uint32_t> program;
    
    for (const auto& line : lines) {
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;
        if (trimmed[0] == '#') continue;  // Skip comment lines
        
        // Skip labels (lines ending with ':')
        if (trimmed.back() == ':') continue;
        
        uint32_t inst = assemble_instruction(trimmed);
        program.push_back(inst);
    }
    
    return program;
}