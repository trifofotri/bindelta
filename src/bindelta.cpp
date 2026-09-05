#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <unistd.h>

#include "bindelta.hpp"
#include "file_spec.hpp"
#include "binary/binary_file.hpp"
#include "binary/elf_file.hpp"
#include "binary/raw_file.hpp"
#include "diff/byte_diff.hpp"
#include "diff/insn_diff.hpp"
#include "diff/diff_config.hpp"
#include "render/asm_renderer.hpp"
#include "render/color.hpp"

void print_usage_and_exit(char* ep) {
    printf("bindelta\n");
    printf("    Byte-level binary diffing tool with disassembly-aware output.\n\n");

    printf("Usage: %s <options> binary1 binary2\n", ep);
    printf("\n");
    printf("Each binary can optionally be marked as a raw (headerless) binary using:\n");
    printf("  path@base_addr                treat as raw, loaded at base_addr (hex)\n");
    printf("  path@base_addr:start-end      same, but only scan file offsets [start,end) (hex)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s program1 program2\n", ep);
    printf("  %s firmware1.bin@0x8000 firmware2.bin@0x8000\n", ep);
    printf("  %s firmware1.bin@0x8000:0x100-0x2000 firmware2.bin@0x8000\n", ep);
    printf("\n");
    printf("Options:\n");
    printf(" -h    --help:                      displays this.\n");
    printf(" -v    --verbose:                   show noisy metadata sections (build-id, comments, etc).\n");
    printf(" -nc   --no-color:                  disable colored output.\n");
    printf(" -nlcs --no-lcs:                    disable alignment, maybe for speed, fall back to raw diff.\n");
    printf("       --window <n>:                instructions of context around each diff (default 40).\n");
    printf("       --context <n>:               unchanged lines shown before/after a change (default 2).\n");
    printf("       --merge-distance <n>:        byte distance to merge nearby diff regions (default 128).\n");
    exit(1);
}

std::vector<uint8_t> slice_section(const std::vector<uint8_t>& raw, const bd::Section& sec) {
    if (sec.file_offset + sec.size > raw.size()) return {};
    return std::vector<uint8_t>(raw.begin() + sec.file_offset, raw.begin() + sec.file_offset + sec.size);
}

