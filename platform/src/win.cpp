#ifdef _WIN32
module;
#    include <windows.h>
module platform;

namespace plt
{
void set_thread_name(const char* name) {
    HRESULT hr = SetThreadDescription(GetCurrentThread(), std::wstring(name).c_str());
    if (FAILED(hr)) {
        // Handle error
    }
}

auto is_terminal() -> bool { return _isatty(_fileno(stdout)); }
} // namespace plt

#endif