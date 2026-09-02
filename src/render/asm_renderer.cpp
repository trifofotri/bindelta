#include "render/asm_renderer.hpp"
#include "diff/insn_diff.hpp"
#include "render/color.hpp"

#include <capstone/capstone.h>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace bd {
    std::vector<DisasmLine> disassemble(const std::vector<uint8_t>& bytes, uint64_t start_address, bool is_64bit) {
        std::vector<DisasmLine> lines;
        if (bytes.empty()) return lines;

        csh handle;
        cs_mode mode = is_64bit ? CS_MODE_64 : CS_MODE_32;
        if (cs_open(CS_ARCH_X86, mode, &handle) != CS_ERR_OK) {
            return lines;
        }

        cs_insn* insn;
        size_t count = cs_disasm(handle, bytes.data(), bytes.size(), start_address, 0, &insn);

        for (size_t i = 0; i < count; i++) {
            DisasmLine line;
            line.address = insn[i].address;
            line.length = insn[i].size;
            line.mnemonic = insn[i].mnemonic;
            line.operands = insn[i].op_str;

            std::ostringstream hex;
            for (int b = 0; b < insn[i].size; b++) {
                if (b) {
                    hex << ' ';
                }
                hex << std::hex << std::setw(2) << std::setfill('0') << (int)insn[i].bytes[b];
            }
            line.bytes_hex = hex.str();

            lines.push_back(std::move(line));
        }

        if (count > 0) cs_free(insn, count);
        cs_close(&handle);

        return lines;
    }

    void print_line(const DisasmLine& l, const char* prefix, bool is_new, bool dimmed) {
        std::ostringstream text;
        text << std::hex << std::setfill('0') << std::setw(8) << l.address << std::dec << "  "
            << std::left << std::setfill(' ') << std::setw(20) << l.bytes_hex
            << std::left << std::setw(8) << l.mnemonic << l.operands;

        std::string full = std::string(prefix) + " " + text.str();

        if (dimmed) {
            printf("%s\n", dim(full).c_str());
        } else {
            printf("%s\n", (is_new ? bg_green(full) : bg_red(full)).c_str());
        }
    }

    void print_asm_diff(const std::vector<DisasmLine>& old_lines, const std::vector<DisasmLine>& new_lines, size_t context_lines) {
        size_t max_len = std::max(old_lines.size(), new_lines.size());
        
        std::vector<Row> rows;
        for (size_t i = 0; i < max_len; i++) {
            bool has_old = i < old_lines.size();
            bool has_new = i < new_lines.size();

            if (has_old && has_new) {
                const auto& o = old_lines[i];
                const auto& n = new_lines[i];
                bool same = o.bytes_hex == n.bytes_hex && o.mnemonic == n.mnemonic && o.operands == n.operands;
                rows.push_back({same, &o, &n});
            } else if (has_old) {
                rows.push_back({false, &old_lines[i], nullptr});
            } else {
                rows.push_back({false, nullptr, &new_lines[i]});
            }
        }

        size_t i = 0;
        while (i < rows.size()) {
            if (!rows[i].same) {
                if (rows[i].old_l) {
                    print_line(*rows[i].old_l, "-", false, false);
                }
                if (rows[i].new_l) {
                    print_line(*rows[i].new_l, "+", true, false);
                }
                i++;
                continue;
            }

            size_t run_start = i;
            while (i < rows.size() && rows[i].same) {
                i++;
            }
            size_t run_len = i - run_start;

            if (run_len <= context_lines * 2) {
                for (size_t j = run_start; j < i; j++) {
                    print_line(*rows[j].old_l, " ", false, true);
                }
            } else {
                for (size_t j = run_start; j < run_start + context_lines; j++) {
                    print_line(*rows[j].old_l, " ", false, true);
                }
                printf("%s\n", dim("  ... " + std::to_string(run_len - context_lines * 2) + " unchanged instruction(s) ...").c_str());
                for (size_t j = i - context_lines; j < i; j++) {
                    print_line(*rows[j].old_l, " ", false, true);
                }
            }
        }
    }
}