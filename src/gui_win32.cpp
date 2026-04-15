#ifdef _WIN32
#include "gui.h"
#include "config.h"
#include "checker.h"

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <functional>
#include <mutex>
#include <atomic>

// ---------------------------------------------------------------------------
// IDs
// ---------------------------------------------------------------------------
#define ID_IP_LABEL      3001
#define ID_WEBHOOK_EDIT  3003
#define ID_INTERVAL_EDIT 3004
#define ID_ENABLE_CHECK  3005
#define ID_SAVE_BTN      3006
#define ID_REFRESH_BTN   3007

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static Config*                g_cfg        = nullptr;
static std::function<void()>  g_on_changed;
static HWND                   g_win        = nullptr;

// Design tokens
static HBRUSH  g_bg_brush   = nullptr;
static HFONT   g_font_title = nullptr;
static HFONT   g_font_norm  = nullptr;
static HFONT   g_font_ip    = nullptr;

// Colors  (#0f172a background, #f8fafc text, #6366f1 accent)
static const COLORREF BG_COLOR   = RGB(15,  23,  42);
static const COLORREF TEXT_COLOR = RGB(248, 250, 252);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::wstring s2w(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w;
}
static std::string w2s(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
    return s;
}
static std::wstring GetEditText(HWND hwnd, int id) {
    wchar_t buf[1024] = {};
    GetDlgItemTextW(hwnd, id, buf, 1024);
    return buf;
}

