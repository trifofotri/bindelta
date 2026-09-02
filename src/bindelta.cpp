#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unistd.h>

#include "bindelta.hpp"
#include "binary/elf_file.hpp"
#include "diff/byte_diff.hpp"
#include "render/asm_renderer.hpp"
#include "render/color.hpp"

void print_usage_and_exit(char* ep) {
    printf("bindelta\n");
    printf("    Byte-level binary diffing tool with disassembly-aware output.\n\n");

    printf("Usage: %s <options> binary1 binary2\n", ep);
    printf("Options:\n");
    printf(" -h  --help:                 displays this.\n");
    printf(" -v  --verbose:              show noisy metadata sections (build-id, comments, etc).\n");
    printf(" -nc --no-color:             disable colored output.\n");
    exit(1);
}

std::vector<uint8_t> slice_section(const std::vector<uint8_t>& raw, const bd::Section& sec) {
    if (sec.file_offset + sec.size > raw.size()) return {};
    return std::vector<uint8_t>(raw.begin() + sec.file_offset, raw.begin() + sec.file_offset + sec.size);
}

bool is_noisy_section(const std::string& name) {
    static const std::vector<std::string> noisy = {
        ".note.gnu.build-id",
        ".note.gnu.property",
        ".note.ABI-tag",
        ".comment",
        ".gnu.version",
        ".gnu.version_r",
        ".shstrtab",
    };
    for (const auto& n : noisy) {
        if (name == n) return true;
    }
    return false;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage_and_exit(argv[0]);
    }

    bool no_color_flag = false;
    bool verbose_flag = false;
    bd::bdctx current_ctx;
    for (int argi = 1; argi < argc; argi++) {
        if (std::strcmp(argv[argi], "-h") == 0 || std::strcmp(argv[argi], "--help") == 0) print_usage_and_exit(argv[0]);
        
        if (std::strcmp(argv[argi], "-nc") == 0 || std::strcmp(argv[argi], "--no-color") == 0) {
            no_color_flag = true;
            continue;
        }
        
        if (std::strcmp(argv[argi], "-v") == 0 || std::strcmp(argv[argi], "--verbose") == 0) {
            verbose_flag = true;
            continue;
        }

        if (current_ctx.binary1path.empty()) {
            current_ctx.binary1path = argv[argi];
        } else {
            current_ctx.binary2path = argv[argi];
        }
    }
    bd::g_color_enabled = !no_color_flag && isatty(fileno(stdout));

    if (current_ctx.binary1path.empty() || current_ctx.binary2path.empty()) print_usage_and_exit(argv[0]);

    std::ifstream b1fs(current_ctx.binary1path, std::ios::binary);
    if (!b1fs.is_open()) {
        printf("%s: File does not exist.\n", current_ctx.binary1path.c_str());
        return 1;
    }

    std::ifstream b2fs(current_ctx.binary2path, std::ios::binary);
    if (!b2fs.is_open()) {
        printf("%s: File does not exist.\n", current_ctx.binary2path.c_str());
        return 1;
    }

    size_t b1size = std::filesystem::file_size(current_ctx.binary1path);
    size_t b2size = std::filesystem::file_size(current_ctx.binary2path);
    printf("Comparing %s (%zu bytes) and %s (%zu bytes)...\n", current_ctx.binary1path.c_str(), b1size, current_ctx.binary2path.c_str(), b2size);

    std::vector<uint8_t> b1(b1size);
    std::vector<uint8_t> b2(b2size);

    b1fs.read(reinterpret_cast<char*>(b1.data()), b1size);
    b2fs.read(reinterpret_cast<char*>(b2.data()), b2size);

    bool b1_is_elf = b1.size() >= 4 && memcmp(b1.data(), "\x7F" "ELF", 4) == 0;
    bool b2_is_elf = b2.size() >= 4 && memcmp(b2.data(), "\x7F" "ELF", 4) == 0;

    if (!b1_is_elf || !b2_is_elf) {
        printf("Only ELF binaries are supported right now.\n");
        return 1;
    }

    std::vector<uint8_t> b1_raw = b1;
    std::vector<uint8_t> b2_raw = b2;

    bd::ElfFile elf1(std::move(b1));
    bd::ElfFile elf2(std::move(b2));

    std::unordered_map<std::string, const bd::Section*> b2_sections_by_name;
    for (const auto& sec : elf2.sections()) {
        b2_sections_by_name[sec.name] = &sec;
    }

    for (const auto& sec1 : elf1.sections()) {
        if (!verbose_flag && is_noisy_section(sec1.name)) continue; /* skips useless noisy sections */

        auto it = b2_sections_by_name.find(sec1.name);
        if (it == b2_sections_by_name.end()) {
            printf("[removed] %s (not present in %s)\n", sec1.name.c_str(), current_ctx.binary2path.c_str());
            continue;
        }

        const bd::Section& sec2 = *it->second;
        std::vector<uint8_t> bytes1 = slice_section(b1_raw, sec1);
        std::vector<uint8_t> bytes2 = slice_section(b2_raw, sec2);

        auto regions = bd::diff_bytes(bytes1, bytes2);
        if (regions.empty()) {
            continue;
        }

        if (regions.size() > 1) {
            printf("\n[%s] %zu changed regions:\n", sec1.name.c_str(), regions.size());
        } else printf("\n[%s] %zu changed region:\n", sec1.name.c_str(), regions.size());
        
        if (sec1.executable) {
            auto old_insns = bd::disassemble(bytes1, sec1.virtual_address, elf1.is_64bit());
            auto new_insns = bd::disassemble(bytes2, sec2.virtual_address, elf2.is_64bit());

            auto find_overlapping = [](const std::vector<bd::DisasmLine>& insns, uint64_t sec_vaddr, uint64_t off, uint64_t len) -> std::pair<size_t, size_t> {
                uint64_t target_start = sec_vaddr + off;
                uint64_t target_end = target_start + len;

                size_t first = SIZE_MAX, last = SIZE_MAX;
                for (size_t i = 0; i < insns.size(); i++) {
                    uint64_t insn_start = insns[i].address;
                    uint64_t insn_end = insn_start + insns[i].length;
                    if (insn_end > target_start && insn_start < target_end) {
                        if (first == SIZE_MAX) first = i;
                        last = i;
                    }
                }
                return {first, last};
            };

            for (const auto& r : regions) {
                auto [old_first, old_last] = find_overlapping(old_insns, sec1.virtual_address, r.old_range.offset, r.old_range.length);
                auto [new_first, new_last] = find_overlapping(new_insns, sec2.virtual_address, r.new_range.offset, r.new_range.length);

                if (old_first == SIZE_MAX || new_first == SIZE_MAX) {
                    continue; /* couldn't map this region to instructions */
                }

                // grab 1 instruction of context before/after on each side
                size_t old_ctx_first = old_first > 0 ? old_first - 1 : 0;
                size_t old_ctx_last = std::min(old_last + 1, old_insns.size() - 1);
                size_t new_ctx_first = new_first > 0 ? new_first - 1 : 0;
                size_t new_ctx_last = std::min(new_last + 1, new_insns.size() - 1);

                std::vector<bd::DisasmLine> old_slice(old_insns.begin() + old_ctx_first, old_insns.begin() + old_ctx_last + 1);
                std::vector<bd::DisasmLine> new_slice(new_insns.begin() + new_ctx_first, new_insns.begin() + new_ctx_last + 1);

                printf("%s\n\n", bd::cyan("[" + sec1.name + "] instruction diff:").c_str());
                bd::print_asm_diff(old_slice, new_slice);
            }
        } else {
            for (const auto& r : regions) {
                printf("  old: offset=0x%lx len=%lu   new: offset=0x%lx len=%lu\n", r.old_range.offset, r.old_range.length, r.new_range.offset, r.new_range.length);
            }
        }
    }

    // sections only present in binary2
    std::unordered_map<std::string, bool> b1_names;
    for (const auto& sec : elf1.sections()) b1_names[sec.name] = true;
    for (const auto& sec : elf2.sections()) {
        if (!b1_names.count(sec.name)) {
            printf("[added] %s (not present in %s)\n", sec.name.c_str(), current_ctx.binary1path.c_str());
        }
    }

    return 0;
}