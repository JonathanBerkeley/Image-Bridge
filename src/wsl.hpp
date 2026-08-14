#pragma once

#include <cstdint>
#include <span>
#include <string>


namespace wsl {

// Pipe `input` to `wsl.exe bash "<script>"` on stdin and return its exit code.
// The script serves those bytes on the X11 clipboard.
[[nodiscard]] int run(const std::wstring& script_path, std::span<const std::uint8_t> input);

//TODO: Remove xclip dependency requirement with a separate static binary 
// for Linux (WSL) which handles bridging and clipboard serve better (tsz:L)

}
