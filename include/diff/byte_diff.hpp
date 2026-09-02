#pragma once
#include <cstdint>
#include <vector>

namespace bd {
    struct ByteRange {
        uint64_t offset;
        uint64_t length;
    };

    struct DiffRegion {
        ByteRange old_range;
        ByteRange new_range;
    };

    // compares two buffers up to min(a.size(), b.size())
    std::vector<DiffRegion> diff_bytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);
    
    std::vector<bd::DiffRegion> merge_close_regions(const std::vector<bd::DiffRegion>& regions, uint64_t merge_distance);
}