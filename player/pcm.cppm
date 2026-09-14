export module qcm.player:pcm;
import rstd;

namespace player
{
struct PcmBlock {
    rstd::array<float, 1024 * 2> samples;
    rstd::u32                    frames {};
    rstd::f64                    seconds {};
};
struct ClockSpan {
    rstd::u64 output_frame {};
    rstd::u32 frames {};
    rstd::f64 seconds {};
};
} // namespace player
