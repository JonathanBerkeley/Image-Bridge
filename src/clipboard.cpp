#include "clipboard.hpp"

#include "util.hpp"


std::optional<std::wstring> ClipContents::first_image_file() const {
    for (const auto& file : files) {
        if (has_image_ext(file)) {
            return file;
        }
    }
    return {};
}

ClipContents ClipContents::read() {
    ClipContents contents;
    ClipboardSession session;
    if (not session) {
        return contents;
    }

    // Prefer an app-provided encoded PNG: it preserves alpha, where CF_DIB
    // flattens transparent pixels (often to black). The format id is cached.
    static const UINT png_format = RegisterClipboardFormatW(L"PNG");
    if (png_format and session.available(png_format)) {
        if (HANDLE handle = session.get(png_format)) {
            GlobalMemLock<std::uint8_t> lock{ handle };
            if (lock) {
                if (SIZE_T size = GlobalSize(handle)) {
                    contents.png.assign(lock.get(), lock.get() + size);
                }
            }
        }
    }
    if (contents.png.empty() and session.available(CF_DIB)) {
        if (HANDLE handle = session.get(CF_DIB)) {
            GlobalMemLock<std::uint8_t> lock{ handle };
            if (lock) {
                if (SIZE_T size = GlobalSize(handle)) {
                    contents.dib.assign(lock.get(), lock.get() + size);
                }
            }
        }
    }
    if (contents.png.empty() and contents.dib.empty() and session.available(CF_HDROP)) {
        if (HANDLE handle = session.get(CF_HDROP)) {
            auto drop = static_cast<HDROP>(handle);
            UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count; ++i) {
                wchar_t path[MAX_PATH];
                if (DragQueryFileW(drop, i, path, MAX_PATH)) {
                    contents.files.emplace_back(path);
                }
            }
        }
    }
    return contents;
}

UniqueHGlobal make_drop_handle(const std::span<const std::wstring> files) {
    size_t char_count = 1;   // the trailing extra NUL that terminates the double-NUL list
    for (const auto& file : files) {
        char_count += file.size() + 1;
    }
    size_t byte_count = sizeof(DROPFILES) + char_count * sizeof(wchar_t);

    UniqueHGlobal memory{ GlobalAlloc(GMEM_MOVEABLE, byte_count) };
    if (not memory) {
        return {};
    }
    GlobalMemLock<DROPFILES> lock{ memory.get() };
    if (not lock) {
        return {};
    }

    auto* drop = lock.get();
    ZeroMemory(drop, byte_count);
    drop->pFiles = sizeof(DROPFILES);   // the file list begins right after the struct
    drop->fWide = TRUE;
    auto* dst = reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(drop) + sizeof(DROPFILES));
    for (const auto& file : files) {
        std::memcpy(dst, file.c_str(), (file.size() + 1) * sizeof(wchar_t));
        dst += file.size() + 1;
    }
    return memory;
}

void set_clipboard(UINT fmt, UniqueHGlobal mem) {
    if (not mem) {
        return;
    }

    ClipboardSession session;
    if (not session) {
        return;
    }

    session.clear();
    if (session.set(fmt, mem.get())) {
        static_cast<void>(mem.release());   // the OS owns the handle now
    }
}
