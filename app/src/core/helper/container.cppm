export module qcm.helper:container;

export template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

export template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
