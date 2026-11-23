// src/offsets.hpp
#pragma once
#include <cstdint>

namespace offsets {
    // From your dump — Build 14200 (2026-11-22)
    inline std::uintptr_t dwEntityList            = 0x270D5B0;
    inline std::uintptr_t dwLocalPlayerPawn       = 0x2557AB8;
    inline std::uintptr_t dwLocalPlayerController = 0x252F300;
    inline std::uintptr_t dwViewMatrix            = 0x255D3D0;
    inline std::uintptr_t dwViewAngles            = 0x256DBE8;

    // Fields from the same dump
    constexpr std::uintptr_t m_iHealth            = 0x34C;
    constexpr std::uintptr_t m_iTeamNum           = 0x3E7;
    constexpr std::uintptr_t m_vOldOrigin         = 0x14A4;
    constexpr std::uintptr_t m_pGameSceneNode     = 0x330;
    constexpr std::uintptr_t m_modelState         = 0x140;
    constexpr std::uintptr_t m_boneArray          = 0x80;
    constexpr std::uintptr_t m_hPlayerPawn        = 0x92C;
    constexpr std::uintptr_t m_bDormant           = 0x103;   // on CGameSceneNode
    constexpr std::uintptr_t m_vecViewOffset      = 0xF60;
    constexpr std::uintptr_t m_angEyeAngles       = 0x3630;
}
