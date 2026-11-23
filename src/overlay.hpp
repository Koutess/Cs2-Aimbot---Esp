// src/overlay.hpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

class Overlay {
public:
    HWND hwnd = nullptr;
    HDC  hdc = nullptr;
    int  w = 0, h = 0;

    static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
        if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
        return DefWindowProcW(h, m, wp, lp);
    }

    bool create() {
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);

        WNDCLASSEXW wc{ sizeof(wc) };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"Valve001";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);

        hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            L"Valve001", L"CS2 Overlay",
            WS_POPUP, 0, 0, w, h,
            nullptr, nullptr, wc.hInstance, nullptr);

        if (!hwnd) return false;
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(hwnd, SW_SHOW);
        hdc = GetDC(hwnd);
        set_click_through(true);
        return true;
    }

    void set_click_through(bool enable) {
        LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if (enable) ex |= WS_EX_TRANSPARENT;
        else        ex &= ~WS_EX_TRANSPARENT;
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    }

    void begin() {
        HBRUSH br = CreateSolidBrush(RGB(0, 0, 0));
        RECT rc{ 0, 0, w, h };
        FillRect(hdc, &rc, br);
        DeleteObject(br);
    }

    void rect(int x, int y, int bw, int bh, COLORREF col, int thick = 1) {
        HPEN pen = CreatePen(PS_SOLID, thick, col);
        auto old = SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + bw, y + bh);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }

    void filled_rect(int x, int y, int bw, int bh, COLORREF col) {
        HBRUSH br = CreateSolidBrush(col);
        RECT rc{ x, y, x + bw, y + bh };
        FillRect(hdc, &rc, br);
        DeleteObject(br);
    }

    void line(int x1, int y1, int x2, int y2, COLORREF col, int thick = 1) {
        HPEN pen = CreatePen(PS_SOLID, thick, col);
        auto old = SelectObject(hdc, pen);
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }

    void text(int x, int y, const std::wstring& s, COLORREF col) {
        SetTextColor(hdc, col);
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, x, y, s.c_str(), (int)s.size());
    }

    void destroy() {
        if (hdc) ReleaseDC(hwnd, hdc);
        if (hwnd) DestroyWindow(hwnd);
    }
};
