#include "platform/platform.h"
#include <windows.h>

namespace plt
{
void set_thread_name(const char* name) {
    // HRESULT hr =
    //     SetThreadDescription(GetCurrentThread(), std::wstring(name).c_str());
    // if (FAILED(hr)) {
    //     // Handle error
    // }
} // namespace plt