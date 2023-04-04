#include <bitset>
#include <array>

template<typename EnumT>
    requires std::is_enum_v<EnumT>
class BitFlags {
    using UnderlyingT = typename std::make_unsigned_t<typename std::underlying_type_t<EnumT>>;

public:
    constexpr BitFlags() noexcept: bits_(0u) {}
    constexpr BitFlags(UnderlyingT val) noexcept: bits_(val) {}
    BitFlags& operator=(UnderlyingT val) noexcept { bits_ = val; }

    template<typename... Ts>
        requires(std::same_as<Ts, EnumT> && ...)
    constexpr BitFlags(Ts... es) noexcept {
        std::array arr_es { es... };
        for (auto e : arr_es) {
            set(e);
        }
    }

    BitFlags& set(EnumT e, bool value = true) noexcept {
        bits_.set(underlying(e), value);
        return *this;
    }

    BitFlags& reset(EnumT e) noexcept {
        set(e, false);
        return *this;
    }

    BitFlags& reset() noexcept {
        bits_.reset();
        return *this;
    }

    [[nodiscard]] bool all() const noexcept { return bits_.all(); }

    [[nodiscard]] bool any() const noexcept { return bits_.any(); }

    [[nodiscard]] bool none() const noexcept { return bits_.none(); }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return bits_.size(); }

    [[nodiscard]] std::size_t count() const noexcept { return bits_.count(); }

    constexpr bool operator[](EnumT e) const { return bits_[underlying(e)]; }

    constexpr bool operator[](UnderlyingT t) const { return bits_[t]; }

    auto to_string() const { return bits_.to_string(); }

private:
    static constexpr UnderlyingT underlying(EnumT e) { return static_cast<UnderlyingT>(e); }

private:
    std::bitset<sizeof(UnderlyingT) * 8> bits_;
};
