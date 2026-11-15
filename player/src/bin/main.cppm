export module qcm.player.bin;
import :process;
import rstd;
import rstd.log;

int main() {
    static auto logger = rstd::log::EnvLogger {};
    (void)rstd::log::set_logger(logger);
    rstd::log::set_max_level(logger.filter());
    auto runtime = rstd::async::RuntimeBuilder::current_thread().enable_all().build();
    if (runtime.is_err()) {
        rstd::io::eprintln("qcm-player: runtime: {}", runtime.unwrap_err());
        return 1;
    }
    auto result = runtime->block_on(qcm::player_process::run());
    if (result.is_err()) {
        rstd::io::eprintln("qcm-player: {}", result.unwrap_err());
        return 1;
    }
    return result->to_primitive();
}
