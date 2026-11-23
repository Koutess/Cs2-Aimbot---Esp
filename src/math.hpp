// src/math.hpp
#pragma once
#include <cmath>

struct Vec3 {
    float x{}, y{}, z{};
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    float dist(const Vec3& o) const { return (*this - o).length(); }
};

struct Vec2 { float x{}, y{}; };

struct Matrix4x4 { float m[4][4]{}; };

inline bool world_to_screen(const Vec3& world, Vec2& screen, const Matrix4x4& vm, int w, int h) {
    float _w = vm.m[3][0]*world.x + vm.m[3][1]*world.y + vm.m[3][2]*world.z + vm.m[3][3];
    if (_w < 0.001f) return false;
    float x = vm.m[0][0]*world.x + vm.m[0][1]*world.y + vm.m[0][2]*world.z + vm.m[0][3];
    float y = vm.m[1][0]*world.x + vm.m[1][1]*world.y + vm.m[1][2]*world.z + vm.m[1][3];
    screen.x = (w / 2.f) + (w / 2.f) * x / _w;
    screen.y = (h / 2.f) - (h / 2.f) * y / _w;
    return true;
}

inline Vec3 calc_angle(const Vec3& src, const Vec3& dst) {
    Vec3 d = dst - src;
    float hyp = std::sqrt(d.x*d.x + d.y*d.y);
    Vec3 a;
    a.x = -std::atan2(d.z, hyp) * (180.f / 3.14159265f);
    a.y =  std::atan2(d.y, d.x) * (180.f / 3.14159265f);
    if (d.x >= 0.f) a.y += 180.f;
    a.z = 0.f;
    return a;
}

inline float get_fov(const Vec3& view, const Vec3& aim) {
    float dy = aim.y - view.y;
    while (dy > 180.f)  dy -= 360.f;
    while (dy < -180.f) dy += 360.f;
    float dx = aim.x - view.x;
    return std::sqrt(dx*dx + dy*dy);
}
