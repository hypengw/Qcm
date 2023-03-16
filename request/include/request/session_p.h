#pragma once

#include <set>

#include "core/core.h"
#include "session.h"

namespace request
{

class CurlMulti;
class Session::Private {
public:
    Private(asio::any_io_executor ex) noexcept;
    rc<CurlMulti>          multi_info;
    asio::any_io_executor  ex;
    std::set<rc<Response>> rsps;
};

} // namespace request
