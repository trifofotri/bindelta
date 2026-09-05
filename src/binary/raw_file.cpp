#include "binary/raw_file.hpp"
#include <stdexcept>

namespace bd {
    bd::RawFile::RawFile(std::vector<uint8_t> raw, uint64_t base_addr, uint64_t range_start, uint64_t range_end) : raw_(std::move(raw)) {
        uint64_t end = (range_end == 0x00) ? raw_.size() : range_end;

        if (range_start >= raw_.size() || end > raw_.size() || range_start >= end) {
            throw std::runtime_error("invalid raw binary range");
        }

        Section sec;
        sec.name = ".raw";
        sec.file_offset = range_start;
        sec.size = end - range_start;
        sec.virtual_address = base_addr + range_start;
        sec.executable = true;
        sections_.push_back(sec);
    }
}