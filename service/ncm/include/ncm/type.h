#pragma once

#include <asio/awaitable.hpp>

#include "ncrequest/request.hpp"
#include "error/error.h"
#include "json_helper/helper.h"

namespace ncm
{

namespace json = qcm::json;

using Params     = std::map<std::string, std::string, std::less<>>;
using Error      = error::Error;
using BodyReader = ncrequest::req_opt::Read;

using Operation = ncrequest::Operation;
using UrlParams = ncrequest::UrlParams;

template<typename T>
using Result = nstd::expected<T, Error>;

template<typename T, typename Executor = asio::any_io_executor>
using awaitable = asio::awaitable<T, Executor>;

} // namespace ncm
