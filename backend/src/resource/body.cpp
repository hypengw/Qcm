module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.body;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::bytes::BytesMut;
using rstd::sync::Arc;
using rstd::sync::Mutex;
namespace watch = rstd::async::watch;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::resource_cache
{
struct ResourceBodyControl {
    struct Status {
        Option<Box<dyn<ResourceBodySource>>> source;
        bool                                 reading {};
        bool                                 closing {};
        bool                                 closed {};
        bool                                 cancelled {};
        bool                                 ended {};
    };
    AsyncMutex<empty>   operation { empty {} };
    Mutex<Status>       status;
    watch::Sender<bool> cancelled { false };

    explicit ResourceBodyControl(Box<dyn<ResourceBodySource>> source)
        : status(Status { .source = Some(rstd::move(source)) }) {}
    ~ResourceBodyControl() { cancel(); }

    void cancel() {
        Option<Box<dyn<ResourceBodySource>>> source;
        {
            auto state = status.lock().unwrap();
            if (state->cancelled || state->closed) return;
            state->cancelled = true;
            source           = state->source.take();
        }
        (void)cancelled.send_replace(true);
        if (source.is_some()) (*source)->cancel();
    }
    void begin_close() {
        bool reading;
        {
            auto state     = status.lock().unwrap();
            state->closing = true;
            reading        = state->reading;
        }
        if (reading) cancel();
    }
};

struct ResourceBodyOperation {
    Arc<ResourceBodyControl>     owner;
    Box<dyn<ResourceBodySource>> source;
    bool                         completed {};

    ~ResourceBodyOperation() {
        bool retained;
        bool closed;
        {
            auto state     = owner->status.lock().unwrap();
            state->reading = false;
            closed         = state->closed;
            // EOF and errors still retain the source for its asynchronous close.
            retained = completed && ! state->cancelled && ! closed;
            if (retained)
                state->source = Some(rstd::move(source));
            else
                state->ended = true;
        }
        // The borrowing next/close future is destroyed before this operation releases its source.
        if (! retained && ! closed) source->cancel();
    }
};

auto read_resource_body(Option<Arc<ResourceBodyControl>> owner)
    -> coro<Result<Option<Bytes>, Error>> {
    if (owner.is_none()) co_return Ok(None());
    auto                                 operation = co_await (*owner)->operation.lock();
    auto                                 cancelled = (*owner)->cancelled.subscribe();
    Option<Box<dyn<ResourceBodySource>>> source;
    {
        auto state = (*owner)->status.lock().unwrap();
        if (state->closed || state->ended) co_return Ok(None());
        if (state->cancelled) co_return Err(Error::invalidated());
        if (state->closing) co_return Ok(None());
        source         = state->source.take();
        state->reading = true;
    }
    ResourceBodyOperation reading { owner->clone(), rstd::move(source).unwrap() };
    auto                  selected =
        co_await rstd::async::select(reading.source->next().into_future(), cancelled.changed());
    if (selected.is_right()) {
        rstd_co_try(rstd::move(selected).unwrap_right(), [](auto error) {
            return Error::state("resource body cancellation channel closed"_str, rstd::move(error));
        });
        co_return Err(Error::invalidated());
    }
    auto result = rstd::move(selected).unwrap_left();
    {
        auto state        = (*owner)->status.lock().unwrap();
        reading.completed = true;
        state->reading    = false;
        if (result.is_err() || result->is_none()) state->ended = true;
        if (state->cancelled && result.is_ok()) co_return Err(Error::invalidated());
    }
    co_return rstd::move(result);
}

auto close_resource_body(Option<Arc<ResourceBodyControl>> owner) -> coro<empty> {
    if (owner.is_none()) co_return empty {};
    auto                                 operation = co_await (*owner)->operation.lock();
    Option<Box<dyn<ResourceBodySource>>> source;
    {
        auto state = (*owner)->status.lock().unwrap();
        if (state->closed) co_return empty {};
        source = state->source.take();
        if (source.is_none()) {
            state->closed = true;
            co_return empty {};
        }
    }
    ResourceBodyOperation closing { owner->clone(), rstd::move(source).unwrap() };
    co_await closing.source->close();
    {
        auto state    = (*owner)->status.lock().unwrap();
        state->closed = true;
    }
    co_return empty {};
}

auto collect_resource_body(Option<Arc<ResourceBodyControl>> owner) -> coro<Result<Bytes, Error>> {
    BytesMut bytes;
    if (owner.is_none()) co_return Ok(bytes.freeze());
    for (;;) {
        auto result = co_await read_resource_body(Some(owner->clone()));
        if (result.is_err()) {
            (*owner)->begin_close();
            co_await close_resource_body(Some(owner->clone()));
            co_return Err(rstd::move(result).unwrap_err());
        }
        if (result->is_none()) break;
        auto reserved = bytes.try_reserve((**result).len());
        if (reserved.is_err()) {
            (*owner)->cancel();
            (*owner)->begin_close();
            co_await close_resource_body(Some(owner->clone()));
            co_return Err(Error::state("resource body buffer allocation failed"_str));
        }
        bytes.extend_from_slice((**result).as_slice());
    }
    bool cancelled;
    {
        auto state = (*owner)->status.lock().unwrap();
        cancelled  = state->cancelled;
    }
    (*owner)->begin_close();
    co_await close_resource_body(Some(owner->clone()));
    if (cancelled) co_return Err(Error::invalidated());
    co_return Ok(bytes.freeze());
}

struct ResourceBytesBody {
    Mutex<Option<Bytes>> bytes;
    explicit ResourceBytesBody(Bytes bytes): bytes(Some(rstd::move(bytes))) {}
    auto next() -> coro<Result<Option<Bytes>, Error>> {
        auto state = bytes.lock().unwrap();
        if (state->is_none() || (*state)->is_empty()) {
            *state = None();
            co_return Ok(None());
        }
        co_return Ok(state->take());
    }
    void cancel() {
        auto state = bytes.lock().unwrap();
        *state     = None();
    }
    auto close() -> coro<empty> {
        cancel();
        co_return empty {};
    }
};

auto ResourceBody::owner() const -> Option<Arc<ResourceBodyControl>> {
    return source_.is_some() ? Some(source_->clone()) : None();
}

ResourceBody::ResourceBody() noexcept = default;
ResourceBody::ResourceBody(Box<dyn<ResourceBodySource>> source)
    : source_(Some(Arc<ResourceBodyControl>::make(rstd::move(source)))) {}
ResourceBody::ResourceBody(ResourceBody&& other) noexcept: source_(other.source_.take()) {}
auto ResourceBody::operator=(ResourceBody&& other) noexcept -> ResourceBody& {
    if (this != &other) {
        cancel();
        source_ = other.source_.take();
    }
    return *this;
}
ResourceBody::~ResourceBody() { cancel(); }

auto ResourceBody::next() -> coro<Result<Option<Bytes>, Error>> {
    return read_resource_body(owner());
}
auto ResourceBody::collect() -> coro<Result<Bytes, Error>> {
    return collect_resource_body(owner());
}
auto ResourceBody::close() -> coro<empty> {
    auto source = source_.take();
    if (source.is_some()) (*source)->begin_close();
    return close_resource_body(rstd::move(source));
}
void ResourceBody::cancel() {
    if (auto source = owner()) (*source)->cancel();
}

auto ResourceBody::from_bytes(Bytes bytes) -> ResourceBody {
    return resource_body(Arc<ResourceBytesBody>::make(rstd::move(bytes)));
}
} // namespace qcm::backend::resource_cache
