#pragma once

#include <system_error>
#include <curl/curl.h>

namespace std
{
template<>
struct is_error_code_enum<CURLMcode> : true_type {};

template<>
struct is_error_code_enum<CURLcode> : true_type {};
} // namespace std

namespace request
{
class CURLMEcategory : public std::error_category {
public:
    static CURLMEcategory const& instance() {
        static CURLMEcategory instance;
        return instance;
    }

    char const* name() const noexcept override { return "CURLMEcategory"; }

    std::string message(int code) const override { return curl_multi_strerror((CURLMcode)code); }
};

const CURLMEcategory theCURLMEcategory {};

class CURLEcategory : public std::error_category {
public:
    static CURLEcategory const& instance() {
        static CURLEcategory instance;
        return instance;
    }

    char const* name() const noexcept override { return "CURLEcategory"; }

    std::string message(int code) const override { return curl_easy_strerror((CURLcode)code); }
};

const CURLEcategory theCURLEcategory {};
} // namespace request

inline std::error_code make_error_code(CURLMcode code) {
    return {
        static_cast<int>(code),
        request::theCURLMEcategory,
    };
}
inline std::error_code make_error_code(CURLcode code) {
    return {
        static_cast<int>(code),
        request::theCURLEcategory,
    };
}
