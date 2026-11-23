// src/cheat.hpp
#pragma once
#include "memory.hpp"
#include "offsets.hpp"
#include "math.hpp"
#include "overlay.hpp"
#include <vector>
#include <string>
#include <cmath>

namespace cfg {
    inline bool  esp_box      = true;
    inline bool  esp_skeleton = true;
    inline bool  esp_health   = true;
    inline bool  esp_name     = false;
    inline bool  aimbot       = true;
    inline bool  team_check   = true;
    inline float aim_fov      = 3.8f;
    inline float aim_smooth   = 2.6f;
    inline int   aim_bone     = 6;          // head
    inline int   aim_key      = VK_XBUTTON2;
    inline float max_dist     = 150.f;
    inline float mouse_sens   = 0.022f;     // match your sens * m_yaw roughly
}

struct Player {
    std::uintptr_t pawn{};
    Vec3 origin{}, head{};
    int health{}, team{};
    bool dormant{};
};

class Cheat {
public:
    Memory  mem;
    Overlay overlay;
    int screen_w = 0, screen_h = 0;

    bool init() {
        if (!mem.attach(L"cs2.exe")) return false;
        mem.client = mem.module_base(L"client.dll");
        if (!mem.client) return false;
        if (!overlay.create()) return false;
        screen_w = overlay.width;
        screen_h = overlay.height;
        return true;
    }

    std::uintptr_t entity_list() {
        return mem.read<std::uintptr_t>(mem.client + offsets::dwEntityList);
    }

    std::uintptr_t local_pawn() {
        return mem.read<std::uintptr_t>(mem.client + offsets::dwLocalPlayerPawn);
    }

    Matrix4x4 view_matrix() {
        return mem.read<Matrix4x4>(mem.client + offsets::dwViewMatrix);
    }

    Vec3 bone_pos(std::uintptr_t pawn, int bone) {
        auto scene = mem.read<std::uintptr_t>(pawn + offsets::m_pGameSceneNode);
        if (!scene) return {};
        auto boneArray = mem.read<std::uintptr_t>(scene + offsets::m_modelState + offsets::m_boneArray);
        if (!boneArray) return {};
        return mem.read<Vec3>(boneArray + bone * 32);
    }

    std::vector<Player> collect() {
        std::vector<Player> out;
        auto local = local_pawn();
        if (!local) return out;

        int localTeam = mem.read<int>(local + offsets::m_iTeamNum);
        auto list = entity_list();

        for (int i = 1; i < 64; ++i) {
            auto entry1 = mem.read<std::uintptr_t>(list + (8 * ((i & 0x7FFF) >> 9)) + 16);
            if (!entry1) continue;
            auto controller = mem.read<std::uintptr_t>(entry1 + 120 * (i & 0x1FF));
            if (!controller) continue;

            uint32_t handle = mem.read<uint32_t>(controller + offsets::m_hPlayerPawn);
            if (!handle) continue;

            auto entry2 = mem.read<std::uintptr_t>(list + 0x8 * ((handle & 0x7FFF) >> 9) + 16);
            if (!entry2) continue;
            auto pawn = mem.read<std::uintptr_t>(entry2 + 120 * (handle & 0x1FF));
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

            out.push_back({ pawn, org, head, hp, team, dorm });
        }
        return out;
    }

    void draw_esp(const std::vector<Player>& players, const Matrix4x4& vm) {
        for (const auto& p : players) {
            Vec2 feet, head;
            if (!world_to_screen(p.origin, feet, vm, screen_w, screen_h)) continue;
            if (!world_to_screen(p.head, head, vm, screen_w, screen_h)) continue;

            float h = feet.y - head.y;
            float w = h * 0.42f;
            int x = static_cast<int>(head.x - w / 2);
            int y = static_cast<int>(head.y);
            int bw = static_cast<int>(w);
            int bh = static_cast<int>(h);

            COLORREF col = (p.team == 2) ? RGB(255, 80, 80) : RGB(80, 160, 255);

            if (cfg::esp_box) {
                overlay.rect(x, y, bw, bh, col, 1);
            }

            if (cfg::esp_health) {
                int bar_h = static_cast<int>(bh * (p.health / 100.f));
                overlay.rect(x - 6, y + (bh - bar_h), 3, bar_h, RGB(0, 255, 80), 1);
            }

            if (cfg::esp_skeleton) {
                // simple 5-bone stick for visibility
                Vec3 neck = bone_pos(p.pawn, 5);
                Vec3 pelvis = bone_pos(p.pawn, 0);
                Vec2 n, pel;
                if (world_to_screen(neck, n, vm, screen_w, screen_h) &&
                    world_to_screen(pelvis, pel, vm, screen_w, screen_h)) {
                    overlay.line(static_cast<int>(head.x), static_cast<int>(head.y),
                                 static_cast<int>(n.x), static_cast<int>(n.y), col);
                    overlay.line(static_cast<int>(n.x), static_cast<int>(n.y),
                                 static_cast<int>(pel.x), static_cast<int>(pel.y), col);
                }
            }
        }
    }

    void aim(const std::vector<Player>& players) {
        if (!(GetAsyncKeyState(cfg::aim_key) & 0x8000)) return;

        auto local = local_pawn();
        if (!local) return;

        Vec3 origin = mem.read<Vec3>(local + offsets::m_vOldOrigin);
        Vec3 viewOff = mem.read<Vec3>(local + offsets::m_vecViewOffset);
        Vec3 eye = { origin.x + viewOff.x, origin.y + viewOff.y, origin.z + viewOff.z };

        // view angles — if this offset is wrong the aim will be off; update it
        Vec3 view = mem.read<Vec3>(mem.client + offsets::dwViewAngles);

        float best = cfg::aim_fov;
        Vec3  target{};
        bool  hit = false;

        for (const auto& p : players) {
            if (eye.dist(p.head) > cfg::max_dist * 40.f) continue;
            Vec3 ang = calc_angle(eye, p.head);
            float fov = get_fov(view, ang);
            if (fov < best) {
                best = fov;
                target = ang;
                hit = true;
            }
        }
        if (!hit) return;

        float dy = target.y - view.y;
        while (dy > 180.f)  dy -= 360.f;
        while (dy < -180.f) dy += 360.f;
        float dx = target.x - view.x;

        // humanized smooth + small jitter
        dx /= cfg::aim_smooth;
        dy /= cfg::aim_smooth;
        dx += (rand() % 3 - 1) * 0.015f;
        dy += (rand() % 3 - 1) * 0.015f;

        mouse_event(MOUSEEVENTF_MOVE,
                    static_cast<DWORD>(dy / cfg::mouse_sens),
                    static_cast<DWORD>(-dx / cfg::mouse_sens),
                    0, 0);
    }

    void run() {
        while (true) {
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            if (!mem.hProcess) break;

            auto players = collect();
            auto vm = view_matrix();

            overlay.begin();
            draw_esp(players, vm);
            if (cfg::aimbot) aim(players);
            overlay.end();

            // randomized low delay
            Sleep(1 + (rand() % 2));
        }
    }

    void shutdown() {
        overlay.destroy();
        mem.close();
    }
};
