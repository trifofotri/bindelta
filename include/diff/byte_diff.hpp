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
}