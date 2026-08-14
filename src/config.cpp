#include "config.hpp"

#include "util.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>


namespace {

    std::string trim(const std::string_view s) {
        constexpr std::string_view whitespace = " \t\r\n";
        size_t begin = s.find_first_not_of(whitespace);
        if (begin == std::string_view::npos) {
            return {};
        }
        size_t end = s.find_last_not_of(whitespace);
        return std::string(s.substr(begin, end - begin + 1));
    }

    std::optional<long> parse_non_negative(const std::string& s) {
        long value = 0;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
        if (ec == std::errc{} and ptr == s.data() + s.size() and value >= 0) {
            return value;
        }
        return {};
    }

}

std::wstring default_config_path() {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    return (std::filesystem::path(exe_path).parent_path() / L"image-bridge.conf").wstring();
}

// Recognised keys: serve_script, max_dim, settle_ms, restore_ms, hotkey, terminal. 
// Each line is `key = value`; `#` starts a comment.
Config load_config(const std::wstring& config_path) {
    Config cfg;
    std::ifstream file{ std::filesystem::path(config_path) };
    if (not file) {
        return cfg;   // no file: pure defaults
    }

    std::string line;
    while (std::getline(file, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) {
            line.erase(hash);
        }

        auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        std::string key = trim(std::string_view{ line }.substr(0, equals));
        std::string value = trim(std::string_view{ line }.substr(equals + 1));
        if (key.empty() or value.empty()) {
            continue;
        }

        if (key == "serve_script") {
            cfg.serve_script = widen(value);
        }
        else if (key == "hotkey") {
            cfg.hotkey = value;
        }
        else if (key == "terminal") {
            cfg.terminal_exe = widen(value);
        }
        else if (key == "max_dim") {
            if (auto parsed{ parse_non_negative(value) }; parsed and *parsed > 0) {
                cfg.max_dim = static_cast<int>(*parsed);
            }
        }
        else if (key == "settle_ms") {
            if (auto parsed = parse_non_negative(value)) {
                cfg.settle_ms = static_cast<DWORD>(*parsed);
            }
        }
        else if (key == "restore_ms") {
            if (auto parsed = parse_non_negative(value)) {
                cfg.restore_ms = static_cast<DWORD>(*parsed);
            }
        }
        // unknown keys are ignored
    }
    return cfg;
}
