#pragma once
#include "binary_file.hpp"

namespace bd {
    class ElfFile : public BinaryFile {
    public:
        explicit ElfFile(std::vector<uint8_t> raw);

        const std::vector<Section>& sections() const override {
            return sections_;
        }

        bool is_64bit() const override {
            return is64_;
        }

        const std::vector<uint8_t>& raw() const override {
            return raw_;
        }
    private:
        std::vector<uint8_t> raw_;
        std::vector<Section> sections_;
        bool is64_ = false;

        void parse();
    };
}