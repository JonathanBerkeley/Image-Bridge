#include "notify.hpp"


void send_ctrl_v() {
    INPUT inputs[4] = {};
    auto key = [&](const int index, WORD vk, const bool key_up) {
        inputs[index].type       = INPUT_KEYBOARD;
        inputs[index].ki.wVk     = vk;
        inputs[index].ki.dwFlags = key_up ? KEYEVENTF_KEYUP : 0;
    };
    key(0, VK_CONTROL, false);
    key(1, 'V',        false);
    key(2, 'V',        true);
    key(3, VK_CONTROL, true);
    SendInput(4, inputs, sizeof(INPUT));
}

void error_alert(HINSTANCE hInst, const std::wstring& text) {
    const wchar_t* class_name = L"KiroImgBridgeNotify";
    WNDCLASSW window_class{};
    window_class.lpfnWndProc   = DefWindowProcW;
    window_class.hInstance     = hInst;
    window_class.lpszClassName = class_name;
    RegisterClassW(&window_class);

    HWND window = CreateWindowExW(0, class_name, L"", WS_OVERLAPPED, 0, 0, 0, 0,
                                  nullptr, nullptr, hInst, nullptr);

    NOTIFYICONDATAW icon_data{};
    icon_data.cbSize      = sizeof(icon_data);
    icon_data.hWnd        = window;
    icon_data.uID         = 1;
    icon_data.uFlags      = NIF_ICON | NIF_INFO | NIF_TIP;
    icon_data.hIcon       = LoadIconW(nullptr, IDI_ERROR);
    icon_data.dwInfoFlags = NIIF_ERROR;
    wcscpy_s(icon_data.szTip, L"Image Bridge");
    wcscpy_s(icon_data.szInfoTitle, L"Kiro Image Bridge");
    wcsncpy_s(icon_data.szInfo, text.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_ADD, &icon_data);
    Sleep(3000);
    Shell_NotifyIconW(NIM_DELETE, &icon_data);

    DestroyWindow(window);
    UnregisterClassW(class_name, hInst);
}
