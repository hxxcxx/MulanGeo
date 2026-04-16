/**
 * @file Win32Window.h
 * @brief Win32 原生窗口，IWindow 的 Windows 平台实现
 * @author hxxcxx
 * @date 2026-04-16
 *
 * 直接创建 HWND，可脱离 Qt / SDL / GLFW 独立使用。
 * IWindow 只定义窗口属性（句柄、尺寸），事件循环由外部主循环负责。
 */

#pragma once

#include "../Window.h"

#ifdef _WIN32

#include <string>

namespace MulanGeo::Engine {

class Win32Window : public IWindow {
public:
    struct Desc {
        std::wstring title     = L"MulanGeo";
        uint32_t     width     = 1280;
        uint32_t     height    = 720;
        bool         resizable = true;
        RenderConfig renderConfig = {};
    };

    explicit Win32Window(const Desc& desc = {})
        : m_width(desc.width)
        , m_height(desc.height)
    {
        renderConfig = desc.renderConfig;
        registerClass();
        createWindow(desc);
    }

    ~Win32Window() override {
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    // --- IWindow 接口 ---

    NativeWindowHandle nativeHandle() const override {
        return NativeWindowHandle::makeWin32(
            reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),
            reinterpret_cast<uintptr_t>(m_hwnd));
    }

    uint32_t width()  const override { return m_width; }
    uint32_t height() const override { return m_height; }

    // --- Win32 特有（外部主循环使用，不是 IWindow 接口）---

    bool shouldClose() const { return m_shouldClose; }

    void pollEvents() {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void close() { m_shouldClose = true; }

    HWND hwnd() const { return m_hwnd; }

private:
    static constexpr const wchar_t* kClassName = L"MulanGeoWindow";

    void registerClass() {
        static bool registered = false;
        if (registered) return;

        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = &WndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = kClassName;

        RegisterClassExW(&wc);
        registered = true;
    }

    void createWindow(const Desc& desc) {
        DWORD style = WS_OVERLAPPEDWINDOW;
        if (!desc.resizable) {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        RECT rc = { 0, 0, (LONG)desc.width, (LONG)desc.height };
        AdjustWindowRect(&rc, style, FALSE);

        m_hwnd = CreateWindowExW(
            0,
            kClassName,
            desc.title.c_str(),
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left,
            rc.bottom - rc.top,
            nullptr, nullptr,
            GetModuleHandleW(nullptr),
            this);   // 传 this 给 WM_NCCREATE

        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hwnd);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Win32Window* self = nullptr;

        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<Win32Window*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->m_hwnd = hwnd;
        } else {
            self = reinterpret_cast<Win32Window*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self) {
            switch (msg) {
            case WM_SIZE:
                self->m_width  = LOWORD(lParam);
                self->m_height = HIWORD(lParam);
                self->notifyResize(self->m_width, self->m_height);
                return 0;

            case WM_CLOSE:
                self->m_shouldClose = true;
                return 0;

            case WM_DESTROY:
                self->m_shouldClose = true;
                return 0;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    HWND     m_hwnd         = nullptr;
    uint32_t m_width        = 0;
    uint32_t m_height       = 0;
    bool     m_shouldClose  = false;
};

} // namespace MulanGeo::Engine

#endif // _WIN32
