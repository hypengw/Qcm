module;
#include <rstd/macro.hpp>
export module qcm.backend.startup_trace;
import rstd.bench;
import rstd.log;

using rstd::async::coro;
using rstd::bench::probe::ProbeCollector;
using rstd::bench::probe::ProbeId;
using rstd::bench::probe::ProbeRecorder;
using rstd::bench::probe::ProbeRegistry;
using rstd::bench::probe::ProbeSchema;
using rstd::bench::probe::ProbeSession;
using rstd::bench::probe::RecorderConfig;
using rstd::bench::probe::SpanGuard;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::startup
{
inline constexpr auto TRACE_TARGET = "qcm.startup"_str;

template<class Stage>
class Trace {
    Option<Arc<ProbeSchema>> schema_;
    Option<ProbeRecorder>    recorder_;
    SpanGuard                total_;

    template<typename T>
    auto record_async(Stage stage, coro<T> task) -> coro<T> {
        auto guard = span(stage);
        co_return co_await rstd::move(task);
    }

public:
    template<rstd::size_t N>
    explicit Trace(const ref<str> (&labels)[N]) {
        static_assert(N > 0);
        if (! rstd::log::log_enabled(rstd::log::Level::Debug, TRACE_TARGET)) return;
        auto registry = ProbeRegistry::new_();
        for (auto label : labels) (void)registry.register_probe(label).unwrap();
        auto schema            = rstd::move(registry).freeze();
        schema_                = Some(schema.clone());
        auto           session = ProbeSession::new_(rstd::move(schema));
        RecorderConfig config;
        config.sample_capacity      = rstd::cmp::max(usize(32), usize(N) * usize(2));
        config.active_span_capacity = usize(8);
        recorder_                   = Some(session.recorder(rstd::move(config)));
        total_                      = recorder_->span(ProbeId::from_u32(u32()));
    }
    ~Trace() { finish("failed"_str); }

    auto span(Stage stage) -> SpanGuard {
        if (recorder_.is_none()) return {};
        auto id = ProbeId::from_u32(u32(static_cast<unsigned>(stage)));
        rstd::log::debug(
            rstd::log::Target { TRACE_TARGET }, "startup begin {}", (*schema_)->label(id).unwrap());
        return recorder_->span(id);
    }

    template<typename F>
    decltype(auto) measure(Stage stage, F&& function) {
        auto guard = span(stage);
        return rstd::forward<F>(function)();
    }

    template<typename T>
    auto measure_async(Stage stage, coro<T> task) -> coro<T> {
        if (recorder_.is_none()) return task;
        return record_async(stage, rstd::move(task));
    }

    void finish(ref<str> outcome) {
        if (recorder_.is_none()) return;
        (void)total_.finish();
        auto batch = recorder_->drain();
        if (batch.is_err()) {
            rstd::log::debug(rstd::log::Target { TRACE_TARGET },
                             "startup profiling could not collect completed spans");
            total_    = SpanGuard {};
            recorder_ = None();
            schema_   = None();
            return;
        }
        auto collector = ProbeCollector::new_(batch->schema_owner());
        (void)collector.ingest(*batch).unwrap();
        auto report       = rstd::move(collector).finish();
        recorder_         = None();
        schema_           = None();
        auto milliseconds = [](rstd::time::Duration elapsed) {
            return f64(static_cast<double>(elapsed.as_nanos().to_primitive()) / 1'000'000.0);
        };
        for (const auto& summary : report.overall()) {
            rstd::log::debug(rstd::log::Target { TRACE_TARGET },
                             "startup outcome={} stage={} count={} total_ms={:.3} self_ms={:.3}",
                             outcome,
                             report.schema()->label(summary.probe).unwrap(),
                             summary.count,
                             milliseconds(summary.inclusive_total),
                             milliseconds(summary.exclusive_total));
        }
    }
};
} // namespace qcm::backend::startup
