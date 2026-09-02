#include "diff/byte_diff.hpp"
#include <algorithm>

namespace bd {
    std::vector<DiffRegion> diff_bytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
        std::vector<DiffRegion> regions;
        uint64_t common = std::min(a.size(), b.size());

        uint64_t i = 0;
        while (i < common) {
            if (a[i] == b[i]) {
                i++;
                continue;
            }
            uint64_t start = i;
            while (i < common && a[i] != b[i]) {
                i++;
            }
            regions.push_back(DiffRegion{
                ByteRange{start, i - start},
                ByteRange{start, i - start}
            });
        }

        if (a.size() != b.size()) {
            uint64_t longer_len = std::max(a.size(), b.size());
            regions.push_back(DiffRegion{
                ByteRange{common, a.size() > common ? a.size() - common : 0},
                ByteRange{common, b.size() > common ? b.size() - common : 0}
            });
            (void)longer_len;
        }

        return regions;
    }

    std::vector<bd::DiffRegion> merge_close_regions(const std::vector<bd::DiffRegion>& regions, uint64_t merge_distance) {
        if (regions.empty()) return {};
        std::vector<bd::DiffRegion> merged;
        bd::DiffRegion current = regions[0];

        for (size_t i = 1; i < regions.size(); i++) {
            const auto& next = regions[i];
            uint64_t current_end = current.old_range.offset + current.old_range.length;

            if (next.old_range.offset <= current_end + merge_distance) {
                uint64_t new_end = std::max(current_end, next.old_range.offset + next.old_range.length); /* overlapping or close enough so we extend current region to cover both */
                current.old_range.length = new_end - current.old_range.offset;

                uint64_t new_end_b = std::max(current.new_range.offset + current.new_range.length, next.new_range.offset + next.new_range.length);
                current.new_range.length = new_end_b - current.new_range.offset;
            } else {
                merged.push_back(current);
                current = next;
            }
        }

        merged.push_back(current);
        return merged;
    }
}