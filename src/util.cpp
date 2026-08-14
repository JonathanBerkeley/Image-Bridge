#include "util.hpp"

#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iterator>


std::vector<std::uint8_t> read_file(const std::wstring& path) {
    std::ifstream file{ 
        std::filesystem::path(path),
        std::ios::binary
    };

    if (not file) {
        return {};
    }

    return { 
        std::istreambuf_iterator{file},
        std::istreambuf_iterator<char>{}
    };
}

bool has_image_ext(const std::wstring& path) {
    std::wstring ext = std::filesystem::path(path).extension().wstring();

    using namespace std::ranges;
    transform(ext, ext.begin(), [](const wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });

    static constexpr std::array<std::wstring_view, 6> known{
        L".png", L".jpg", L".jpeg", L".bmp", L".gif", L".webp"
    };
    return find(known, std::wstring_view{ ext }) != known.end();
}

std::wstring widen(const std::string_view str) {
    std::wstring wide;
    wide.reserve(str.size());
    for (const unsigned char c : str) {
        wide.push_back(c);
    }
    return wide;
}
