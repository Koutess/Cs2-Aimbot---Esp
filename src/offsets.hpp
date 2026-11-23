// src/offsets.hpp
#pragma once
#include <cstdint>

// !!! REPLACE THESE WITH CURRENT DUMP VALUES OR YOU WILL SEE 0 PLAYERS !!!
// Use a2x/cs2-dumper or equivalent against your live client.dll

namespace offsets {
    constexpr std::uintptr_t dwEntityList            = 0x19C2C98;
    constexpr std::uintptr_t dwLocalPlayerPawn       = 0x1824A18;
    constexpr std::uintptr_t dwViewMatrix            = 0x1A21A30;
    constexpr std::uintptr_t dwViewAngles            = 0x1A2B4D0;

    constexpr std::uintptr_t m_iHealth               = 0x334;
    constexpr std::uintptr_t m_iTeamNum              = 0x3CB;
    constexpr std::uintptr_t m_vOldOrigin            = 0x127C;
    constexpr std::uintptr_t m_pGameSceneNode        = 0x318;
    constexpr std::uintptr_t m_modelState            = 0x160;
    constexpr std::uintptr_t m_boneArray             = 0x80;
    constexpr std::uintptr_t m_hPlayerPawn           = 0x7EC;
    constexpr std::uintptr_t m_bDormant              = 0xE8;
    constexpr std::uintptr_t m_vecViewOffset         = 0xC58;
}
