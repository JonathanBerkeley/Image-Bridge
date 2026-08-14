#pragma once

#include "win32.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>


struct Image {
    int                       w = 0;
    int                       h = 0;
    std::vector<std::uint8_t> rgba;   // tightly packed RGBA8

    [[nodiscard]] bool empty()        const { return rgba.empty() or w <= 0 or h <= 0; }
    [[nodiscard]] int  longest_side() const { return std::max(w, h); }

    void                                                   cap_to(int max_dim);
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> encode_png() const;  // PNG bytes in memory
    [[nodiscard]] UniqueHGlobal                            to_dib() const;

    [[nodiscard]] static std::optional<Image> decode(std::span<const std::uint8_t> bytes);
    [[nodiscard]] static std::optional<Image> from_dib(std::span<const std::uint8_t> dib);
};
