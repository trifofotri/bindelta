#pragma once
#include "render/asm_renderer.hpp"
#include <vector>
#include <string>

namespace bd {
    enum class InsnDiffKind {
        Unchanged,
        Removed,
        Added
    };

    struct InsnDiffEntry {
        InsnDiffKind kind;
        const DisasmLine* old_line = nullptr;
        const DisasmLine* new_line = nullptr;
    };

    std::string normalize_insn(const DisasmLine& line);

    // aligns two instruction sequences using LCS, returns diffs.
    std::vector<InsnDiffEntry> align_instructions(const std::vector<DisasmLine>& old_insns, const std::vector<DisasmLine>& new_insns);

    void print_insn_alignment(const std::vector<InsnDiffEntry>& entries, size_t context_lines);
}