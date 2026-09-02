#pragma once
#include <cstddef>
#include <cstdint>

namespace bd {
    /* read help to understand what these do */
    struct DiffConfig {
        size_t window = 40;  
        size_t context_lines = 2; 
        uint64_t merge_distance = 128;
    };
}