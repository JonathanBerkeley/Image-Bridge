#pragma once

#include "config.hpp"
#include "win32.hpp"

#include <optional>
#include <string>


struct KeyChord { UINT mods; UINT vk; };

[[nodiscard]] std::optional<KeyChord> parse_chord(const std::string& text);

// Run the focus-gated global-hotkey daemon until its message loop ends.
int run_daemon(HINSTANCE hInst, UINT mods, UINT vk, const Config& cfg);
