#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <string>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Color {
    inline const char* Reset      = "\033[0m";
    inline const char* Bold       = "\033[1m";
    inline const char* Dim        = "\033[2m";
    inline const char* Red        = "\033[31m";
    inline const char* Green      = "\033[32m";
    inline const char* Yellow     = "\033[33m";
    inline const char* Blue       = "\033[34m";
    inline const char* Magenta    = "\033[35m";
    inline const char* Cyan       = "\033[36m";
    inline const char* White      = "\033[37m";

    inline const char* BoldRed    = "\033[1;31m";
    inline const char* BoldGreen  = "\033[1;32m";
    inline const char* BoldYellow = "\033[1;33m";
    inline const char* BoldBlue   = "\033[1;34m";
    inline const char* BoldMagenta= "\033[1;35m";
    inline const char* BoldCyan   = "\033[1;36m";
    inline const char* BoldWhite  = "\033[1;37m";

    inline void init() {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
#endif
    }
}

#endif // TERMINAL_HPP
