// src/overlay.hpp
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include "math.hpp"

class Overlay {
public:
    HWND hwnd = nullptr;
    HDC  hdc = nullptr;
    int  width = 0, height = 0;

    bool create(const wchar_t* className = L"CEF-OSC-WIDGET") { // generic looking name
        width  = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);

        WNDCLASSEXW wc{ sizeof(wc) };
        wc.lpfnWndProc   = DefWindowProcW;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.lpszClassName = className;
        RegisterClassExW(&wc);

        hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            className, L"",
            WS_POPUP,
            0, 0, width, height,
            nullptr, nullptr, wc.hInstance, nullptr);

        if (!hwnd) return false;

        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(hwnd, SW_SHOW);
        hdc = GetDC(hwnd);
        return true;
    }

    void begin() {
        // clear with color key
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        RECT rc{ 0, 0, width, height };
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
    }

    void end() {
        // nothing special for GDI color-key
    }

    void rect(int x, int y, int w, int h, COLORREF col, int thickness = 1) {
        HPEN pen = CreatePen(PS_SOLID, thickness, col);
        HGDIOBJ old = SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x, y, x + w, y + h);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }

    void line(int x1, int y1, int x2, int y2, COLORREF col, int thickness = 1) {
        HPEN pen = CreatePen(PS_SOLID, thickness, col);
        HGDIOBJ old = SelectObject(hdc, pen);
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }

    void text(int x, int y, const std::wstring& str, COLORREF col) {
        SetTextColor(hdc, col);
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, x, y, str.c_str(), static_cast<int>(str.size()));
    }

    void destroy() {
        if (hdc) ReleaseDC(hwnd, hdc);
        if (hwnd) DestroyWindow(hwnd);
    }
};
