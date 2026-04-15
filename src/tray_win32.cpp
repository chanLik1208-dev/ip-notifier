#ifdef _WIN32
#include "tray.h"
#include "checker.h"

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <mutex>

//----------------------------------------------------------
// Message / menu IDs
//----------------------------------------------------------
#define WM_TRAY_MSG       (WM_APP + 1)
#define ID_TRAY_IP        1000
#define ID_TRAY_SETTINGS  1002
#define ID_TRAY_CHECKNOW  1003
#define ID_TRAY_QUIT      1005

static HWND             g_hwnd   = nullptr;
static NOTIFYICONDATAW  g_nid    = {};
static TrayCallbacks    g_cbs;
static HMENU            g_menu   = nullptr;

//----------------------------------------------------------
// Helpers
//----------------------------------------------------------
static std::wstring s2w(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    return w;
}

//----------------------------------------------------------
// Window procedure
//----------------------------------------------------------
static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAY_MSG:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(g_menu,
                TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                pt.x, pt.y, 0, hwnd, nullptr);
            PostMessageW(hwnd, WM_NULL, 0, 0);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_SETTINGS:
            if (g_cbs.on_open_settings) g_cbs.on_open_settings();
            break;
        case ID_TRAY_CHECKNOW:
            if (g_cbs.on_check_now) g_cbs.on_check_now();
            break;
        case ID_TRAY_QUIT:
            if (g_cbs.on_quit) g_cbs.on_quit();
            break;
        }
        break;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        if (g_menu) { DestroyMenu(g_menu); g_menu = nullptr; }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//----------------------------------------------------------
// Public API
//----------------------------------------------------------
void TrayInit(const TrayCallbacks& callbacks) {
    g_cbs = callbacks;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = TrayWndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"IPNotifierTrayClass";
    RegisterClassExW(&wc);

    g_hwnd = CreateWindowExW(0, L"IPNotifierTrayClass", L"IP Notifier",
                             0, 0, 0, 0, 0, HWND_MESSAGE,
                             nullptr, GetModuleHandleW(nullptr), nullptr);

    // System tray icon
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = g_hwnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY_MSG;
    g_nid.hIcon            = LoadIconW(nullptr, IDI_NETWORK);
    if (!g_nid.hIcon) g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"IP Notifier");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Context menu
    g_menu = CreatePopupMenu();
    {
        std::string ip;
        { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
        std::wstring label = L"IP: " + s2w(ip);
        AppendMenuW(g_menu, MF_STRING | MF_GRAYED, ID_TRAY_IP, label.c_str());
    }
    AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menu, MF_STRING, ID_TRAY_SETTINGS, L"Settings");
    AppendMenuW(g_menu, MF_STRING, ID_TRAY_CHECKNOW, L"Check Now");
    AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g_menu, MF_STRING, ID_TRAY_QUIT,     L"Quit");
}

void TraySetIP(const std::string& ip) {
    if (!g_menu) return;
    std::wstring label = L"IP: " + s2w(ip);
    ModifyMenuW(g_menu, ID_TRAY_IP,
                MF_BYCOMMAND | MF_STRING | MF_GRAYED,
                ID_TRAY_IP, label.c_str());
}

void TrayShowNotification(const std::string& title, const std::string& message) {
    g_nid.uFlags    |= NIF_INFO;
    g_nid.dwInfoFlags = NIIF_INFO;
    wcscpy_s(g_nid.szInfoTitle, s2w(title).substr(0, 63).c_str());
    wcscpy_s(g_nid.szInfo,      s2w(message).substr(0, 255).c_str());
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void TrayRun() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void TrayQuit() {
    if (g_hwnd) PostMessageW(g_hwnd, WM_DESTROY, 0, 0);
}

#endif // _WIN32
