#pragma once

#include "config.hpp"
#include "image.hpp"

#include <optional>
#include <string>
#include <vector>


struct Acquired {
    Image                     image;
    bool                      was_file_drop = false;
    std::vector<std::wstring> files{};
};

[[nodiscard]] std::optional<Acquired> acquire_image();
int                                   run(HINSTANCE hInst, const Config& cfg);
