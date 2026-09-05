#pragma once
#include <cstdint>
#include <string>

namespace bd {
    struct FileSpec {
        std::string path;
        bool is_raw = false;
        uint64_t base_addr = 0;
        uint64_t range_start = 0;
        uint64_t range_end = 0; // whole file
    };

    FileSpec parse_file_spec(const std::string& arg);
}