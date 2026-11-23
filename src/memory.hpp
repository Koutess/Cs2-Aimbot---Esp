// src/memory.hpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <cstdint>
#include <vector>
#include <string>

#pragma comment(lib, "psapi.lib")

class Memory {
public:
    HANDLE hProcess = nullptr;
    DWORD  pid = 0;
    std::uintptr_t client = 0;
    size_t client_size = 0;

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

    std::uintptr_t module_base(const wchar_t* mod, size_t* out_size = nullptr) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        MODULEENTRY32W me{ sizeof(me) };
        std::uintptr_t base = 0;
        if (Module32FirstW(snap, &me)) {
            do {
                if (!_wcsicmp(me.szModule, mod)) {
                    base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
                    if (out_size) *out_size = me.modBaseSize;
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

    bool read_raw(std::uintptr_t addr, void* buf, size_t size) {
        SIZE_T br = 0;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(addr), buf, size, &br) && br == size;
    }

    // Simple pattern scan (IDA style: "48 8B 0D ? ? ? ? 48 85 C9")
    std::uintptr_t find_pattern(std::uintptr_t start, size_t size, const char* pattern) {
        auto parse = [](const char* pat, std::vector<int>& out) {
            out.clear();
            for (const char* p = pat; *p; ++p) {
                if (*p == ' ') continue;
                if (*p == '?') {
                    out.push_back(-1);
                    if (*(p+1) == '?') ++p;
                } else {
                    char buf[3] = { p[0], p[1], 0 };
                    out.push_back((int)strtoul(buf, nullptr, 16));
                    ++p;
                }
            }
        };

        std::vector<int> bytes;
        parse(pattern, bytes);
        if (bytes.empty()) return 0;

        std::vector<uint8_t> buffer(size);
        if (!read_raw(start, buffer.data(), size)) return 0;

        for (size_t i = 0; i < size - bytes.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < bytes.size(); ++j) {
                if (bytes[j] != -1 && buffer[i + j] != (uint8_t)bytes[j]) {
                    found = false;
                    break;
                }
            }
            if (found) return start + i;
        }
        return 0;
    }

    // Resolve RIP-relative (e.g. 48 8B 0D [rel32])
    std::uintptr_t resolve_rel32(std::uintptr_t instr, int offset = 3, int instr_size = 7) {
        int32_t rel = read<int32_t>(instr + offset);
        return instr + instr_size + rel;
    }

    void close() {
        if (hProcess) { CloseHandle(hProcess); hProcess = nullptr; }
    }
};