static void CreateFonts() {
    g_font_title = CreateFontW(22, 0, 0, 0, FW_BOLD,    FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_font_norm  = CreateFontW(16, 0, 0, 0, FW_NORMAL,  FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_font_ip    = CreateFontW(30, 0, 0, 0, FW_BOLD,    FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
}
static void SetFont(HWND ctrl, HFONT f) {
    SendMessageW(ctrl, WM_SETFONT, (WPARAM)f, TRUE);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    // Dark background
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, g_bg_brush);
        return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, BG_COLOR);
        SetTextColor(hdc, TEXT_COLOR);
        return (LRESULT)g_bg_brush;
    }
    case WM_CTLCOLORBTN:
        return (LRESULT)g_bg_brush;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ENABLE_CHECK:
            // BS_AUTOCHECKBOX handles toggle automatically
            break;

        case ID_REFRESH_BTN: {
            std::string ip;
            { std::lock_guard<std::mutex> lk(g_status_mutex); ip = g_status.current_ip; }
            SetDlgItemTextW(hwnd, ID_IP_LABEL, s2w(ip).c_str());
            break;
        }
        case ID_SAVE_BTN: {
            if (!g_cfg) break;
            g_cfg->webhook_url            = w2s(GetEditText(hwnd, ID_WEBHOOK_EDIT));
            g_cfg->check_interval_minutes = _wtoi(GetEditText(hwnd, ID_INTERVAL_EDIT).c_str());
            if (g_cfg->check_interval_minutes <= 0) g_cfg->check_interval_minutes = 1;
            g_cfg->enable_webhook =
                (IsDlgButtonChecked(hwnd, ID_ENABLE_CHECK) == BST_CHECKED);

            SaveConfig(*g_cfg);
            if (g_on_changed) g_on_changed();

            MessageBoxW(hwnd, L"Settings saved!", L"IP Notifier",
                        MB_OK | MB_ICONINFORMATION);
            break;
        }
        }
        break;

    case WM_DESTROY:
        if (g_font_title) { DeleteObject(g_font_title); g_font_title = nullptr; }
        if (g_font_norm)  { DeleteObject(g_font_norm);  g_font_norm  = nullptr; }
        if (g_font_ip)    { DeleteObject(g_font_ip);    g_font_ip    = nullptr; }
        if (g_bg_brush)   { DeleteObject(g_bg_brush);   g_bg_brush   = nullptr; }
        g_win = nullptr;
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void ShowSettingsWindow(Config* config, std::function<void()> on_config_changed) {
    if (g_win) { SetForegroundWindow(g_win); return; }

    g_cfg        = config;
    g_on_changed = on_config_changed;

    g_bg_brush   = CreateSolidBrush(BG_COLOR);
    CreateFonts();

    static bool reg = false;
    if (!reg) {
        WNDCLASSEXW wc = {};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = SettingsWndProc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hbrBackground = g_bg_brush;
        wc.lpszClassName = L"IPNotifierSettings";
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
        reg = true;
    }

    // Center window on screen
    const int WW = 500, WH = 480;
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);

    g_win = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"IPNotifierSettings",
        L"IP Notifier — Settings",
        (WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME) | WS_VISIBLE,
        (sx - WW) / 2, (sy - WH) / 2, WW, WH,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

    //-- Layout constants ----------
    const int X = 22, W = WW - 44;
    int y = 18;

    // Title
    auto* hTitle = CreateWindowW(L"STATIC", L"IP Address Notifier",
        WS_CHILD | WS_VISIBLE, X, y, W, 32, g_win, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    SetFont(hTitle, g_font_title);
    y += 44;

    // Current IP
    std::string cur;
    { std::lock_guard<std::mutex> lk(g_status_mutex); cur = g_status.current_ip; }
    auto* hIP = CreateWindowW(L"STATIC", s2w(cur).c_str(),
        WS_CHILD | WS_VISIBLE, X, y, W, 38, g_win,
        (HMENU)(UINT_PTR)ID_IP_LABEL, GetModuleHandleW(nullptr), nullptr);
    SetFont(hIP, g_font_ip);
    y += 52;

    // Webhook label + edit
    auto* lbWH = CreateWindowW(L"STATIC", L"Discord Webhook URL:",
        WS_CHILD | WS_VISIBLE, X, y, W, 20, g_win, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    SetFont(lbWH, g_font_norm);
    y += 24;
    auto* edWH = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
        s2w(config->webhook_url).c_str(),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        X, y, W, 28, g_win,
        (HMENU)(UINT_PTR)ID_WEBHOOK_EDIT, GetModuleHandleW(nullptr), nullptr);
    SetFont(edWH, g_font_norm);
    y += 40;

    // Interval label + edit
    auto* lbIN = CreateWindowW(L"STATIC", L"Check interval (minutes):",
        WS_CHILD | WS_VISIBLE, X, y, 240, 20, g_win, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    SetFont(lbIN, g_font_norm);
    y += 24;
    auto* edIN = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT",
        std::to_wstring(config->check_interval_minutes).c_str(),
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        X, y, 100, 28, g_win,
        (HMENU)(UINT_PTR)ID_INTERVAL_EDIT, GetModuleHandleW(nullptr), nullptr);
    SetFont(edIN, g_font_norm);
    y += 42;

    // Enable checkbox
    auto* hChk = CreateWindowW(L"BUTTON", L"Enable Discord Webhook notifications",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        X, y, W, 26, g_win,
        (HMENU)(UINT_PTR)ID_ENABLE_CHECK, GetModuleHandleW(nullptr), nullptr);
    SetFont(hChk, g_font_norm);
    SendMessageW(hChk, BM_SETCHECK,
        config->enable_webhook ? BST_CHECKED : BST_UNCHECKED, 0);
    y += 44;

    // Buttons
    auto* btnR = CreateWindowW(L"BUTTON", L"Refresh IP",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        X, y, 120, 32, g_win,
        (HMENU)(UINT_PTR)ID_REFRESH_BTN, GetModuleHandleW(nullptr), nullptr);
    SetFont(btnR, g_font_norm);

    auto* btnS = CreateWindowW(L"BUTTON", L"Save Settings",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        WW - X - 140, y, 140, 32, g_win,
        (HMENU)(UINT_PTR)ID_SAVE_BTN, GetModuleHandleW(nullptr), nullptr);
    SetFont(btnS, g_font_norm);

    UpdateWindow(g_win);
    // Window is now visible; messages dispatched by TrayRun()'s GetMessage loop.
    (void)edWH; (void)edIN; (void)lbWH; (void)lbIN; (void)hTitle;
    (void)hIP; (void)hChk; (void)btnR; (void)btnS;
}

#endif // _WIN32
