// src/cheat.hpp
#pragma once
#include "memory.hpp"
#include "offsets.hpp"
#include "math.hpp"
#include "overlay.hpp"
#include <vector>
#include <string>
#include <cstdio>

namespace cfg {
    inline bool  menu_open    = true;
    inline bool  esp_box      = true;
    inline bool  esp_health   = true;
    inline bool  esp_skeleton = false;
    inline bool  aimbot       = true;
    inline bool  team_check   = true;
    inline float aim_fov      = 4.5f;
    inline float aim_smooth   = 2.4f;
    inline int   aim_bone     = 6;
    inline int   aim_key      = VK_XBUTTON2;
    inline float max_dist     = 180.f;
    inline float mouse_sens   = 0.022f;
}

struct Player {
    std::uintptr_t pawn{};
    Vec3 origin{}, head{};
    int health{}, team{};
};

class Cheat {
public:
    Memory  mem;
    Overlay ov;
    int players_found = 0;
    bool patterns_ok = false;

    bool init() {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        printf("[*] Waiting for cs2.exe ...\n");
        while (!mem.attach(L"cs2.exe")) {
            Sleep(1000);
        }
        printf("[+] Attached (PID %u)\n", mem.pid);

        mem.client = mem.module_base(L"client.dll", &mem.client_size);
        if (!mem.client) {
            printf("[-] client.dll not found\n");
            return false;
        }
        printf("[+] client.dll @ 0x%llX  size 0x%zX\n", (unsigned long long)mem.client, mem.client_size);

        if (!resolve_offsets()) {
            printf("[-] Pattern scan failed. Game may have updated hard.\n");
            MessageBoxW(nullptr, L"Pattern scan failed.\nGame update broke the signatures.", L"CS2 External", MB_ICONERROR);
            return false;
        }
        patterns_ok = true;

        if (!ov.create()) return false;
        printf("[+] Overlay ready\n");
        printf("[*] INSERT = menu | END = exit | XBUTTON2 = aim\n");
        return true;
    }

    bool resolve_offsets() {
        printf("[*] Scanning patterns ...\n");

        // These patterns are the common stable ones used by public externals.
        // They still break on big updates, but much less often than raw offsets.
        // dwEntityList
        auto e1 = mem.find_pattern(mem.client, mem.client_size,
            "48 8B 0D ? ? ? ? 48 89 7C 24 ? 8B FA C1 EB");
        if (e1) {
            offsets::dwEntityList = mem.resolve_rel32(e1) - mem.client;
            printf("  dwEntityList        = 0x%llX\n", (unsigned long long)offsets::dwEntityList);
        }

        // dwLocalPlayerPawn
        auto e2 = mem.find_pattern(mem.client, mem.client_size,
            "48 8B 05 ? ? ? ? 48 85 C0 74 0C 8B 40");
        if (e2) {
            offsets::dwLocalPlayerPawn = mem.resolve_rel32(e2) - mem.client;
            printf("  dwLocalPlayerPawn   = 0x%llX\n", (unsigned long long)offsets::dwLocalPlayerPawn);
        }

        // dwViewMatrix
        auto e3 = mem.find_pattern(mem.client, mem.client_size,
            "48 8D 0D ? ? ? ? 48 C1 E0 06");
        if (e3) {
            offsets::dwViewMatrix = mem.resolve_rel32(e3) - mem.client;
            printf("  dwViewMatrix        = 0x%llX\n", (unsigned long long)offsets::dwViewMatrix);
        }

        // Fallback / secondary patterns if the first ones miss
        if (!offsets::dwEntityList) {
            e1 = mem.find_pattern(mem.client, mem.client_size, "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B F0");
            if (e1) offsets::dwEntityList = mem.resolve_rel32(e1) - mem.client;
        }
        if (!offsets::dwLocalPlayerPawn) {
            e2 = mem.find_pattern(mem.client, mem.client_size, "48 8B 05 ? ? ? ? 48 85 C0 74 ? 8B 40 ? 24");
            if (e2) offsets::dwLocalPlayerPawn = mem.resolve_rel32(e2) - mem.client;
        }
        if (!offsets::dwViewMatrix) {
            e3 = mem.find_pattern(mem.client, mem.client_size, "48 8D 0D ? ? ? ? 48 C1 E0 06 48 03 C1");
            if (e3) offsets::dwViewMatrix = mem.resolve_rel32(e3) - mem.client;
        }

        bool ok = offsets::dwEntityList && offsets::dwLocalPlayerPawn && offsets::dwViewMatrix;
        if (ok) printf("[+] All critical offsets resolved\n");
        else    printf("[-] Missing critical offsets\n");
        return ok;
    }

    std::uintptr_t local_pawn() {
        return mem.read<std::uintptr_t>(mem.client + offsets::dwLocalPlayerPawn);
    }

