// src/offsets.hpp
// CS2 offsets — UPDATE THESE AFTER EVERY GAME PATCH
// Dump with: https://github.com/a2x/cs2-dumper  or  haze-dumper / manual IDA
// Last placeholder set: structure only. Replace values before use.

#pragma once
#include <cstdint>

namespace offsets {
    // client.dll
    constexpr std::uintptr_t dwEntityList              = 0x19C2C98;
    constexpr std::uintptr_t dwLocalPlayerController   = 0x1A0F0A8;
    constexpr std::uintptr_t dwLocalPlayerPawn         = 0x1824A18;
    constexpr std::uintptr_t dwViewMatrix              = 0x1A21A30;
    constexpr std::uintptr_t dwViewAngles              = 0x1A2B4D0; // verify module

    // C_BaseEntity / pawn
    constexpr std::uintptr_t m_iHealth                 = 0x334;
    constexpr std::uintptr_t m_iTeamNum                = 0x3CB;
    constexpr std::uintptr_t m_vOldOrigin              = 0x127C;
    constexpr std::uintptr_t m_pGameSceneNode          = 0x318;
    constexpr std::uintptr_t m_modelState              = 0x160;
    constexpr std::uintptr_t m_boneArray               = 0x80;
    constexpr std::uintptr_t m_hPlayerPawn             = 0x7EC;
    constexpr std::uintptr_t m_bDormant                = 0xE8;
    constexpr std::uintptr_t m_vecViewOffset           = 0xC58;
    constexpr std::uintptr_t m_angEyeAngles            = 0x1578;
    constexpr std::uintptr_t m_iIDEntIndex             = 0x15A4;
    constexpr std::uintptr_t m_iszPlayerName           = 0x640; // controller
}
