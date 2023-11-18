#include "uri.h"

#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/integer.hpp>

#include "core/core.h"

namespace pegtl = tao::pegtl;

namespace request
{

/* -------------------------------------------------------------------------- */
/*                                grammer                                     */
/* -------------------------------------------------------------------------- */

namespace grammer_uri
{
using namespace tao::pegtl;

// according to https://datatracker.ietf.org/doc/html/rfc3986

// pct-encoded = "%" HEXDIG HEXDIG
struct pct_encoded : seq<one<'%'>, abnf::HEXDIG, abnf::HEXDIG> {};

// reserved    = gen-delims / sub-delims
// gen-delims  = ":" / "/" / "?" / "#" / "[" / "]" / "@"
// sub-delims  = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
struct gen_delims : one<':', '/', '?', '#', '[', ']', '@'> {};
struct sub_delims : one<'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='> {};
struct reserved : sor<gen_delims, sub_delims> {};

// unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
struct unreserved : sor<abnf::ALPHA, abnf::DIGIT, one<'-', '.', '_', '~'>> {};

// scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
struct scheme : seq<abnf::ALPHA, star<sor<abnf::ALPHA, abnf::DIGIT, one<'+', '-', '.'>>>> {};

// userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
struct userinfo : star<sor<unreserved, pct_encoded, sub_delims, one<':'>>> {};

//      IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
//      dec-octet   = DIGIT                 ; 0-9
//                  / %x31-39 DIGIT         ; 10-99
//                  / "1" 2DIGIT            ; 100-199
//                  / "2" %x30-34 DIGIT     ; 200-249
//                  / "25" %x30-35          ; 250-255
struct dec_octet : maximum_rule<std::uint8_t> {};
struct IPv4address : seq<dec_octet, one<'.'>, dec_octet, one<'.'>, dec_octet, one<'.'>, dec_octet> {
};

// IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
struct IPvFuture : seq<one<'v', 'V'>, plus<abnf::HEXDIG>, one<'.'>,
                       plus<sor<unreserved, sub_delims, one<':'>>>> {};

//      ls32        = ( h16 ":" h16 ) / IPv4address ; least-significant 32 bits of address
//      h16         = 1*4HEXDIG ; 16 bits of address represented in hexadecimal
//      IPv6address =                            6( h16 ":" ) ls32
//                  /                       "::" 5( h16 ":" ) ls32
//                  / [               h16 ] "::" 4( h16 ":" ) ls32
//                  / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
//                  / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
//                  / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
//                  / [ *4( h16 ":" ) h16 ] "::"              ls32
//                  / [ *5( h16 ":" ) h16 ] "::"              h16
//                  / [ *6( h16 ":" ) h16 ] "::"
struct h16 : rep_min_max<1, 4, abnf::HEXDIG> {};
struct ls32 : sor<seq<h16, one<':'>, h16>, IPv4address> {};
// clang-format off
struct IPv6address : sor<
    seq<                                                    rep<6, seq<h16, one<':'>>>, ls32>,
    seq<                                          two<':'>, rep<5, seq<h16, one<':'>>>, ls32>,
    seq<opt<                                h16>, two<':'>, rep<4, seq<h16, one<':'>>>, ls32>,
    seq<opt<opt<       seq<h16, one<':'>>>, h16>, two<':'>, rep<3, seq<h16, one<':'>>>, ls32>,
    seq<opt<rep_opt<2, seq<h16, one<':'>>>, h16>, two<':'>, rep<2, seq<h16, one<':'>>>, ls32>,
    seq<opt<rep_opt<3, seq<h16, one<':'>>>, h16>, two<':'>,            h16, one<':'>  , ls32>,
    seq<opt<rep_opt<4, seq<h16, one<':'>>>, h16>, two<':'>,                             ls32>,
    seq<opt<rep_opt<5, seq<h16, one<':'>>>, h16>, two<':'>,                             h16 >,
    seq<opt<rep_opt<6, seq<h16, one<':'>>>, h16>, two<':'>                                  >
> {};
// clang-format on

// IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
struct IP_literal : seq<one<'['>, sor<IPv6address, IPvFuture>, one<']'>> {};

// reg-name    = *( unreserved / pct-encoded / sub-delims )
struct reg_name : star<sor<unreserved, pct_encoded, sub_delims>> {};

// port        = *DIGIT
struct port : star<abnf::DIGIT> {};

//      path          = path-abempty    ; begins with "/" or is empty
//                    / path-absolute   ; begins with "/" but not "//"
//                    / path-noscheme   ; begins with a non-colon segment
//                    / path-rootless   ; begins with a segment
//                    / path-empty      ; zero characters
//
//      path-abempty  = *( "/" segment )
//      path-absolute = "/" [ segment-nz *( "/" segment ) ]
//      path-noscheme = segment-nz-nc *( "/" segment )
//      path-rootless = segment-nz *( "/" segment )
//      path-empty    = 0<pchar>
//
//      segment       = *pchar
//      segment-nz    = 1*pchar
//      segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
//                    ; non-zero-length segment without any colon ":"
//
//      pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
struct pchar : sor<unreserved, pct_encoded, sub_delims, one<':'>, one<'@'>> {};
struct segment : star<pchar> {};
struct segment_nz : plus<pchar> {};
struct segment_nz_nc : plus<sor<unreserved, pct_encoded, sub_delims, one<'@'>>> {};

struct path_abempty : star<seq<one<'/'>, segment>> {};
struct path_absolute : seq<one<'/'>, opt<segment_nz, star<seq<one<'/'>, segment>>>> {};
struct path_noscheme : seq<segment_nz_nc, star<seq<one<'/'>, segment>>> {};
struct path_rootless : seq<segment_nz, star<seq<one<'/'>, segment>>> {};
struct path_empty : success {};

struct path : sor<path_absolute, path_noscheme, path_rootless, path_abempty, path_empty> {
}; // smarll first

// query       = *( pchar / "/" / "?" )
struct query : star<sor<pchar, one<'/'>, one<'?'>>> {};

// fragment    = *( pchar / "/" / "?" )
struct fragment : star<sor<pchar, one<'/'>, one<'?'>>> {};

//  host        = IP-literal / IPv4address / reg-name
struct host : sor<IP_literal, IPv4address, reg_name> {};

// authority   = [ userinfo "@" ] host [ ":" port ]
struct authority : seq<opt<userinfo, one<'@'>>, host, opt<one<':'>, port>> {};

//      URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
//
//      hier-part   = "//" authority path-abempty
//                  / path-absolute
//                  / path-rootless
//                  / path-empty
//
//   The scheme and path components are required, though the path may be
//   empty (no characters).  When authority is present, the path must
//   either be empty or begin with a slash ("/") character.  When
//   authority is not present, the path cannot begin with two slash
//   characters ("//").  These restrictions result in five different ABNF
//   rules for a path (Section 3.3), only one of which will match any
//   given URI reference.
//
//   The following are two example URIs and their component parts:
//
//         foo://example.com:8042/over/there?name=ferret#nose
//         \_/   \______________/\_________/ \_________/ \__/
//          |           |            |            |        |
//       scheme     authority       path        query   fragment
//          |   _____________________|__
//         / \ /                        \ ;
//         urn:example:animal:ferret:nose
struct hier_part
    : sor<seq<two<'/'>, authority, path_abempty>, path_absolute, path_rootless, path_empty> {};
struct URI : seq<scheme, one<':'>, hier_part, opt<one<'?'>, query>, opt<one<'#'>, fragment>> {};

// relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
//
// relative-part = "//" authority path-abempty
//               / path-absolute
//               / path-noscheme
//               / path-empty
struct relative_part
    : sor<seq<two<'/'>, authority, path_abempty>, path_absolute, path_noscheme, path_empty> {};
struct relative_ref : seq<relative_part, opt<one<'?'>, query>, opt<one<'#'>, fragment>> {};

// URI-reference = URI / relative-ref
struct URI_reference : sor<URI, relative_ref> {};

// absolute-URI  = scheme ":" hier-part [ "?" query ]
struct absolute_URI : seq<scheme, one<':'>, hier_part, opt<one<'?'>, query>> {};

} // namespace grammer_uri

/* -------------------------------------------------------------------------- */
/*                                code                                        */
/* -------------------------------------------------------------------------- */

namespace uri
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

} // namespace uri

URI::URI(): m_valid(false) {}

URI::URI(std::string_view sin): URI() {
    using grammer = grammer_uri::URI;
    m_holder      = sin;
    uri           = m_holder;

    pegtl::memory_input input(uri.data(), uri.size(), "uri");
    m_valid = pegtl::parse<grammer, uri::action, uri::control>(input, *this);
}
URI::~URI() {}
URI::URI(const URI& o): URI() { *this = o; }
URI& URI::operator=(const URI& o) {
    m_holder        = o.m_holder;
    uri             = m_holder;
    m_valid         = o.m_valid;

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

bool URI::valid() const {
    return m_valid;
}

} // namespace request

// namespace test