export module qcm.player.bin:transport;
import rstd;

using alloc::collections::VecDeque;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::Notify;
using rstd::async::NotifyHandle;
using rstd::bytes::Bytes;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::player_process
{
class PipeWriter {
    struct Shared {
        rstd::async::pipe::Sender output;
        VecDeque<Bytes>           pending;
        Option<Bytes>             state;
        usize                     queued_bytes {};
        Notify                    changed;
        NotifyHandle              session;
        bool                      closing {}, failed {};

        Shared(rstd::async::pipe::Sender pipe, Notify wake, NotifyHandle parent)
            : output(rstd::move(pipe)), changed(rstd::move(wake)), session(rstd::move(parent)) {}

        void fail() {
            failed = true;
            (void)session.notify();
        }
        auto take() -> Option<Bytes> {
            auto next = pending.pop_front();
            if (next.is_some())
                queued_bytes -= next->len();
            else
                next = state.take();
            return next;
        }
    };

    Rc<Shared>               shared_;
    AbortOnDropHandle<empty> worker_;

    static auto write(Rc<Shared> shared) -> coro<empty> {
        for (;;) {
            if (auto next = shared.get()->take(); next.is_some()) {
                auto written = co_await rstd::async::timeout(
                    shared->output.write_all(rstd::move(next).unwrap()).into_future(),
                    Duration::from_secs(u64(5)));
                if (written.is_err() || written->is_err()) {
                    shared.get()->fail();
                    break;
                }
            } else {
                if (shared->closing) break;
                if ((co_await shared->changed.notified()).is_err()) {
                    shared.get()->fail();
                    break;
                }
            }
        }
        co_return empty {};
    }

    explicit PipeWriter(Rc<Shared> shared)
        : shared_(shared.clone()), worker_(rstd::async::spawn_local(write(rstd::move(shared)))) {}

public:
    static auto make(rstd::async::pipe::Sender output, NotifyHandle session)
        -> Result<PipeWriter, String> {
        auto changed = Notify::make();
        if (changed.is_err())
            return Err(rstd::format("stdout notification: {}", changed.unwrap_err()));
        return Ok(PipeWriter(make_rc<Shared>(
            rstd::move(output), rstd::move(changed).unwrap(), rstd::move(session))));
    }

    PipeWriter(PipeWriter&&) noexcept                    = default;
    auto operator=(PipeWriter&&) noexcept -> PipeWriter& = default;

    auto push(ref<str> frame, bool replaceable) -> bool {
        auto& shared = *shared_.get();
        if (shared.closing || shared.failed) return false;
        auto size = frame.as_bytes().len();
        if (size > usize(1024 * 1024 + 1)) return false;
        if (replaceable) {
            // Reserve one bounded slot so the final snapshot survives backpressure.
            shared.state = Some(Bytes::copy_from_slice(frame.as_bytes()));
        } else {
            auto count      = shared.pending.len() + usize(shared.state.is_some() ? 1 : 0);
            auto state_size = shared.state.is_some() ? shared.state->len() : usize();
            if (count >= usize(256) ||
                shared.queued_bytes + state_size + size > usize(2 * 1024 * 1024))
                return false;
            shared.queued_bytes += state_size + size;
            if (shared.state.is_some()) shared.pending.push_back(shared.state.take().unwrap());
            shared.pending.push_back(Bytes::copy_from_slice(frame.as_bytes()));
        }
        return shared.changed.notifier().notify().is_ok();
    }

    auto failed() const -> bool { return shared_->failed; }

    auto finish() -> coro<bool> {
        shared_.get()->closing = true;
        (void)shared_->changed.notifier().notify();
        auto drained = co_await rstd::async::timeout(rstd::future::by_ref(worker_),
                                                     Duration::from_secs(u64(2)));
        if (drained.is_err()) {
            shared_.get()->fail();
            worker_.abort();
            (void)co_await rstd::move(worker_);
        } else if (drained->is_err()) {
            shared_.get()->fail();
        }
        auto closed = co_await rstd::move(shared_.get()->output).close();
        co_return ! shared_->failed && closed.is_ok();
    }
};
} // namespace qcm::player_process
