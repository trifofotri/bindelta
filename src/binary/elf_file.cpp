#include "binary/elf_file.hpp"

#include <elf.h>
#include <cstring>
#include <stdexcept>

namespace bd {

    ElfFile::ElfFile(std::vector<uint8_t> raw) : raw_(std::move(raw)) {
        parse();
    }

    void ElfFile::parse() {
        if (raw_.size() < sizeof(Elf32_Ehdr)) {
            throw std::runtime_error("file too small to be a valid ELF");
        }

        is64_ = raw_[EI_CLASS] == ELFCLASS64;

        const char* shstrtab = nullptr;

        if (is64_) {
            if (raw_.size() < sizeof(Elf64_Ehdr)) {
                throw std::runtime_error("truncated ELF64 header");
            }
            auto* eh = reinterpret_cast<const Elf64_Ehdr*>(raw_.data());

            if (eh->e_shoff == 0 || eh->e_shnum == 0) {
                throw std::runtime_error("ELF has no section headers");
            }
            if (eh->e_shoff + (uint64_t)eh->e_shnum * eh->e_shentsize > raw_.size()) {
                throw std::runtime_error("section header table out of bounds");
            }

            auto* shdrs = reinterpret_cast<const Elf64_Shdr*>(raw_.data() + eh->e_shoff);

            const Elf64_Shdr& strsec = shdrs[eh->e_shstrndx];
            shstrtab = reinterpret_cast<const char*>(raw_.data() + strsec.sh_offset);

            for (int i = 0; i < eh->e_shnum; i++) {
                const Elf64_Shdr& sh = shdrs[i];
                if (sh.sh_type == SHT_NULL) continue;

                Section sec;
                sec.name = shstrtab + sh.sh_name;
                sec.virtual_address = sh.sh_addr;
                sec.file_offset = sh.sh_offset;
                sec.size = sh.sh_size;
                sec.executable = (sh.sh_flags & SHF_EXECINSTR) != 0;
                sections_.push_back(std::move(sec));
            }
        } else {
            if (raw_.size() < sizeof(Elf32_Ehdr)) {
                throw std::runtime_error("truncated ELF32 header");
            }
            auto* eh = reinterpret_cast<const Elf32_Ehdr*>(raw_.data());

            if (eh->e_shoff == 0 || eh->e_shnum == 0) {
                throw std::runtime_error("ELF has no section headers");
            }
            if (eh->e_shoff + (uint64_t)eh->e_shnum * eh->e_shentsize > raw_.size()) {
                throw std::runtime_error("section header table out of bounds");
            }

            auto* shdrs = reinterpret_cast<const Elf32_Shdr*>(raw_.data() + eh->e_shoff);

            const Elf32_Shdr& strsec = shdrs[eh->e_shstrndx];
            shstrtab = reinterpret_cast<const char*>(raw_.data() + strsec.sh_offset);

            for (int i = 0; i < eh->e_shnum; i++) {
                const Elf32_Shdr& sh = shdrs[i];
                if (sh.sh_type == SHT_NULL) continue;

                Section sec;
                sec.name = shstrtab + sh.sh_name;
                sec.virtual_address = sh.sh_addr;
                sec.file_offset = sh.sh_offset;
                sec.size = sh.sh_size;
                sec.executable = (sh.sh_flags & SHF_EXECINSTR) != 0;
                sections_.push_back(std::move(sec));
            }
        }
    }

}