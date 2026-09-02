#pragma once
#include <string>

namespace bd {
    inline bool g_color_enabled = true;

    // ANSI escape codes
    namespace color {
        constexpr const char* RESET = "\033[0m";
        constexpr const char* RED   = "\033[31m";
        constexpr const char* GREEN = "\033[32m";
        constexpr const char* DIM   = "\033[2m";
        constexpr const char* BOLD  = "\033[1m";
        constexpr const char* CYAN  = "\033[36m";

        constexpr const char* BG_RED = "\033[41m";
        constexpr const char* BG_GREEN = "\033[42m";
        constexpr const char* BG_RED_DIM = "\033[48;5;52m";
        constexpr const char* BG_GREEN_DIM = "\033[48;5;22m";
    }

    inline std::string wrap(const std::string& code, const std::string& s) {
        if (!g_color_enabled) return s;
        return code + s + color::RESET;
    }

    inline std::string red(const std::string& s) { return wrap(color::RED, s); }
    inline std::string green(const std::string& s) { return wrap(color::GREEN, s); }
    inline std::string dim(const std::string& s) { return wrap(color::DIM, s); }
    inline std::string bold(const std::string& s) { return wrap(color::BOLD, s); }
    inline std::string cyan(const std::string& s) { return wrap(color::CYAN, s); }
    inline std::string bg_red(const std::string& s) { return wrap(color::BG_RED_DIM, s);}
    inline std::string bg_green(const std::string& s) { return wrap(color::BG_GREEN_DIM , s); }
}