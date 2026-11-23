// src/main.cpp
// CS2 External ESP + Aimbot
// C++17 | Windows 11 x64 | pure WinAPI + GDI | no write, mouse-only aim
#include "cheat.hpp"
#include <ctime>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    srand(static_cast<unsigned>(time(nullptr)));

    Cheat cheat;
    if (!cheat.init()) {
        MessageBoxW(nullptr, L"Attach failed. Start CS2 first.", L"Error", MB_ICONERROR);
        return 1;
    }

    cheat.run();
    cheat.shutdown();
    return 0;
}
