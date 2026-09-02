#pragma once
#include <string>

namespace bd {
    // ANSI escape codes
    namespace color {
        constexpr const char* RESET = "\033[0m";
        constexpr const char* RED   = "\033[31m";
        constexpr const char* GREEN = "\033[32m";
        constexpr const char* DIM   = "\033[2m";
        constexpr const char* BOLD  = "\033[1m";
        constexpr const char* CYAN  = "\033[36m";
    }

    inline std::string red(const std::string& s) { return std::string(color::RED) + s + color::RESET; }
    inline std::string green(const std::string& s) { return std::string(color::GREEN) + s + color::RESET; }
    inline std::string dim(const std::string& s) { return std::string(color::DIM) + s + color::RESET; }
    inline std::string bold(const std::string& s) { return std::string(color::BOLD) + s + color::RESET; }
    inline std::string cyan(const std::string& s){ return std::string(color::CYAN) + s + color::RESET; }
}