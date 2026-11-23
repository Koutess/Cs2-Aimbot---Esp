// src/memory.hpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>

class Memory {
public:
    HANDLE hProcess = nullptr;
    DWORD  pid = 0;
    std::uintptr_t client = 0;

    bool attach(const wchar_t* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32W pe{ sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (!_wcsicmp(pe.szExeFile, name)) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        if (!pid) return false;
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        return hProcess != nullptr;
    }

    std::uintptr_t module_base(const wchar_t* mod) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        MODULEENTRY32W me{ sizeof(me) };
        std::uintptr_t base = 0;
        if (Module32FirstW(snap, &me)) {
            do {
                if (!_wcsicmp(me.szModule, mod)) {
                    base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return base;
    }

    template<typename T>
    T read(std::uintptr_t addr) {
        T v{};
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(addr), &v, sizeof(T), nullptr);
        return v;
    }

    void close() {
        if (hProcess) { CloseHandle(hProcess); hProcess = nullptr; }
    }
};
