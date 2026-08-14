#include "image.hpp"

#include <cmath>

// The single translation unit that instantiates the stb single-header libraries.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

#include <webp/decode.h>


std::optional<Image> Image::decode(const std::span<const std::uint8_t> bytes) {
    if (bytes.empty()) {
        return {};
    }

    int webp_width = 0, webp_height = 0;
    if (WebPGetInfo(bytes.data(), bytes.size(), &webp_width, &webp_height)) {
        if (std::uint8_t* pixels = WebPDecodeRGBA(bytes.data(), bytes.size(), &webp_width, &webp_height)) {
            Image image{ 
                .w = webp_width,
                .h = webp_height,
                .rgba = std::vector(pixels, pixels + static_cast<size_t>(webp_width) * webp_height * 4)
            };
            WebPFree(pixels);
            return image;
        }
    }

    int width = 0, height = 0, channels = 0;
    std::uint8_t* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
                                                 &width, &height, &channels, 4);
    if (not pixels) {
        return {};
    }

    Image image{
        .w = width,
        .h = height,
        .rgba = std::vector(pixels, pixels + static_cast<size_t>(width) * height * 4)
    };
    stbi_image_free(pixels);

    return image;
}

// CF_DIB has no BITMAPFILEHEADER; prepend one and let decode() read it as a BMP.
std::optional<Image> Image::from_dib(const std::span<const std::uint8_t> dib) {
    if (dib.size() < sizeof(BITMAPINFOHEADER)) {
        return {};
    }
    const auto* header = reinterpret_cast<const BITMAPINFOHEADER*>(dib.data());

    constexpr DWORD file_header_size = 14;
    DWORD info_size       = header->biSize;
    DWORD bit_count       = header->biBitCount;
    DWORD colors_used     = header->biClrUsed;
    DWORD palette_entries = (bit_count <= 8) ? (colors_used ? colors_used : (1u << bit_count)) : colors_used;
    DWORD bitfield_masks  = (header->biCompression == BI_BITFIELDS) ? 3u : 0u;
    DWORD pixel_offset    = file_header_size + info_size + palette_entries * 4 + bitfield_masks * 4;

    std::vector<std::uint8_t> bmp;
    bmp.reserve(file_header_size + dib.size());
    auto put16 = [&](WORD value) {
        bmp.push_back(value & 0xff);
        bmp.push_back((value >> 8) & 0xff);
    };
    auto put32 = [&](DWORD value) {
        bmp.push_back(value & 0xff);
        bmp.push_back((value >> 8) & 0xff);
        bmp.push_back((value >> 16) & 0xff);
        bmp.push_back((value >> 24) & 0xff);
    };
    bmp.push_back('B'); bmp.push_back('M');                     // signature
    put32(file_header_size + static_cast<DWORD>(dib.size()));   // total file size
    put16(0); put16(0);                                         // reserved
    put32(pixel_offset);                                        // offset to pixel data
    bmp.insert(bmp.end(), dib.begin(), dib.end());

    return decode(bmp);
}

void Image::cap_to(const int max_dim) {
    if (longest_side() <= max_dim) {
        return;
    }
    const double scale = static_cast<double>(max_dim) / longest_side();
    const int    new_w = std::max(1, static_cast<int>(std::lround(w * scale)));
    const int    new_h = std::max(1, static_cast<int>(std::lround(h * scale)));
    std::vector<std::uint8_t> resized(static_cast<size_t>(new_w) * new_h * 4);
    if (stbir_resize_uint8_srgb(rgba.data(), w, h, 0, resized.data(), new_w, new_h, 0, STBIR_RGBA)) {
        w    = new_w;
        h    = new_h;
        rgba = std::move(resized);
    }
}

std::optional<std::vector<std::uint8_t>> Image::encode_png() const {
    int length = 0;
    unsigned char* png = stbi_write_png_to_mem(rgba.data(), w * 4, w, h, 4, &length);
    if (not png or length <= 0) {
        return {};
    }
    std::vector bytes(png, png + length);
    STBIW_FREE(png);
    return bytes;
}

UniqueHGlobal Image::to_dib() const {
    const size_t pixel_count = static_cast<size_t>(w) * h;
    const size_t image_bytes = pixel_count * 4;
    UniqueHGlobal memory{ GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + image_bytes) };
    if (not memory) {
        return {};
    }
    GlobalMemLock<BITMAPINFOHEADER> lock{ memory.get() };
    if (not lock) {
        return {};
    }

    auto* header = lock.get();
    ZeroMemory(header, sizeof(BITMAPINFOHEADER));
    header->biSize        = sizeof(BITMAPINFOHEADER);
    header->biWidth       = w;
    header->biHeight      = -h;          // negative height means top-down rows
    header->biPlanes      = 1;
    header->biBitCount    = 32;
    header->biCompression = BI_RGB;
    header->biSizeImage   = static_cast<DWORD>(image_bytes);

    // CF_DIB stores pixels as BGRA; our buffer is RGBA, so swap red and blue.
    auto* dib_pixels = reinterpret_cast<std::uint8_t*>(header + 1);   // pixel data follows the header
    for (size_t i = 0; i < pixel_count; ++i) {
        const std::uint8_t* source      = &rgba[i * 4];          // RGBA
        std::uint8_t*       destination = &dib_pixels[i * 4];    // BGRA
        destination[0] = source[2];  // B
        destination[1] = source[1];  // G
        destination[2] = source[0];  // R
        destination[3] = source[3];  // A
    }
    return memory;
}
