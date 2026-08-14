#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>


[[nodiscard]] std::vector<std::uint8_t> read_file(const std::wstring& path);
[[nodiscard]] bool                      has_image_ext(const std::wstring& path);
[[nodiscard]] std::wstring              widen(std::string_view str);  // byte-wise; inputs here are ASCII
