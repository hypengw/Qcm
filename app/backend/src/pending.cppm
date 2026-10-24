module;
#include <cstdint>
#include <map>
#include <utility>

export module qcm.backend.connection:pending;
import rstd;

export namespace qcm
{
template<class Message, class Error>
class BackendPending {
public:
    using Handle = rstd::async::CompletionHandle<Message, Error>;

    auto contains(std::int32_t id) const -> bool { return responses_.contains(id); }

    auto add(std::int32_t id, Handle response) -> bool {
        return responses_.emplace(id, rstd::move(response)).second;
    }

    auto complete(std::int32_t id, Message message) -> bool {
        auto response = responses_.extract(id);
        if (response.empty()) return false;
        (void)response.mapped().complete(rstd::move(message));
        return true;
    }

    void cancel(std::int32_t id) { responses_.erase(id); }

    void fail_all(const Error& error) {
        auto responses = std::exchange(responses_, {});
        for (auto& [id, response] : responses) {
            (void)id;
            (void)response.fail(error);
        }
    }

private:
    std::map<std::int32_t, Handle> responses_;
};
} // namespace qcm
