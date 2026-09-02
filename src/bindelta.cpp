#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "bindelta.hpp"

void print_usage_and_exit(char* ep) {
    printf("bindelta\n");
    printf("    Byte-level binary diffing tool with disassembly-aware output.\n\n");

    printf("Usage: %s <options> binary1 binary2\n", ep);
    printf("Options:\n");
    printf(" -h --help:              displays this.\n");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage_and_exit(argv[0]);
    }

    bd::bdctx current_ctx;
    for (int argi = 1; argi < argc; argi++) {
        if (std::strcmp(argv[argi], "-h") == 0 || std::strcmp(argv[argi], "--help") == 0 ) print_usage_and_exit(argv[0]);
        
        if (current_ctx.binary1path.empty()) {
            current_ctx.binary1path = argv[argi];
        } else {
            current_ctx.binary2path = argv[argi];
        }
    }

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
    
    char* b1 = reinterpret_cast<char *>(malloc(b1size));
    char* b2 = reinterpret_cast<char *>(malloc(b2size));

    b1fs.read(b1, b1size);
    b2fs.read(b2, b2size);

    if (memcmp(b1, "\x7F" "ELF", 4) == 0) {

    }
    return 0;
}