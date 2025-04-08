#pragma once

#include <locale>
#include <string>
#include <array>
extern "C" {
#include <libavutil/error.h>
}

#include <limits>
#include "error/error.h"

namespace player
{

struct FFmpegError : public error::ErrorBase<FFmpegError> {
    constexpr static int EABORTED { std::numeric_limits<int>::min() };
    FFmpegError(): code(0) {}
    FFmpegError(int e): code(e) {}
    FFmpegError& operator=(int e) noexcept {
        code = e;
        return *this;
    }
    bool operator==(int e) const noexcept { return e == code; }
    operator bool() const { return code < 0; }

    std::string what() {
        std::array<char, AV_ERROR_MAX_STRING_SIZE> err_buf;
        av_make_error_string(err_buf.data(), err_buf.size(), code);
        return std::format("{}", err_buf.data());
    }

    int code;
};

template<typename T>
using FFmpegResult = nstd::expected<T, FFmpegError>;

} // namespace player
