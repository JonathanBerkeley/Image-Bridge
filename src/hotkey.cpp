#include "hotkey.hpp"

#include "app.hpp"
#include "config.hpp"
#include "notify.hpp"
#include "tray.hpp"

#include <filesystem>


std::optional<KeyChord> parse_chord(const std::string& text) {
    auto lower = [](const char c) {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
    };
    auto upper = [](const char c) {
        return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c;
    };

    UINT mods = 0;
    int key_vk = 0;
    int key_count = 0;
    std::string run;

    auto flush = [&]() -> bool {
        std::string token;
        for (const auto c : run) {
            if (c != ' ' && c != '\t') {
                token += lower(c);
            }
        }
        run.clear();

        // tolerate a stray '+'
        if (token.empty()) {
            return true;
        }

        if (token == "ctrl" or token == "control") { mods |= MOD_CONTROL; return true; }
        if (token == "alt") { mods |= MOD_ALT; return true; }
        if (token == "shift") { mods |= MOD_SHIFT; return true; }
        if (token == "win" or token == "super" or token == "meta" or token == "cmd") {
            mods |= MOD_WIN;
            return true;
        }

        // a non-modifier token must be a single key
        if (token.size() != 1) {
            return false;
        }

        const char key = upper(token[0]);
        const bool letter = key >= 'A' && key <= 'Z';
        const bool digit = key >= '0' && key <= '9';

        if (not letter and not digit) {
            return false;
        }

        key_vk = static_cast<unsigned char>(key);
        ++key_count;
        return true;
    };

    for (const auto c : text) {
        if (c == '+') {
            if (not flush()) {
                return {};
            }
        }
        else {
            run += c;
        }
    }
    if (not flush()) {
        return {};
    }
    // exactly one non-modifier key
    if (key_count != 1) {
        return {};
    }

    return KeyChord{
        .mods = mods,
        .vk = static_cast<UINT>(key_vk)
    };
}

namespace {

// Daemon state, reached from the WinEvent callback (which carries no user data).
HINSTANCE    g_instance       = nullptr;
UINT         g_mods           = kDefaultMods;
UINT         g_vk             = kDefaultVk;
std::wstring g_terminal_exe;   // set from cfg in run_daemon; empty init avoids a throwing static ctor
bool         g_registered     = false;
bool         g_warned_reg_err = false;
bool         g_busy           = false;

// True when the window belongs to a process whose exe name is the target terminal.
bool foreground_is_target_terminal(HWND window) {
    if (not window) {
        return false;
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (not pid) {
        return false;
    }
    UniqueHandle process{ OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid) };
    if (not process) {
        return false;   // e.g. an elevated terminal we cannot open
    }
    wchar_t path[MAX_PATH];
    DWORD   length = MAX_PATH;
    if (not QueryFullProcessImageNameW(process.get(), 0, path, &length)) {
        return false;
    }
    std::wstring exe_name = std::filesystem::path(path).filename().wstring();
    return lstrcmpiW(exe_name.c_str(), g_terminal_exe.c_str()) == 0;
}

// Arm the hotkey while the target terminal is focused; release it otherwise.
void update_hotkey_for_foreground(HWND window) {
    const bool focused = foreground_is_target_terminal(window);
    if (focused and not g_registered) {
        if (RegisterHotKey(nullptr, kHotkeyId, g_mods | MOD_NOREPEAT, g_vk)) {
            g_registered = true;
        } else if (not g_warned_reg_err) {
            g_warned_reg_err = true;
            error_alert(g_instance, L"Could not register hotkey (already in use?) [E4]");
        }
    } else if (not focused and g_registered) {
        UnregisterHotKey(nullptr, kHotkeyId);
        g_registered = false;
    }
}

void CALLBACK win_event_proc(HWINEVENTHOOK, DWORD event, HWND window,
                             LONG objectId, LONG childId, DWORD, DWORD) {
    if (event != EVENT_SYSTEM_FOREGROUND) {
        return;
    }
    if (objectId != OBJID_WINDOW or childId != CHILDID_SELF) {
        return;
    }
    update_hotkey_for_foreground(window);
}

}

int run_daemon(HINSTANCE hInst, UINT mods, UINT vk, const Config& cfg) {
    g_instance     = hInst;
    g_mods         = mods;
    g_vk           = vk;
    g_terminal_exe = cfg.terminal_exe;

    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
        win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (not hook) {
        error_alert(hInst, L"Could not install focus hook [E5]");
        return 1;
    }

    update_hotkey_for_foreground(GetForegroundWindow());   // arm now if the terminal is already focused

    tray_create(hInst);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_HOTKEY and msg.wParam == static_cast<WPARAM>(kHotkeyId)) {
            if (not g_busy) {
                g_busy = true;
                run(hInst, cfg);
                g_busy = false;
                // Drop any presses that queued while the paste ran, so a held
                // chord does not fire a second time.
                MSG queued;
                while (PeekMessageW(&queued, nullptr, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {}
            }
        } else {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (g_registered) {
        UnregisterHotKey(nullptr, kHotkeyId);
    }
    UnhookWinEvent(hook);
    tray_destroy();
    return 0;
}
