#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace bd {
    struct Section {
        std::string name;
        uint64_t virtual_address;
        uint64_t file_offset;
        uint64_t size;
        bool executable;
        std::vector<uint8_t> data;
    };

    class BinaryFile {
    public:
        virtual ~BinaryFile() = default;
        virtual const std::vector<Section>& sections() const = 0;
        virtual bool is_64bit() const = 0;
    };
}