#pragma once

#include <locale>
#include <string>
#include <array>
extern "C" {
#include <libavutil/error.h>
}

#include <limits>

import qcm.log;

namespace player
{

namespace log = qcm::log;

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

    std::string what() const {
        std::array<char, AV_ERROR_MAX_STRING_SIZE> err_buf;
        av_make_error_string(err_buf.data(), err_buf.size(), code);
        return std::format("{}", err_buf.data());
    }

    int code;
};

template<typename T>
using FFmpegResult = rstd::Result<T, FFmpegError>;

} // namespace player

template<>
struct rstd::Impl<rstd::fmt::Display, player::FFmpegError> : rstd::ImplBase<player::FFmpegError> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        auto s = this->self().what();
        return f.write_raw((const u8*)s.data(), s.size());
    }
};
