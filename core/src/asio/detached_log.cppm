export module qcm.asio:detached_log;
export import qcm.core;
export import asio;



namespace qcm
{

export class asio_detached_log_t {
public:
    asio_detached_log_t(const std::source_location = std::source_location::current());
    void operator()(std::exception_ptr);

    std::source_location loc;
};

export void handle_asio_exception(std::exception_ptr                                   eptr,
                                  asio::any_completion_handler<void(std::string_view)> on_error,
                                  const std::source_location                           loc);

} // namespace qcm