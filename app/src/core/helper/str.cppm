module;
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
export module qcm.helper:str;

export using namespace std::literals::string_view_literals;

export namespace helper
{
auto to_upper(std::string_view in) -> std::string {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return out;
}
} // namespace helper
