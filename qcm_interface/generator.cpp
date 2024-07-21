#include <string>
#include <vector>
#include <string_view>
#include <span>
#include <variant>
#include <ranges>

#include <fstream>
#include <filesystem>

#include <QtCore>

#include "peg_cpp.hpp"

#include "core/core.h"
#include "core/fmt.h"
#include "core/log.h"
#include "core/type.h"
#include "core/variant_helper.h"

using namespace std::literals::string_view_literals;
constexpr auto DECLARE_MODEL { "DECLARE_MODEL"sv };
constexpr auto DECLARE_PROPERTY { "DECLARE_PROPERTY"sv };
constexpr auto MT_COPY { "MT_COPY"sv };

namespace pegtl = tao::pegtl;

namespace details
{
constexpr auto dels { " :;,(){}<>[]|+=*/\\\n"sv };

inline auto is_del(char c) -> bool { return dels.find(c) != std::string_view::npos; };

inline auto replace_identity(std::string_view src, std::string_view match, std::string_view replace)
    -> std::string {
    std::string out;
    out.reserve(src.size());
    auto is_del = [src](std::string_view::iterator it) -> bool {
        return it < src.begin() || it >= src.end() || details::is_del(*it);
    };
    auto end  = src.end() - match.size();
    auto prev = src.begin() - 1;
    auto it   = src.begin();
    while (it < end) {
        if (is_del(prev)) {
            auto next = it + match.size();
            if (match == std::string_view(it, next) && is_del(next)) {
                out.append(replace);
                it   = next;
                prev = it - 1;
                continue;
            }
        }
        out.push_back(*it);
        it++;
        prev++;
    }
    if (it < src.end()) out.append(std::string_view { it, src.end() });
    return out;
}

} // namespace details

struct ClassInfo {
    std::string name;
    std::string parent;

    bool is_struct;
};

struct MacroInfo {
    std::vector<std::string> ns;
    std::vector<ClassInfo>   class_;
    std::string              name;
    std::vector<std::string> params;
};

struct InputInfo {
    std::vector<MacroInfo> macros;
};

struct OutputInfo {
    struct VarInfo {
        std::string type;
        std::string name;
        std::string sig;
    };
    struct ModelInfo {
        std::vector<std::string> ns;
        std::string              fullname;
        std::string              name;
        std::string              parent;
        std::vector<VarInfo>     vars;

        bool copyable { false };
        bool is_struct { false };
    };

    std::vector<ModelInfo> models;
};

namespace cpp_rule
{

namespace res
{
struct declare_class {
    bool        is_struct { false };
    std::string name {};
    std::string parent {};
};
struct declare_namespace {
    std::string name;
};
using type = std::variant<std::monostate, res::declare_class, res::declare_namespace>;
} // namespace res

struct state {
    res::type            result;
    std::optional<usize> end;
};

template<typename Rule>
struct action {
    template<typename ActionInput>
    static void apply(const ActionInput& in, state& s) {
        if constexpr (std::same_as<Rule, grammer_cpp::dec_namespace_name>) {
            std::get<res::declare_namespace>(s.result).name = in.string_view();
        } else if constexpr (std::same_as<Rule, grammer_cpp::alias::dec_class_name>) {
            std::get<res::declare_class>(s.result).name = in.string_view();
        } else if constexpr (std::is_base_of_v<grammer_cpp::end::end, Rule>) {
            s.end = in.position().byte; // + 1;
        } else if constexpr (std::same_as<Rule, grammer_cpp::alias::dec_class_inherit_name>) {
            std::get<res::declare_class>(s.result).parent = in.string_view();
        }
        // fmt::print("try: {}\n", ycore::type_name<Rule>());
    }
};

struct match_end {
    template<typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M,
             template<typename...> class Action, template<typename...> class Control,
             typename ParseInput, typename... States>
    static bool match(ParseInput& in, state& s) {
        if constexpr (std::same_as<Rule, grammer_cpp::dec_namespace>) {
            s.result = res::declare_namespace {};
        } else if constexpr (std::same_as<Rule, grammer_cpp::dec_class>) {
            s.result = res::declare_class {
                .is_struct = ! std::string_view { in.begin(), in.end() }.starts_with("class"),
            };
        }
        bool ok = tao::pegtl::match<Rule, A, M, Action, Control>(in, s);
        return ok || s.end;
    }
};

template<>
struct action<grammer_cpp::dec_namespace> : match_end {};

template<>
struct action<grammer_cpp::dec_class> : match_end {};
} // namespace cpp_rule

