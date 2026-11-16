module;
#include <rstd/macro.hpp>
export module qcm.backend:script;
import :lua_memory;
export import luato;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
// Owns one Lua state; provider scripts and configuration entries run here.
class ScriptEngine {
    LuaMemorySummary memory_;
    luato::State     state_;
    explicit ScriptEngine(luato::State state)
        : memory_(state, "script"_str), state_(rstd::move(state)) {}

public:
    static auto make() -> Result<ScriptEngine, luato::Error> {
        auto created =
            luato::State::create(luato::StateOptions::base(), luato::MemoryOptions::pooled());
        if (created.is_err()) return Err(rstd::move(created).unwrap_err());
        return Ok(ScriptEngine { rstd::move(created).unwrap() });
    }
    auto evaluate(ref<str> name, ref<str> source) -> Result<luato::MultiValue, luato::Error> {
        auto module = luato::LuaModuleSource { String::make(name),
                                               String::make(name),
                                               String::make(name),
                                               Vec<u8>::from(source.as_bytes()) };
        return state_.evaluate_entry(rstd::move(module));
    }
    auto execute(ref<str> name, ref<str> source) -> Result<luato::ExecutionReport, luato::Error> {
        auto module = luato::LuaModuleSource { String::make(name),
                                               String::make(name),
                                               String::make(name),
                                               Vec<u8>::from(source.as_bytes()) };
        return state_.execute_entry(rstd::move(module));
    }
    auto state() -> luato::State& { return state_; }
};
} // namespace qcm::backend
