#include "app.hpp"
#include "config.hpp"
#include "hotkey.hpp"
#include "notify.hpp"
#include "util.hpp"
#include "win32.hpp"

#include <optional>
#include <string>


namespace {

// Prevent multiple daemons running at the same time
std::optional<UniqueHandle> claim_single_instance() {
    HANDLE raw = CreateMutexW(nullptr, FALSE, L"Local\\KiroImageBridge.SingleInstance");
    const bool already_running = (GetLastError() == ERROR_ALREADY_EXISTS);
    UniqueHandle mutex{ raw };

    if (already_running) {
        return {};
    }
    return mutex;
}

}


int main(int argc, char** argv) {
    HINSTANCE hInst = GetModuleHandleW(nullptr);

    bool daemon = true;
    std::wstring config_path = default_config_path();
    std::optional<std::wstring> cli_serve;
    std::optional<std::wstring> cli_terminal;
    std::optional<std::string> cli_hotkey;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if      (arg == "--once")                         daemon       = false;
        else if (arg == "--config" && i + 1 < argc)       config_path  = widen(argv[++i]);
        else if (arg.starts_with("--config="))            config_path  = widen(arg.substr(9));
        else if (arg == "--hotkey" && i + 1 < argc)       cli_hotkey   = argv[++i];
        else if (arg.starts_with("--hotkey="))            cli_hotkey   = arg.substr(9);
        else if (arg == "--serve-script" && i + 1 < argc) cli_serve    = widen(argv[++i]);
        else if (arg.starts_with("--serve-script="))      cli_serve    = widen(arg.substr(15));
        else if (arg == "--terminal" && i + 1 < argc)     cli_terminal = widen(argv[++i]);
        else if (arg.starts_with("--terminal="))          cli_terminal = widen(arg.substr(11));
    }

    Config cfg = load_config(config_path);
    if (cli_serve)    cfg.serve_script = *cli_serve;
    if (cli_terminal) cfg.terminal_exe = *cli_terminal;
    if (cli_hotkey)   cfg.hotkey       = *cli_hotkey;

    if (not daemon) return run(hInst, cfg);

    const auto instance = claim_single_instance();
    if (not instance) return 0;

    UINT mods = kDefaultMods, vk = kDefaultVk;
    if (not cfg.hotkey.empty()) {
        auto chord = parse_chord(cfg.hotkey);
        if (not chord) {
            error_alert(hInst, L"Invalid hotkey (try e.g. ctrl+u or ctrl+alt+v) [E6]");
            return 2;
        }
        mods = chord->mods;
        vk   = chord->vk;
    }
    return run_daemon(hInst, mods, vk, cfg);
}