auto parse(std::string_view src, std::span<const std::string_view> macro_names) -> InputInfo {
    enum class StatusType
    {
        Start,
        MacroCall
    };

    enum class BracketType
    {
        Round,
        Square,
        Curly
    };

    enum class BracketContextType
    {
        Namespace,
        Class,
        MacroCall,
        Other
    };

    struct Context {
        std::vector<std::string> ns;
        std::vector<ClassInfo>   class_;
        InputInfo                info;
        bool                     last_is_del { true };
        std::string_view         word {};

        struct Status {
            StatusType               type {};
            std::vector<std::string> words {};
        };

        struct BracketStatus {
            BracketType              type { BracketType::Round };
            BracketContextType       ctx_type { BracketContextType::Other };
            bool                     is_open { true };
            std::vector<std::string> words {};
        };
        std::vector<Status>        status { Status { StatusType::Start } };
        std::vector<BracketStatus> bracket_stack;
    };
    Context ctx;

    auto is_next_word = [](std::string_view in, std::string_view word) -> bool {
        return in.starts_with(word) && details::is_del(in.at(word.size()));
    };

    auto has_status = [&ctx](StatusType t) {
        return std::find_if(ctx.status.begin(), ctx.status.end(), [t](auto& s) {
                   return s.type == t;
               }) != ctx.status.end();
    };

    auto has_bracket = [&ctx](BracketType t, BracketContextType ct) {
        return std::find_if(ctx.bracket_stack.begin(), ctx.bracket_stack.end(), [t, ct](auto& s) {
                   return s.type == t && s.ctx_type == ct;
               }) != ctx.bracket_stack.end();
    };

    auto open_bracket = [&ctx](BracketType type) {
        if (! ctx.bracket_stack.empty() && ! ctx.bracket_stack.back().is_open) {
            ctx.bracket_stack.back().is_open = true;
        } else {
            ctx.bracket_stack.push_back(
                Context::BracketStatus { type, BracketContextType::Other, true });
            auto s = ctx.status.back();
            switch (s.type) {
            case StatusType::MacroCall: {
                ctx.status.pop_back();
                ctx.bracket_stack.back().ctx_type = BracketContextType::MacroCall;
                break;
            }
            default: {
            }
            }
        }
    };

    auto close_bracket = [&ctx] {
        auto status = ctx.bracket_stack.back();
        switch (status.ctx_type) {
        case BracketContextType::Namespace: {
            ctx.ns.pop_back();
            break;
        }
        case BracketContextType::Class: {
            ctx.class_.pop_back();
            break;
        }
        case BracketContextType::MacroCall: {
            ctx.info.macros.back().params.emplace_back(std::string { ctx.word });
            ctx.word = {};
            break;
        }
        default: {
        }
        }
        ctx.bracket_stack.pop_back();
    };

    auto it = src.begin();

    auto eat_until = [&it, src](std::string_view ends, bool with = false) {
        while (it < src.end() && ! std::string_view { it, src.end() }.starts_with(ends)) {
            it++;
        }
        if (with) it += std::min<i32>(src.end() - it, ends.size());
    };

    auto extend_word = [&ctx, &it, src](usize i) {
        ctx.word = ctx.word.empty() ? std::string_view { it, 1 }
                                    : std::string_view { ctx.word.begin(),
                                                         std::min(ctx.word.end() + i, src.end()) };
    };

    auto emit_word = [&ctx, &it, has_bracket, src, macro_names]() {
        if (has_bracket(BracketType::Round, BracketContextType::MacroCall)) {
        } else {
            cpp_rule::state          state;
            auto                     t = std::string_view { ctx.word.begin(), src.end() };
            tao::pegtl::memory_input input(t, "src");
            tao::pegtl::parse<grammer_cpp::text, cpp_rule::action>(input, state);
            if (state.end) {
                it = ctx.word.begin() + state.end.value();
                it--;
                std::visit(overloaded {
                               [](std::monostate) {
                               },
                               [&ctx](const cpp_rule::res::declare_namespace& s) {
                                   ctx.ns.emplace_back(s.name.empty()
                                                           ? "<anonymous>"sv
                                                           : std::string_view { s.name });
                                   ctx.bracket_stack.push_back(Context::BracketStatus {
                                       BracketType::Curly, BracketContextType::Namespace, false });
                               },
                               [&ctx](const cpp_rule::res::declare_class& s) {
                                   ctx.class_.push_back({ s.name, s.parent, s.is_struct });

                                   ctx.bracket_stack.push_back(Context::BracketStatus {
                                       BracketType::Curly, BracketContextType::Class, false });
                               } },
                           state.result);
            } else {
                for (auto& mn : macro_names) {
                    if (ctx.word == mn) {
                        ctx.info.macros.push_back(MacroInfo { .ns     = ctx.ns,
                                                              .class_ = ctx.class_,
                                                              .name   = std::string(mn),
                                                              .params = {} });
                        ctx.status.push_back({ StatusType::MacroCall });
                        break;
                    }
                }
            }
            ctx.word = {};
        }
    };

    auto do_del = [emit_word, &ctx]() {
        if (! ctx.last_is_del) emit_word();
        ctx.last_is_del = true;
    };

    auto do_comma = [&ctx, do_del] {
        if (! ctx.bracket_stack.empty() &&
            ctx.bracket_stack.back().ctx_type == BracketContextType::MacroCall) {
            ctx.info.macros.back().params.push_back(std::string { ctx.word });
            ctx.word = {};
        }
    };

    while (it < src.end()) {
        auto c   = *it;
        auto sub = std::string_view { it, src.end() };
        if (sub.starts_with("//")) {
            do_del();
            eat_until("\n", true);
            continue;
        } else if (sub.starts_with("/*")) {
            do_del();
            eat_until("*/", true);
            continue;
        } else if (sub.starts_with("::")) {
            ctx.last_is_del = false;
            extend_word(2);
            it += 2;
            continue;
        } else if (c == ',') {
            do_del();
            do_comma();
        } else if (c == '(') {
            do_del();
            open_bracket(BracketType::Round);
        } else if (c == '[') {
            do_del();
            open_bracket(BracketType::Square);
        } else if (c == '{') {
            do_del();
            open_bracket(BracketType::Curly);
        } else if (c == '}' || c == ')' || c == ']') {
            do_del();
            _assert_msg_rel_(ctx.bracket_stack.size() && ctx.bracket_stack.back().is_open,
                             "wrong pair of {{}}");
            close_bracket();
        } else if (details::is_del(c)) {
            do_del();
        } else {
            ctx.last_is_del = false;
            extend_word(1);
        }
        it++;
    }
    return ctx.info;
}

