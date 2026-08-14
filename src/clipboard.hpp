#pragma once

#include "win32.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>


// A snapshot of the clipboard formats we care about, in priority order.
struct ClipContents {
    std::vector<std::uint8_t> png;    // encoded PNG (keeps alpha) if an app offered a "PNG" format
    std::vector<std::uint8_t> dib;    // raw bitmap; the universal fallback for clipboard images
    std::vector<std::wstring> files;  // a file drop, if present

    [[nodiscard]] bool has_png()   const { return not png.empty(); }
    [[nodiscard]] bool has_image() const { return not dib.empty(); }
    [[nodiscard]] std::optional<std::wstring> first_image_file() const;

    [[nodiscard]] static ClipContents read();
};

[[nodiscard]] UniqueHGlobal make_drop_handle(std::span<const std::wstring> files);
void                        set_clipboard(UINT fmt, UniqueHGlobal mem);