    std::uintptr_t entity_list() {
        return mem.read<std::uintptr_t>(mem.client + offsets::dwEntityList);
    }

    Matrix4x4 view_matrix() {
        return mem.read<Matrix4x4>(mem.client + offsets::dwViewMatrix);
    }

    Vec3 bone_pos(std::uintptr_t pawn, int bone) {
        auto scene = mem.read<std::uintptr_t>(pawn + offsets::m_pGameSceneNode);
        if (!scene) return {};
        auto arr = mem.read<std::uintptr_t>(scene + offsets::m_modelState + offsets::m_boneArray);
        if (!arr) return {};
        return mem.read<Vec3>(arr + bone * 32);
    }

    std::vector<Player> collect() {
        std::vector<Player> out;
        auto local = local_pawn();
        if (!local) return out;

        int localTeam = mem.read<int>(local + offsets::m_iTeamNum);
        auto list = entity_list();
        if (!list) return out;

        for (int i = 1; i < 64; ++i) {
            auto e1 = mem.read<std::uintptr_t>(list + (8 * ((i & 0x7FFF) >> 9)) + 16);
            if (!e1) continue;
            auto controller = mem.read<std::uintptr_t>(e1 + 120 * (i & 0x1FF));
            if (!controller) continue;

            uint32_t handle = mem.read<uint32_t>(controller + offsets::m_hPlayerPawn);
            if (!handle) continue;

            auto e2 = mem.read<std::uintptr_t>(list + 0x8 * ((handle & 0x7FFF) >> 9) + 16);
            if (!e2) continue;
            auto pawn = mem.read<std::uintptr_t>(e2 + 120 * (handle & 0x1FF));
            if (!pawn || pawn == local) continue;

            int hp = mem.read<int>(pawn + offsets::m_iHealth);
            if (hp <= 0 || hp > 100) continue;

            int team = mem.read<int>(pawn + offsets::m_iTeamNum);
            if (cfg::team_check && team == localTeam) continue;

            bool dorm = mem.read<bool>(pawn + offsets::m_bDormant);
            if (dorm) continue;

            Vec3 org = mem.read<Vec3>(pawn + offsets::m_vOldOrigin);
            Vec3 head = bone_pos(pawn, cfg::aim_bone);
            if (head.x == 0.f && head.y == 0.f && head.z == 0.f)
                head = { org.x, org.y, org.z + 72.f };

            out.push_back({ pawn, org, head, hp, team });
        }
        players_found = (int)out.size();
        return out;
    }

    void draw_menu() {
        if (!cfg::menu_open) return;
        const int mx = 40, my = 40, mw = 280, mh = 240;
        ov.filled_rect(mx, my, mw, mh, RGB(15, 15, 20));
        ov.rect(mx, my, mw, mh, RGB(0, 190, 255), 1);

        ov.text(mx + 12, my + 10, L"CS2 EXTERNAL  (AUTO)", RGB(0, 200, 255));
        ov.text(mx + 12, my + 32, L"INSERT = toggle menu", RGB(150, 150, 150));
        ov.text(mx + 12, my + 50, L"END    = exit", RGB(150, 150, 150));

        auto on = [](bool v) -> std::wstring { return v ? L"[ON]  " : L"[OFF] "; };
        ov.text(mx + 12, my + 80,  on(cfg::esp_box)      + L"ESP Box     F1", cfg::esp_box ? RGB(80,255,80) : RGB(220,80,80));
        ov.text(mx + 12, my + 100, on(cfg::esp_health)   + L"Health      F2", cfg::esp_health ? RGB(80,255,80) : RGB(220,80,80));
        ov.text(mx + 12, my + 120, on(cfg::esp_skeleton) + L"Skeleton    F3", cfg::esp_skeleton ? RGB(80,255,80) : RGB(220,80,80));
        ov.text(mx + 12, my + 140, on(cfg::aimbot)       + L"Aimbot      F4", cfg::aimbot ? RGB(80,255,80) : RGB(220,80,80));
        ov.text(mx + 12, my + 160, on(cfg::team_check)   + L"Team Check  F5", cfg::team_check ? RGB(80,255,80) : RGB(220,80,80));

        wchar_t buf[64];
        swprintf_s(buf, L"Players visible: %d", players_found);
        ov.text(mx + 12, my + 195, buf, players_found > 0 ? RGB(80,255,80) : RGB(255,180,0));
        ov.text(mx + 12, my + 215, patterns_ok ? L"Patterns: OK" : L"Patterns: FAIL", patterns_ok ? RGB(80,255,80) : RGB(255,80,80));
    }

