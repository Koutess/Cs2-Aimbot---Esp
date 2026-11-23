// src/main.cpp
#include "cheat.hpp"
#include <ctime>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    srand((unsigned)time(nullptr));

    Cheat cheat;
    if (!cheat.init()) {
        Sleep(3000);
        return 1;
    }

    cheat.run();
    cheat.shutdown();
    return 0;
}
