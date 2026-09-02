#include "diff/insn_diff.hpp"
#include "render/color.hpp"
#include <regex>
#include <vector>

namespace bd {
    static const std::vector<std::string> kAddressJumpMnemonics = {
        "jmp", "je", "jne", "jz", "jnz", "jg", "jge", "jl", "jle",
        "ja", "jae", "jb", "jbe", "js", "jns", "jo", "jno", "jp", "jnp",
        "call", "loop", "loope", "loopne"
    };

    static bool is_branch_mnemonic(const std::string& mnemonic) {
        for (const auto& m : kAddressJumpMnemonics) {
            if (mnemonic == m) return true;
        }
        return false;
    }

    std::string normalize_insn(const DisasmLine& line) {
        std::string ops = line.operands;

        static const std::regex rip_disp(R"(rip\s*\+\s*0x[0-9a-fA-F]+)");
        ops = std::regex_replace(ops, rip_disp, "rip + ?");

        if (is_branch_mnemonic(line.mnemonic)) {
            static const std::regex hex_target(R"(0x[0-9a-fA-F]+)");
            ops = std::regex_replace(ops, hex_target, "?");
        }

        return line.mnemonic + " " + ops;
    }

    std::vector<InsnDiffEntry> align_instructions(const std::vector<DisasmLine>& old_insns, const std::vector<DisasmLine>& new_insns) {
        size_t n = old_insns.size();
        size_t m = new_insns.size();

        // precompute keys once
        std::vector<std::string> old_keys(n), new_keys(m);
        for (size_t i = 0; i < n; i++) {
            old_keys[i] = normalize_insn(old_insns[i]);
        }
        for (size_t j = 0; j < m; j++) {
            new_keys[j] = normalize_insn(new_insns[j]);
        }

        // LCS length table
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
        for (size_t i = 1; i <= n; i++) {
            for (size_t j = 1; j <= m; j++) {
                if (old_keys[i - 1] == new_keys[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                } else {
                    dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
                }
            }
        }

        // build the diff script
        std::vector<InsnDiffEntry> result;
        size_t i = n, j = m;
        while (i > 0 && j > 0) {
            if (old_keys[i - 1] == new_keys[j - 1]) {
                result.push_back({InsnDiffKind::Unchanged, &old_insns[i - 1], &new_insns[j - 1]});
                i--;
                j--;
            } else if (dp[i - 1][j] >= dp[i][j - 1]) {
                result.push_back({InsnDiffKind::Removed, &old_insns[i - 1], nullptr});
                i--;
            } else {
                result.push_back({InsnDiffKind::Added, nullptr, &new_insns[j - 1]});
                j--;
            }
        }
        while (i > 0) {
            result.push_back({InsnDiffKind::Removed, &old_insns[i - 1], nullptr});
            i--;
        }

        while (j > 0) {
            result.push_back({InsnDiffKind::Added, nullptr, &new_insns[j - 1]});
            j--;
        }

        std::reverse(result.begin(), result.end());
        return result;
    }

    void print_insn_alignment(const std::vector<InsnDiffEntry>& entries, size_t context_lines) {
        size_t i = 0;
        while (i < entries.size()) {
            if (entries[i].kind != InsnDiffKind::Unchanged) {
                const auto& e = entries[i];
                if (e.kind == InsnDiffKind::Removed) {
                    print_line(*e.old_line, "-", false, false);
                } else {
                    print_line(*e.new_line, "+", true, false);
                }
                i++;
                continue;
            }

            size_t run_start = i;
            while (i < entries.size() && entries[i].kind == InsnDiffKind::Unchanged) {
                i++;
            }
            size_t run_len = i - run_start;

            if (run_len <= context_lines * 2) {
                for (size_t j = run_start; j < i; j++) {
                    print_line(*entries[j].old_line, " ", false, true);
                }
            } else {
                for (size_t j = run_start; j < run_start + context_lines; j++) {
                    print_line(*entries[j].old_line, " ", false, true);
                }
                printf("%s\n", dim("  ... " + std::to_string(run_len - context_lines * 2) + " unchanged instruction(s) ...").c_str());
                for (size_t j = i - context_lines; j < i; j++) {
                    print_line(*entries[j].old_line, " ", false, true);
                }
            }
        }
    }
}