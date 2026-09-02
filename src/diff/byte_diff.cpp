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

}