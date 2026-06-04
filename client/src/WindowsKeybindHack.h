#pragma once

#include "..\Detours\detours.h"

#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

class WindowsKeybindHack
{
public:
    static WindowsKeybindHack* instance()
    {
        static WindowsKeybindHack c;
        return &c;
    }

    void hook(sf::RenderWindow& window)
    {
        HWND hwnd = window.getSystemHandle();
        if (m_hooked && hwnd == m_hwnd)
            return;
        m_hwnd = hwnd;
        m_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)customWndProc);

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)RealGetAsyncKeyState, HookedGetAsyncKeyState);
        DetourAttach(&(PVOID&)RealJoyGetPosEx, HookedJoyGetPosEx);
        DetourAttach(&(PVOID&)RealJoyGetDevCapsW, HookedJoyGetDevCapsW);
        DetourTransactionCommit();

        m_hooked = true;
    }

    void unhook()
    {
        if (!m_hooked)
            return;

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)RealGetAsyncKeyState, HookedGetAsyncKeyState);
        DetourDetach(&(PVOID&)RealJoyGetPosEx, HookedJoyGetPosEx);
        DetourDetach(&(PVOID&)RealJoyGetDevCapsW, HookedJoyGetDevCapsW);
        DetourTransactionCommit();

        if (m_hwnd && m_originalWndProc)
            SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_originalWndProc);

        m_hooked = false;
    }

    // Call once per frame as a safety net
    void validate()
    {
        if (!(RealGetAsyncKeyState(VK_MBUTTON) & 0x8000))
            m_fakeKeyState[VK_F13] = false;
        if (!(RealGetAsyncKeyState(VK_XBUTTON1) & 0x8000))
            m_fakeKeyState[VK_F14] = false;
        if (!(RealGetAsyncKeyState(VK_XBUTTON2) & 0x8000))
            m_fakeKeyState[VK_F15] = false;
    }

private:
    WindowsKeybindHack() :
        m_hooked(false),
        m_originalWndProc(nullptr),
        m_hwnd(nullptr)
    {
        memset(m_fakeKeyState, 0, sizeof(m_fakeKeyState));
    }

    ~WindowsKeybindHack()
    {
        unhook();
    }

    // Keyboard hook
    static inline SHORT(WINAPI* RealGetAsyncKeyState)(int vKey) = GetAsyncKeyState;

    static SHORT WINAPI HookedGetAsyncKeyState(int vKey)
    {
        if (vKey == VK_F13 || vKey == VK_F14 || vKey == VK_F15 || vKey == VK_PAUSE)
        {
            if (instance()->m_fakeKeyState[vKey])
                return (SHORT)0x8000;
            return 0;
        }
        return RealGetAsyncKeyState(vKey);
    }

    // Joystick hooks - make SFML think no joysticks exist
    static inline MMRESULT(WINAPI* RealJoyGetPosEx)(UINT uJoyID, LPJOYINFOEX pji) = joyGetPosEx;
    static inline MMRESULT(WINAPI* RealJoyGetDevCapsW)(UINT uJoyID, LPJOYCAPSW pjc, UINT cbjc) = joyGetDevCapsW;

    static MMRESULT WINAPI HookedJoyGetPosEx(UINT uJoyID, LPJOYINFOEX pji)
    {
        return JOYERR_PARMS;
    }

    static MMRESULT WINAPI HookedJoyGetDevCapsW(UINT uJoyID, LPJOYCAPSW pjc, UINT cbjc)
    {
        return JOYERR_PARMS;
    }

    bool m_fakeKeyState[256];

    static void setFakeKey(WORD vk, bool down)
    {
        instance()->m_fakeKeyState[vk] = down;
    }

    static void sendFakeKey(HWND hwnd, WORD vk, bool down)
    {
        setFakeKey(vk, down);

        UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
        LPARAM lParam = 1 | (scanCode << 16);
        if (!down)
            lParam |= (1 << 30) | (1 << 31);

        PostMessage(hwnd, down ? WM_KEYDOWN : WM_KEYUP, vk, lParam);
    }

    static void clearAllFakeKeys()
    {
        memset(instance()->m_fakeKeyState, 0, sizeof(instance()->m_fakeKeyState));
    }

    static LRESULT CALLBACK customWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_KILLFOCUS:
        {
            clearAllFakeKeys();
            break;
        }
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            WORD vk = 0;
            if (msg == WM_MBUTTONDOWN)
                vk = VK_F13;
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                vk = VK_F14;
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
                vk = VK_F15;
            if (vk)
                sendFakeKey(hwnd, vk, true);
            break;
        }
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            WORD vk = 0;
            if (msg == WM_MBUTTONUP)
                vk = VK_F13;
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                vk = VK_F14;
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
                vk = VK_F15;
            if (vk)
                sendFakeKey(hwnd, vk, false);
            break;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            if (wParam == VK_CAPITAL)
            {
                sendFakeKey(hwnd, VK_PAUSE, msg == WM_KEYDOWN);
                return 0;
            }
            break;
        }
        }
        return CallWindowProc(instance()->m_originalWndProc, hwnd, msg, wParam, lParam);
    }

    bool m_hooked;
    WNDPROC m_originalWndProc;
    HWND m_hwnd;
};

#define sKeybindHack WindowsKeybindHack::instance()