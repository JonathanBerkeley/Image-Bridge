// Built-in defaults, the runtime Config that layers a file and CLI over them,
// and the loader. Precedence: defaults < config file < command-line arguments.
#pragma once

#include "win32.hpp"

#include <string>


inline constexpr auto kServeScript     = L"/usr/local/bin/image-bridge-serve";
inline constexpr auto kDefaultTerminal = L"WindowsTerminal.exe";

inline constexpr int   kMaxDim    = 2000;   // images over 2000px per side cause issues in many-image chats in kiro
inline constexpr DWORD kSettleMs  = 200;    // let SetClipboardData(image) settle before the keystroke
inline constexpr DWORD kRestoreMs = 600;    // wait for Kiro to read X11 before restoring the file drop (hacky)

inline constexpr int  kHotkeyId    = 1;
inline constexpr UINT kDefaultMods = MOD_CONTROL;
inline constexpr UINT kDefaultVk   = 'U';

struct Config {
    std::wstring serve_script = kServeScript;
    int          max_dim      = kMaxDim;
    DWORD        settle_ms    = kSettleMs;
    DWORD        restore_ms   = kRestoreMs;
    std::string  hotkey;
    std::wstring terminal_exe = kDefaultTerminal;
};

[[nodiscard]] std::wstring default_config_path();   // <exe directory>\image-bridge.conf

// Apply key=value overrides from the file at config_path
// Missing file/key = use default
[[nodiscard]] Config load_config(const std::wstring& config_path);
