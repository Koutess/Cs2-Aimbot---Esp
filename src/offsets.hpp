// src/offsets.hpp
#pragma once
#include <cstdint>

// These are filled at runtime by the pattern scanner.
// You no longer edit this file.

namespace offsets {
    inline std::uintptr_t dwEntityList          = 0;
    inline std::uintptr_t dwLocalPlayerPawn     = 0;
    inline std::uintptr_t dwViewMatrix          = 0;
    inline std::uintptr_t dwViewAngles          = 0;

    // These relative offsets change less often
    constexpr std::uintptr_t m_iHealth           = 0x334;
    constexpr std::uintptr_t m_iTeamNum          = 0x3CB;
    constexpr std::uintptr_t m_vOldOrigin        = 0x127C;
    constexpr std::uintptr_t m_pGameSceneNode    = 0x318;
    constexpr std::uintptr_t m_modelState        = 0x160;
    constexpr std::uintptr_t m_boneArray         = 0x80;
    constexpr std::uintptr_t m_hPlayerPawn       = 0x7EC;
    constexpr std::uintptr_t m_bDormant          = 0xE8;
    constexpr std::uintptr_t m_vecViewOffset     = 0xC58;
}