auto collect(const InputInfo& info) -> OutputInfo {
    OutputInfo out;

    auto collect_class_name = [](const MacroInfo& m) -> std::string {
        auto view = std::ranges::transform_view(m.class_, [](auto& c) -> std::string {
            return c.name;
        });
        return fmt::format("{}", fmt::join(view, "::"));
    };
    for (auto& m : info.macros) {
        if (m.name == DECLARE_MODEL) {
            _assert_rel_(! m.class_.empty());
            OutputInfo::ModelInfo model { .ns       = m.ns,
                                          .fullname = collect_class_name(m),
                                          .name     = m.class_.back().name,
                                          .parent   = m.class_.back().parent,
                                          .vars     = {} };

            for (auto& p : m.params) {
                if (p.ends_with(MT_COPY)) {
                    model.copyable = true;
                }
            }
            out.models.push_back(model);
        } else if (m.name == DECLARE_PROPERTY) {
            if (! out.models.empty() && m.ns == out.models.back().ns) {
                auto& model = out.models.back();
                auto  name  = collect_class_name(m);
                if (name == model.name) {
                    _assert_msg_rel_(m.params.size() == 3, "wrong param of {}", DECLARE_PROPERTY);
                    model.vars.push_back({
                        .type = m.params.at(0),
                        .name = m.params.at(1),
                        .sig  = m.params.at(2),
                    });
                }
            }
        }
    }
    return out;
}

