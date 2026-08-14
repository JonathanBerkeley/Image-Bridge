#include "tray.hpp"


namespace {

constexpr UINT kTrayCallback = WM_APP + 1;
constexpr UINT kQuitCommand  = 1;
constexpr UINT kIconId       = 1;

HWND  g_window          = nullptr;
HICON g_icon            = nullptr;
UINT  g_taskbar_created = 0;

// add icon to system tray
void add_icon(const bool display_toast) {
    NOTIFYICONDATAW nid{};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = g_window;
    nid.uID              = kIconId;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = kTrayCallback;
    nid.hIcon            = g_icon;
    wcscpy_s(nid.szTip, L"Kiro Image Bridge (right-click to quit)");
    if (display_toast) {
        nid.uFlags     |= NIF_INFO;
        nid.dwInfoFlags = NIIF_INFO;   // info glyph
        wcscpy_s(nid.szInfoTitle, L"Kiro Image Bridge");
        wcscpy_s(nid.szInfo, L"Now running in the system tray.");
    }
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void remove_icon() {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = g_window;
    nid.uID    = kIconId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void show_menu() {
    POINT cursor;
    GetCursorPos(&cursor);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kQuitCommand, L"Quit Image Bridge");

    // Needed so the menu dismisses on click-away.
    SetForegroundWindow(g_window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, g_window, nullptr);
    PostMessageW(g_window, WM_NULL, 0, 0);

    DestroyMenu(menu);
}

LRESULT CALLBACK wnd_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == kTrayCallback) {
        UINT event = LOWORD(lparam);
        if (event == WM_RBUTTONUP or event == WM_LBUTTONUP or event == WM_CONTEXTMENU) {
            show_menu();
        }
        return 0;
    }
    if (msg == WM_COMMAND and LOWORD(wparam) == kQuitCommand) {
        PostQuitMessage(0);
        return 0;
    }
    if (msg == WM_DESTROY) {
        remove_icon();
        return 0;
    }
    if (g_taskbar_created != 0 and msg == g_taskbar_created) {
        add_icon(false);   // re-add after an Explorer restart
        return 0;
    }
    return DefWindowProcW(window, msg, wparam, lparam);
}

}


bool tray_create(HINSTANCE hInst) {
    g_icon = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                           GetSystemMetrics(SM_CXSMICON),
                                           GetSystemMetrics(SM_CYSMICON),
                                           LR_DEFAULTCOLOR));

    const wchar_t* class_name = L"KiroImgBridgeTray";
    WNDCLASSW window_class{};
    window_class.lpfnWndProc   = wnd_proc;
    window_class.hInstance     = hInst;
    window_class.lpszClassName = class_name;
    RegisterClassW(&window_class);

    // not shown, needed for TaskbarCreated broadcast.
    g_window = CreateWindowExW(0, class_name, L"", WS_OVERLAPPED, 0, 0, 0, 0,
                               nullptr, nullptr, hInst, nullptr);
    if (not g_window) {
        return false;
    }

    g_taskbar_created = RegisterWindowMessageW(L"TaskbarCreated");
    add_icon(true);
    return true;
}

void tray_destroy() {
    if (g_window) {
        DestroyWindow(g_window);   // sends WM_DESTROY, which removes the icon
        g_window = nullptr;
    }
    if (g_icon) {
        DestroyIcon(g_icon);
        g_icon = nullptr;
    }
}