bool is_noisy_section(const std::string& name) { /* there's a better way to do this */
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

std::unique_ptr<bd::BinaryFile> load_binary(const bd::FileSpec& spec) {
    std::ifstream fs(spec.path, std::ios::binary);
    if (!fs.is_open()) {
        printf("%s: File does not exist.\n", spec.path.c_str());
        return nullptr;
    }

    size_t size = std::filesystem::file_size(spec.path);
    std::vector<uint8_t> data(size);
    fs.read(reinterpret_cast<char*>(data.data()), size);

    if (spec.is_raw) {
        try {
            return std::make_unique<bd::RawFile>(std::move(data), spec.base_addr, spec.range_start, spec.range_end);
        } catch (const std::exception& e) {
            printf("%s: %s\n", spec.path.c_str(), e.what());
            return nullptr;
        }
    }

    bool is_elf = data.size() >= 0x4 && memcmp(data.data(), "\x7F" "ELF", 0x4) == 0;
    if (!is_elf) {
        printf("%s: not an ELF file. Use path@base_addr if this is a raw binary.\n", spec.path.c_str());
        return nullptr;
    }

    return std::make_unique<bd::ElfFile>(std::move(data));
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage_and_exit(argv[0]);
    }

    bool no_color_flag = false;
    bool verbose_flag = false;
    bool no_lcs_flag = false;
    bd::DiffConfig diff_config;

    bd::FileSpec spec1, spec2;
    bool have_spec1 = false;

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

        if (std::strcmp(argv[argi], "-nlcs") == 0 || std::strcmp(argv[argi], "--no-lcs") == 0) {
            no_lcs_flag = true;
            continue;
        }

        if (std::strcmp(argv[argi], "--window") == 0 && argi + 1 < argc) {
            diff_config.window = std::strtoul(argv[++argi], nullptr, 10);
            continue;
        }

        if (std::strcmp(argv[argi], "--context") == 0 && argi + 1 < argc) {
            diff_config.context_lines = std::strtoul(argv[++argi], nullptr, 10);
            continue;
        }

        if (std::strcmp(argv[argi], "--merge-distance") == 0 && argi + 1 < argc) {
            diff_config.merge_distance = std::strtoull(argv[++argi], nullptr, 10);
            continue;
        }

        bd::FileSpec spec = bd::parse_file_spec(argv[argi]);
        if (!have_spec1) {
            spec1 = spec;
            have_spec1 = true;
        } else {
            spec2 = spec;
        }
    }

    bd::g_color_enabled = !no_color_flag && isatty(fileno(stdout));

    if (spec1.path.empty() || spec2.path.empty())
        print_usage_and_exit(argv[0]);

    printf("Comparing %s and %s...\n", spec1.path.c_str(), spec2.path.c_str());

    std::unique_ptr<bd::BinaryFile> file1 = load_binary(spec1);
    if (!file1)
        return 1;

    std::unique_ptr<bd::BinaryFile> file2 = load_binary(spec2);
    if (!file2) return 1;

    std::unordered_map<std::string, const bd::Section*> b2_sections_by_name;
    for (const auto& sec : file2->sections()) {
        b2_sections_by_name[sec.name] = &sec;
    }

    for (const auto& sec1 : file1->sections()) {
        if (!verbose_flag && is_noisy_section(sec1.name)) continue;

        auto it = b2_sections_by_name.find(sec1.name);
        if (it == b2_sections_by_name.end()) {
            printf("[removed] %s (not present in %s)\n", sec1.name.c_str(), spec2.path.c_str());
            continue;
        }

        const bd::Section& sec2 = *it->second;
        std::vector<uint8_t> bytes1 = slice_section(file1->raw(), sec1);
        std::vector<uint8_t> bytes2 = slice_section(file2->raw(), sec2);

        auto regions = bd::diff_bytes(bytes1, bytes2);
        if (regions.empty()) {
            continue;
        }

        if (sec1.executable && !no_lcs_flag) {
            regions = merge_close_regions(regions, diff_config.merge_distance);
        }

        if (regions.size() > 1) {
            printf("\n[%s] %zu changed regions:\n", sec1.name.c_str(), regions.size());
        } else {
            printf("\n[%s] %zu changed region:\n", sec1.name.c_str(), regions.size());
        }

        if (sec1.executable) {
            auto old_insns = bd::disassemble(bytes1, sec1.virtual_address, file1->is_64bit());
            auto new_insns = bd::disassemble(bytes2, sec2.virtual_address, file2->is_64bit());

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
                    continue;
                }

                printf("\n%s\n", bd::cyan("[" + sec1.name + "] instruction diff:").c_str());

                if (no_lcs_flag) {
                    size_t old_ctx_first = old_first > 0 ? old_first - 1 : 0;
                    size_t old_ctx_last = std::min(old_last + 1, old_insns.size() - 1);
                    size_t new_ctx_first = new_first > 0 ? new_first - 1 : 0;
                    size_t new_ctx_last = std::min(new_last + 1, new_insns.size() - 1);

                    std::vector<bd::DisasmLine> old_slice(old_insns.begin() + old_ctx_first, old_insns.begin() + old_ctx_last + 1);
                    std::vector<bd::DisasmLine> new_slice(new_insns.begin() + new_ctx_first, new_insns.begin() + new_ctx_last + 1);
                    bd::print_asm_diff(old_slice, new_slice, diff_config.context_lines);
                } else {
                    size_t old_ctx_first = old_first > diff_config.window ? old_first - diff_config.window : 0;
                    size_t old_ctx_last = std::min(old_last + diff_config.window, old_insns.size() - 1);
                    size_t new_ctx_first = new_first > diff_config.window ? new_first - diff_config.window : 0;
                    size_t new_ctx_last = std::min(new_last + diff_config.window, new_insns.size() - 1);

                    std::vector<bd::DisasmLine> old_slice(old_insns.begin() + old_ctx_first, old_insns.begin() + old_ctx_last + 1);
                    std::vector<bd::DisasmLine> new_slice(new_insns.begin() + new_ctx_first, new_insns.begin() + new_ctx_last + 1);

                    auto alignment = bd::align_instructions(old_slice, new_slice);
                    bd::print_insn_alignment(alignment, diff_config.context_lines);
                }
            }
        } else {
            for (const auto& r : regions) {
                printf("  old: offset=0x%lx len=%lu   new: offset=0x%lx len=%lu\n", r.old_range.offset, r.old_range.length, r.new_range.offset, r.new_range.length);
            }
        }
    }

    std::unordered_map<std::string, bool> b1_names;
    for (const auto& sec : file1->sections()) {
        b1_names[sec.name] = true;
    }

    for (const auto& sec : file2->sections()) {
        if (!b1_names.count(sec.name)) {
            printf("[added] %s (not present in %s) \n", sec.name.c_str(), spec1.path.c_str());
        }
    }

    return 0;
}