auto format_model(const OutputInfo::ModelInfo& info) -> std::tuple<std::string, std::string> {
    auto parent = info.parent.starts_with("qcm::model::"sv)
                      ? info.parent.substr("qcm::model::"sv.size())
                      : info.parent;

    auto model_class = details::replace_identity(
        parent, info.name, fmt::format("{}::{}", fmt::join(info.ns, "::"), info.fullname));

    auto view_members    = std::ranges::transform_view(info.vars, [](auto& v) -> std::string {
        return fmt::format("{} {}", v.type, v.name);
    });
    auto view_prop_funcs = std::ranges::transform_view(
        info.vars, [&info, parent](const OutputInfo::VarInfo& v) -> std::string {
            return fmt::format(R"(
auto {}::{}() const -> const {}& {{
    auto d = qcm::model::{}::d_func();
    return d->{};
}}

void {}::set_{}(const {}& val) {{
    auto d = qcm::model::{}::d_func();
    auto& p = d->{};
    p = val;
}}
)",
                               info.fullname,
                               v.name,
                               v.type,
                               parent,
                               v.name,
                               // set
                               info.fullname,
                               v.name,
                               v.type,
                               parent,
                               v.name);
        });

    std::string forward_declare { fmt::format("namespace {} {{ {} {}; }}",
                                              fmt::join(info.ns, "::"),
                                              info.is_struct ? "struct" : "class",
                                              info.name) };

    std::string copy;
    if (info.copyable) {
        copy = fmt::format(R"(

{}::Model(const Model& m) : Model() {{
    Base::Base(static_cast<const Base&>(m));
    *(this->m_ptr) = *(m.m_ptr);
}}
Model& {}::operator=(const Model& m) {{
    Base::operator = (m);
    *(this->m_ptr) = *(m.m_ptr);
    return *this;
}}
)",
                           model_class,
                           model_class);
    }

    return { fmt::format(
                 R"(
{}
namespace qcm::model {{

template<>
class {}::Private {{
public:
{};
}};

template<>
{}::Model() : m_ptr(make_up<Private>()) {{}}

template<>
{}::~Model() {{}}

{}

}}

)",
                 forward_declare,
                 model_class,
                 fmt::join(view_members, ";\n"),
                 model_class, // ct
                 model_class, // dct
                 copy),
             fmt::format(R"(
namespace {} {{
{}
}}
)",
                         fmt::join(info.ns, "::"),
                         fmt::join(view_prop_funcs, "\n")) };
};

auto generate(const OutputInfo& info, std::filesystem::path include) -> std::string {
    std::string out { R"(
#include "qcm_interface/model.h"
)" };
    std::string out_ { fmt::format(R"(

#include "{}"
)",
                                   include.string()) };
    for (auto& m : info.models) {
        auto [pre, normal] = format_model(m);
        out.append(pre);
        out_.append(normal);
    }
    out.append(out_);
    return out;
}

int main(int argc, char** argv) {
    QCoreApplication   app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("code generator for qcm model");
    parser.addHelpOption();
    parser.addPositionalArgument("source", "");
    parser.addPositionalArgument("destination", "");
    parser.process(app);

    std::filesystem::path in, out;
    {
        auto args    = parser.positionalArguments();
        in           = args.at(0).toStdString();
        out          = args.at(1).toStdString();
        in           = std::filesystem::absolute(in);
        out          = std::filesystem::absolute(out);
        auto out_dir = out.parent_path();
        if (! std::filesystem::exists(out_dir)) std::filesystem::create_directories(out_dir);
    }
    std::fstream in_file(in, std::ios_base::in), out_file(out, std::ios_base::out);

    in_file.exceptions(std::ios_base::failbit);
    out_file.exceptions(std::ios_base::failbit);

    auto in_str =
        std::string(std::istreambuf_iterator<char>(in_file), std::istreambuf_iterator<char> {});

    InputInfo   info    = parse(in_str, std::array { DECLARE_MODEL, DECLARE_PROPERTY });
    std::string content = generate(collect(info), in);
    out_file.write(content.c_str(), content.size());
}