export module qcm.asio:detached_log;
export import qcm.core;
export import asio;

namespace cppstd = rstd::cppstd;

namespace qcm
{

export class asio_detached_log_t {
public:
    asio_detached_log_t(const cppstd::source_location = cppstd::source_location::current());
    void operator()(cppstd::exception_ptr);

    cppstd::source_location loc;
};

export void handle_asio_exception(cppstd::exception_ptr                                   eptr,
                                  asio::any_completion_handler<void(cppstd::string_view)> on_error,
                                  const cppstd::source_location                           loc);

} // namespace qcm