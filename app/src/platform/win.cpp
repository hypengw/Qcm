module;

#ifdef _WIN32
#    include <windows.h>
#    include <io.h>
#    include <string>
#endif
module platform;

#ifdef _WIN32
std::wstring utf8_to_wstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], size_needed);
    return wstr;
}

std::string wstring_to_utf8(const std::wstring& wstr) {
    int size_needed =
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &str[0], size_needed, NULL, NULL);
    return str;
}

namespace plt
{
void set_thread_name(const char* name) {
    auto    wname = utf8_to_wstring(name);
    HRESULT hr    = SetThreadDescription(GetCurrentThread(), wname.c_str());
    if (FAILED(hr)) {
        // Handle error
    }
}

auto is_terminal() -> bool { return _isatty(_fileno(stdout)); }

auto support_color() -> bool {
    // Get the handle to the standard output
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Retrieve the current console mode
    DWORD consoleMode = 0;
    if (! GetConsoleMode(hStdout, &consoleMode)) {
        return false;
    }

    // Check if the console supports virtual terminal sequences (ANSI colors)
    return (consoleMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}
} // namespace plt

#endif