    void draw_esp(const std::vector<Player>& list, const Matrix4x4& vm) {
        for (const auto& p : list) {
            Vec2 feet, head;
            if (!world_to_screen(p.origin, feet, vm, ov.w, ov.h)) continue;
            if (!world_to_screen(p.head, head, vm, ov.w, ov.h)) continue;

            float height = feet.y - head.y;
            float width  = height * 0.42f;
            int x = (int)(head.x - width / 2);
            int y = (int)head.y;
            int bw = (int)width;
            int bh = (int)height;
            COLORREF col = (p.team == 2) ? RGB(255, 70, 70) : RGB(70, 160, 255);

            if (cfg::esp_box) ov.rect(x, y, bw, bh, col, 1);
            if (cfg::esp_health) {
                int bar = (int)(bh * (p.health / 100.f));
                ov.filled_rect(x - 6, y + (bh - bar), 3, bar, RGB(0, 220, 80));
            }
            if (cfg::esp_skeleton) {
                Vec3 neck = bone_pos(p.pawn, 5);
                Vec3 pelvis = bone_pos(p.pawn, 0);
                Vec2 n, pel;
                if (world_to_screen(neck, n, vm, ov.w, ov.h) && world_to_screen(pelvis, pel, vm, ov.w, ov.h)) {
                    ov.line((int)head.x, (int)head.y, (int)n.x, (int)n.y, col);
                    ov.line((int)n.x, (int)n.y, (int)pel.x, (int)pel.y, col);
                }
            }
        }
    }

    void aim(const std::vector<Player>& list) {
        if (!(GetAsyncKeyState(cfg::aim_key) & 0x8000)) return;
        auto local = local_pawn();
        if (!local) return;

        Vec3 origin = mem.read<Vec3>(local + offsets::m_vOldOrigin);
        Vec3 viewOff = mem.read<Vec3>(local + offsets::m_vecViewOffset);
        Vec3 eye = { origin.x + viewOff.x, origin.y + viewOff.y, origin.z + viewOff.z };

        // We don't have a reliable viewangles offset auto-resolved in this version,
        // so we use a pure mouse delta toward the target (works without viewangles).
        float bestFov = cfg::aim_fov * 20.f; // screen-space approximation
        Vec2  bestDelta{};
        bool  found = false;

        Matrix4x4 vm = view_matrix();
        Vec2 center = { ov.w / 2.f, ov.h / 2.f };

        for (const auto& p : list) {
            if (eye.dist(p.head) > cfg::max_dist * 40.f) continue;
            Vec2 screen;
            if (!world_to_screen(p.head, screen, vm, ov.w, ov.h)) continue;

            float dx = screen.x - center.x;
            float dy = screen.y - center.y;
            float fov = std::sqrt(dx*dx + dy*dy);
            if (fov < bestFov) {
                bestFov = fov;
                bestDelta = { dx, dy };
                found = true;
            }
        }
        if (!found) return;

        // smooth + micro jitter
        bestDelta.x /= cfg::aim_smooth;
        bestDelta.y /= cfg::aim_smooth;
        bestDelta.x += (rand() % 3 - 1) * 0.4f;
        bestDelta.y += (rand() % 3 - 1) * 0.4f;

        mouse_event(MOUSEEVENTF_MOVE, (DWORD)bestDelta.x, (DWORD)bestDelta.y, 0, 0);
    }

    void handle_hotkeys() {
        static bool ins=false, f1=false, f2=false, f3=false, f4=false, f5=false;
        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!ins) { cfg::menu_open = !cfg::menu_open; ov.set_click_through(!cfg::menu_open); ins=true; }
        } else ins=false;

        if (GetAsyncKeyState(VK_F1) & 0x8000) { if (!f1) { cfg::esp_box=!cfg::esp_box; f1=true; } } else f1=false;
        if (GetAsyncKeyState(VK_F2) & 0x8000) { if (!f2) { cfg::esp_health=!cfg::esp_health; f2=true; } } else f2=false;
        if (GetAsyncKeyState(VK_F3) & 0x8000) { if (!f3) { cfg::esp_skeleton=!cfg::esp_skeleton; f3=true; } } else f3=false;
        if (GetAsyncKeyState(VK_F4) & 0x8000) { if (!f4) { cfg::aimbot=!cfg::aimbot; f4=true; } } else f4=false;
        if (GetAsyncKeyState(VK_F5) & 0x8000) { if (!f5) { cfg::team_check=!cfg::team_check; f5=true; } } else f5=false;
        if (GetAsyncKeyState(VK_END) & 0x8000) PostQuitMessage(0);
    }

    void run() {
        while (true) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) return;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (!mem.hProcess) break;

            handle_hotkeys();
            auto list = collect();
            auto vm = view_matrix();

            ov.begin();
            draw_esp(list, vm);
            if (cfg::aimbot) aim(list);
            draw_menu();
            InvalidateRect(ov.hwnd, nullptr, FALSE);
            Sleep(1);
        }
    }

    void shutdown() {
        ov.destroy();
        mem.close();
        FreeConsole();
    }
};
