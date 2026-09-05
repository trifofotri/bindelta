#pragma once
#include "binary_file.hpp"

namespace bd {
    class RawFile : public BinaryFile {
    public:
        RawFile(std::vector<uint8_t> raw, uint64_t base_addr, uint64_t range_start = 0, uint64_t range_end = 0);

        const std::vector<Section>& sections() const override {
            return sections_;
        }

        bool is_64bit() const override {
            return is64_;
        }
        const std::vector<uint8_t>& raw() const override {
            return raw_;
        }

        void set_64bit(bool v) {
            is64_ = v;
        }

    private:
        std::vector<uint8_t> raw_;
        std::vector<Section> sections_;
        bool is64_ = true;
    };
}