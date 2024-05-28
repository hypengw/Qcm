#include "uri.h"
#include "http_header.h"

#include <charconv>

#include "core/core.h"
#include "peg/uri.h"
#include "peg/http.h"

#include "core/log.h"
#include "core/variant_helper.h"
#include "core/str_helper.h"

namespace pegtl = tao::pegtl;

namespace request
{

namespace uri_rule
{

template<std::string_view URI::*Field>
struct bind {
    template<typename Rule>
    static void reset(URI& uri) {
        auto f     = Field;
        uri.*Field = {};
    }

    template<typename ActionInput>
    static void apply(const ActionInput& in, URI& uri) {
        if (Field == &URI::authority) {
            int i = 0;
        }
        uri.*Field = in.string_view();
    }
};

// clang-format off
template< typename Rule > struct action {};

template<> struct action< grammer_uri::scheme > : bind< &URI::scheme > {};
template<> struct action< grammer_uri::authority > : bind< &URI::authority > {};
template<> struct action< grammer_uri::userinfo > : bind< &URI::userinfo > {};
template<> struct action< grammer_uri::host > : bind< &URI::host > {};
template<> struct action< grammer_uri::port > : bind< &URI::port > {};
template<> struct action< grammer_uri::path_noscheme > : bind< &URI::path > {};
template<> struct action< grammer_uri::path_rootless > : bind< &URI::path > {};
template<> struct action< grammer_uri::path_absolute > : bind< &URI::path > {};
template<> struct action< grammer_uri::path_abempty > : bind< &URI::path > {};
template<> struct action< grammer_uri::query > : bind< &URI::query > {};
template<> struct action< grammer_uri::fragment > : bind< &URI::fragment > {};
// clang-format on

template<typename Rule>
void reset(URI& uri) {
    if constexpr (requires { action<Rule>::template reset<Rule>; }) {
        action<Rule>::template reset<Rule>(uri);
    }
}

template<typename Rule>
struct control : pegtl::normal<Rule> {
    static constexpr bool enable = pegtl::internal::enable_control<Rule> ||
                                   core::is_specialization_of<Rule, pegtl::internal::seq>;

    template<typename ParseInput, typename... States>
    static void failure(const ParseInput&, States&&... states) noexcept {
        auto resets = []<typename... Rules>(pegtl::type_list<Rules...>, request::URI& uri) {
            (reset<Rules>(uri), ...);
        };
        resets(typename Rule::subs_t(), states...);

        reset<Rule>(states...);
    }
};

} // namespace uri_rule

namespace http_rule
{

template<typename Rule>
struct action {};

template<>
struct action<grammer_http::field_name> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, HttpHeader::Field& f) {
        f.name = in.string_view();
    }
};

template<>
struct action<grammer_http::field_value> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, HttpHeader::Field& f) {
        f.value = in.string_view();
    }
};

template<>
struct action<grammer_http::header_field_line> {
    template<typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M,
             template<typename...> class Action, template<typename...> class Control,
             typename ParseInput>
    static bool match(ParseInput& in, HttpHeader& header) {
        HttpHeader::Field f;
        auto              ok = pegtl::match<Rule, A, M, Action, Control>(in, f);
        if (ok) header.fields.push_back(f);
        return ok;
    }
};

template<typename T>
struct StartLineHelperAction {
    template<typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M,
             template<typename...> class Action, template<typename...> class Control,
             typename ParseInput>
    static bool match(ParseInput& in, HttpHeader::Start& s) {
        s = T {};
        return pegtl::match<Rule, A, M, Action, Control>(in, s);
    }
    template<typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M,
             template<typename...> class Action, template<typename...> class Control,
             typename ParseInput>
    static bool match(ParseInput& in, HttpHeader& f) {
        f.start = HttpHeader::Start {};
        return match<Rule, A, M, Action, Control>(in, f.start.value());
    }
};

template<>
struct action<grammer_http::request_line> : StartLineHelperAction<HttpHeader::Request> {};
template<>
struct action<grammer_http::status_line> : StartLineHelperAction<HttpHeader::Status> {};

template<>
struct action<grammer_http::HTTP_version> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, HttpHeader::Start& s) {
        std::visit(overloaded { [&in](auto& s) {
                       s.version = in.string_view();
                   } },
                   s);
    }
};
template<>
struct action<grammer_http::request_target> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, HttpHeader::Start& s_) {
        if (auto s = std::get_if<HttpHeader::Request>(&s_)) {
            s->target = in.string_view();
        }
    }
};
template<>
struct action<grammer_http::methon> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, HttpHeader::Start& s_) {
        if (auto s = std::get_if<HttpHeader::Request>(&s_)) {
            s->method = in.string_view();
        }
    }
};
template<>
struct action<grammer_http::status_code> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, HttpHeader::Start& s_) {
        if (auto s = std::get_if<HttpHeader::Status>(&s_)) {
            auto str = in.string_view();
            std::from_chars(str.begin(), str.end(), s->code);
        }
    }
};

} // namespace http_rule

URI::URI(): m_valid(false) {}

URI::URI(std::string_view sin): URI() {
    using grammer = grammer_uri::URI;
    m_holder      = sin;
    uri           = m_holder;

    pegtl::memory_input input(uri.data(), uri.size(), "uri");
    m_valid = pegtl::parse<grammer, uri_rule::action, uri_rule::control>(input, *this);
}
URI::~URI() {}
URI::URI(const URI& o): URI() { *this = o; }
URI& URI::operator=(const URI& o) {
    m_holder = o.m_holder;
    uri      = m_holder;
    m_valid  = o.m_valid;

    auto trans = [old = o.uri, uri = uri](std::string_view in, std::string_view& out) {
        if (in.data()) out = uri.substr(std::distance(old.data(), in.data()), in.size());
    };

    trans(o.scheme, scheme);
    trans(o.authority, authority);
    trans(o.userinfo, userinfo);
    trans(o.host, host);
    trans(o.port, port);
    trans(o.path, path);
    trans(o.query, query);
    trans(o.fragment, fragment);
    return *this;
}

URI URI::from(std::string_view in) { return URI(in); }

bool URI::valid() const { return m_valid; }

auto HttpHeader::parse_header(std::string_view in) -> std::optional<HttpHeader> {
    HttpHeader          out;
    pegtl::memory_input input(in.data(), in.size(), "http");
    if (pegtl::parse<grammer_http::HTTP_message, http_rule::action>(input, out))
        return out;
    else
        return std::nullopt;
}

auto HttpHeader::parse_start_line(std::string_view in) -> std::optional<Start> {
    HttpHeader::Start out;

    pegtl::memory_input input(in.data(), in.size(), "http start line");
    if (pegtl::parse<grammer_http::start_line, http_rule::action>(input, out))
        return out;
    else
        return std::nullopt;
}
auto HttpHeader::parse_field_line(std::string_view in) -> Field {
    HttpHeader::Field out;

    pegtl::memory_input input(in.data(), in.size(), "http filed line");
    pegtl::parse<grammer_http::header_field_line, http_rule::action>(input, out);
    return out;
}
auto HttpHeader::has_field(std::string_view name) const -> bool {
    return this->fields.end() !=
           std::find_if(this->fields.begin(), this->fields.end(), [name](auto& f) {
               return helper::starts_with_i(f.name, name);
           });
}
} // namespace request

// namespace test