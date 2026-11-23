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
    inline float aim_fov      = 4.0f;
    inline float aim_smooth   = 2.8f;
    inline int   aim_bone     = 6;
    inline int   aim_key      = VK_XBUTTON2;
    inline float max_dist     = 160.f;
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
    bool attached = false;

    bool init() {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);

        printf("[*] Looking for cs2.exe ...\n");
        if (!mem.attach(L"cs2.exe")) {
            printf("[-] cs2.exe not found. Start the game first.\n");
            MessageBoxW(nullptr, L"cs2.exe not found.\nStart Counter-Strike 2 first.", L"CS2 External", MB_ICONERROR);
            return false;
        }
        printf("[+] Attached to PID %u\n", mem.pid);

        mem.client = mem.module_base(L"client.dll");
        if (!mem.client) {
            printf("[-] client.dll base not found\n");
            MessageBoxW(nullptr, L"client.dll not found.", L"CS2 External", MB_ICONERROR);
            return false;
        }
        printf("[+] client.dll @ 0x%llX\n", (unsigned long long)mem.client);

        if (!ov.create()) {
            printf("[-] Overlay creation failed\n");
            return false;
        }
        printf("[+] Overlay ready (%dx%d)\n", ov.w, ov.h);
        printf("[*] INSERT = toggle menu | END = exit\n");
        printf("[*] Hold XBUTTON2 (side mouse) to aim\n");
        printf("[!] Offsets are placeholders — update src/offsets.hpp or you will see 0 players\n");

        attached = true;
        return true;
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

        const int mx = 40, my = 40, mw = 260, mh = 220;
        ov.filled_rect(mx, my, mw, mh, RGB(20, 20, 25));
        ov.rect(mx, my, mw, mh, RGB(0, 180, 255), 1);

        ov.text(mx + 12, my + 10, L"CS2 EXTERNAL", RGB(0, 200, 255));
        ov.text(mx + 12, my + 32, L"INSERT = close menu", RGB(160, 160, 160));
        ov.text(mx + 12, my + 52, L"END    = exit", RGB(160, 160, 160));

        auto onoff = [](bool v) -> std::wstring { return v ? L"[ON] " : L"[OFF]"; };

        ov.text(mx + 12, my + 80,  onoff(cfg::esp_box)      + L" ESP Box      (F1)", cfg::esp_box ? RGB(80,255,80) : RGB(200,80,80));
        ov.text(mx + 12, my + 100, onoff(cfg::esp_health)   + L" Health Bar   (F2)", cfg::esp_health ? RGB(80,255,80) : RGB(200,80,80));
        ov.text(mx + 12, my + 120, onoff(cfg::esp_skeleton) + L" Skeleton     (F3)", cfg::esp_skeleton ? RGB(80,255,80) : RGB(200,80,80));
        ov.text(mx + 12, my + 140, onoff(cfg::aimbot)       + L" Aimbot       (F4)", cfg::aimbot ? RGB(80,255,80) : RGB(200,80,80));
        ov.text(mx + 12, my + 160, onoff(cfg::team_check)   + L" Team Check   (F5)", cfg::team_check ? RGB(80,255,80) : RGB(200,80,80));

        wchar_t buf[64];
        swprintf_s(buf, L"Players: %d", players_found);
        ov.text(mx + 12, my + 190, buf, players_found > 0 ? RGB(80,255,80) : RGB(255,180,0));
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

            if (cfg::esp_box)
                ov.rect(x, y, bw, bh, col, 1);

            if (cfg::esp_health) {
                int bar = (int)(bh * (p.health / 100.f));
                ov.filled_rect(x - 6, y + (bh - bar), 3, bar, RGB(0, 220, 80));
            }

            if (cfg::esp_skeleton) {
                Vec3 neck = bone_pos(p.pawn, 5);
                Vec3 pelvis = bone_pos(p.pawn, 0);
                Vec2 n, pel;
                if (world_to_screen(neck, n, vm, ov.w, ov.h) &&
                    world_to_screen(pelvis, pel, vm, ov.w, ov.h)) {
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
        Vec3 view = mem.read<Vec3>(mem.client + offsets::dwViewAngles);

        float best = cfg::aim_fov;
        Vec3 target{};
        bool found = false;

        for (const auto& p : list) {
            if (eye.dist(p.head) > cfg::max_dist * 40.f) continue;
            Vec3 ang = calc_angle(eye, p.head);
            float fov = get_fov(view, ang);
            if (fov < best) {
                best = fov;
                target = ang;
                found = true;
            }
        }
        if (!found) return;

        float dy = target.y - view.y;
        while (dy >  180.f) dy -= 360.f;
        while (dy < -180.f) dy += 360.f;
        float dx = target.x - view.x;

        dx /= cfg::aim_smooth;
        dy /= cfg::aim_smooth;
        dx += (rand() % 3 - 1) * 0.012f;
        dy += (rand() % 3 - 1) * 0.012f;

        mouse_event(MOUSEEVENTF_MOVE,
            (DWORD)(dy / cfg::mouse_sens),
            (DWORD)(-dx / cfg::mouse_sens),
            0, 0);
    }

    void handle_hotkeys() {
        static bool insert_down = false, f1=false, f2=false, f3=false, f4=false, f5=false;

        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!insert_down) {
                cfg::menu_open = !cfg::menu_open;
                ov.set_click_through(!cfg::menu_open);
                insert_down = true;
            }
        } else insert_down = false;

        if (GetAsyncKeyState(VK_F1) & 0x8000) { if (!f1) { cfg::esp_box = !cfg::esp_box; f1=true; } } else f1=false;
        if (GetAsyncKeyState(VK_F2) & 0x8000) { if (!f2) { cfg::esp_health = !cfg::esp_health; f2=true; } } else f2=false;
        if (GetAsyncKeyState(VK_F3) & 0x8000) { if (!f3) { cfg::esp_skeleton = !cfg::esp_skeleton; f3=true; } } else f3=false;
        if (GetAsyncKeyState(VK_F4) & 0x8000) { if (!f4) { cfg::aimbot = !cfg::aimbot; f4=true; } } else f4=false;
        if (GetAsyncKeyState(VK_F5) & 0x8000) { if (!f5) { cfg::team_check = !cfg::team_check; f5=true; } } else f5=false;

        if (GetAsyncKeyState(VK_END) & 0x8000) {
            PostQuitMessage(0);
        }
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
            // force redraw
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
