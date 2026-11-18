export module qcm.backend:error_response;
export import :error;
export import :protocol.message;
export import liserveto;
using namespace rstd::prelude;
using QcmErrorMessage = p_qcm::p_msg::M_QcmMessage;

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::ProcessError>, Box<p_qcm::p_msg::M_Rsp>> {
    static auto from(qcm::backend::ProcessError error) -> Box<p_qcm::p_msg::M_Rsp>;
};
} // namespace rstd

export namespace qcm::backend
{
auto error_response(i32 id, ProcessError error) -> QcmErrorMessage;
auto ok_response(i32 id) -> QcmErrorMessage;
auto http_error_response(const ProcessError& error) -> liserveto::Response;
} // namespace qcm::backend
