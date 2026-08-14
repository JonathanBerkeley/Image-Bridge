// lean windows.h include, and RAII wrappers for WinAPI
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <memory>
#include <type_traits>
#include <utility>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")


// Opens the clipboard with retry and closes it on scope exit.
class ClipboardSession {
    bool open_ = false;
public:
    ClipboardSession() {
        for (int attempt = 0; attempt < 20 && !open_; ++attempt) {
            open_ = OpenClipboard(nullptr);
            if (!open_) Sleep(10);
        }
    }
    ~ClipboardSession() { if (open_) CloseClipboard(); }
    explicit operator bool() const { return open_; }

    [[nodiscard]] bool   available(UINT fmt) const { return IsClipboardFormatAvailable(fmt); }
    [[nodiscard]] HANDLE get(UINT fmt)       const { return GetClipboardData(fmt); }
    void                 clear()                   { EmptyClipboard(); }
    bool                 set(UINT fmt, HANDLE handle) { return SetClipboardData(fmt, handle); }

    ClipboardSession(const ClipboardSession&)            = delete;
    ClipboardSession& operator=(const ClipboardSession&) = delete;
    ClipboardSession(ClipboardSession&&)                 = delete;
    ClipboardSession& operator=(ClipboardSession&&)      = delete;
};

// GlobalLock/GlobalUnlock pair with a typed view of the locked memory.
template <class T = void>
class GlobalMemLock {
    HGLOBAL handle_;
    T*      ptr_;
public:
    explicit GlobalMemLock(HGLOBAL handle) : handle_(handle), ptr_(static_cast<T*>(::GlobalLock(handle))) {}
    ~GlobalMemLock() { if (ptr_) ::GlobalUnlock(handle_); }
    [[nodiscard]] T* get() const &  { return ptr_; }
    T*               get() const && = delete;   // a temporary's pointer would dangle at once
    explicit operator bool() const { return ptr_ != nullptr; }
    GlobalMemLock(const GlobalMemLock&)            = delete;
    GlobalMemLock& operator=(const GlobalMemLock&) = delete;
    GlobalMemLock(GlobalMemLock&&)                 = delete;
    GlobalMemLock& operator=(GlobalMemLock&&)      = delete;
};

// Owns an HGLOBAL until released (e.g. handed to the OS via SetClipboardData).
struct GlobalFreeDeleter { void operator()(HGLOBAL h) const { if (h) GlobalFree(h); } };
using UniqueHGlobal = std::unique_ptr<std::remove_pointer_t<HGLOBAL>, GlobalFreeDeleter>;

// Owns a HANDLE and closes it on scope exit.
class UniqueHandle {
    HANDLE handle_ = nullptr;
public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE handle) : handle_(handle) {}
    ~UniqueHandle() { reset(); }
    UniqueHandle(UniqueHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) { reset(); handle_ = std::exchange(other.handle_, nullptr); }
        return *this;
    }
    [[nodiscard]] HANDLE get() const &  { return handle_; }
    HANDLE               get() const && = delete;   // a temporary's handle would be closed at once
    void reset() { if (handle_ && handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); handle_ = nullptr; }
    explicit operator bool() const { return handle_ && handle_ != INVALID_HANDLE_VALUE; }
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
};
