#include "wsl.hpp"

#include "win32.hpp"

#include <vector>


namespace wsl {

int run(const std::wstring& script_path, const std::span<const std::uint8_t> input) {
    SECURITY_ATTRIBUTES inherit{
        .nLength = sizeof(inherit),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE
    };
    HANDLE read_end = nullptr, write_end = nullptr;
    if (not CreatePipe(&read_end, &write_end, &inherit, 0)) {
        return -1;
    }
    // the child must not hold the write end
    SetHandleInformation(write_end, HANDLE_FLAG_INHERIT, 0);

    // a NUL sink for the child's stdout/stderr, required by STARTF_USESTDHANDLES.
    UniqueHandle nul{ CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  &inherit, OPEN_EXISTING, 0, nullptr) };

    std::wstring command = L"wsl.exe bash \"" + script_path + L"\"";
    std::vector command_buffer(command.begin(), command.end());
    command_buffer.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb         = sizeof(startup);
    startup.dwFlags    = STARTF_USESTDHANDLES;
    startup.hStdInput  = read_end;
    startup.hStdOutput = nul.get();
    startup.hStdError  = nul.get();

    PROCESS_INFORMATION process{};
    BOOL launched = CreateProcessW(nullptr, command_buffer.data(), nullptr, nullptr, TRUE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);

    // the child holds its own copy now
    CloseHandle(read_end);
    if (not launched) {
        CloseHandle(write_end);
        return -1;
    }
    UniqueHandle proc{ process.hProcess }, thread{ process.hThread };

    // write the PNG, then close to signal EOF.
    const std::uint8_t* cursor = input.data();
    size_t remaining = input.size();
    while (remaining > 0) {
        DWORD chunk   = remaining > (1u << 20) ? (1u << 20) : static_cast<DWORD>(remaining);
        DWORD written = 0;
        if (not WriteFile(write_end, cursor, chunk, &written, nullptr) or written == 0) {
            break;
        }
        cursor    += written;
        remaining -= written;
    }
    // EOF for the child
    CloseHandle(write_end);

    WaitForSingleObject(proc.get(), INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(proc.get(), &exit_code);
    return static_cast<int>(exit_code);
}

}
