#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace bd {
    struct DisasmLine {
        uint64_t address;
        uint32_t length;
        std::string bytes_hex;
        std::string mnemonic;
        std::string operands;
    };
                                                           /* look !!!*/
    // Disassembles a byte buffer starting at a given -->>virtual address<---.
    std::vector<DisasmLine> disassemble(const std::vector<uint8_t>& bytes, uint64_t start_address, bool is_64bit);

    void print_asm_diff(const std::vector<DisasmLine>& old_lines, const std::vector<DisasmLine>& new_lines);
}