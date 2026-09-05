#include "file_spec.hpp"
#include <cstdlib>

namespace bd {
    FileSpec parse_file_spec(const std::string& arg) {
        FileSpec spec;

        size_t at_pos = arg.find('@');
        if (at_pos == std::string::npos) {
            spec.path = arg;
            return spec;
        }

        spec.path = arg.substr(0, at_pos);
        spec.is_raw = true;

        std::string rest = arg.substr(at_pos + 1);

        size_t colon_pos = rest.find(':');
        std::string addr_part = (colon_pos == std::string::npos) ? rest : rest.substr(0, colon_pos);
        spec.base_addr = std::strtoull(addr_part.c_str(), nullptr, 16);

        if (colon_pos != std::string::npos) {
            std::string range_part = rest.substr(colon_pos + 1);
            size_t dash_pos = range_part.find('-');
            if (dash_pos != std::string::npos) {
                spec.range_start = std::strtoull(range_part.substr(0, dash_pos).c_str(), nullptr, 16);
                spec.range_end = std::strtoull(range_part.substr(dash_pos + 1).c_str(), nullptr, 16);
            }
        }

        return spec;
    }
}