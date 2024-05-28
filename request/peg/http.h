#pragma once

#include <tao/pegtl/ascii.hpp>
#include <tao/pegtl/contrib/remove_first_state.hpp>
#include "peg/uri.h"

// https://datatracker.ietf.org/doc/html/rfc7230
namespace grammer_http
{
using namespace tao::pegtl;
namespace uri = grammer_uri;

// HTTP-version  = HTTP-name "/" DIGIT "." DIGIT
// HTTP-name     = %x48.54.54.50 ; "HTTP", case-sensitive
struct HTTP_name : string<'H', 'T', 'T', 'P'> {};
struct HTTP_version : seq<HTTP_name, one<'/'>, abnf::DIGIT, opt_must<one<'.'>, abnf::DIGIT>> {};

// http-URI = "http:" "//" authority path-abempty [ "?" query ] [ "#" fragment ]
struct http_URI
    : if_must<ascii::istring<'h', 't', 't', 'p', ':', '/', '/'>, uri::authority, uri::path_abempty,
              opt_must<one<'?'>, uri::query>, opt_must<one<'#'>, uri::fragment>> {};

// https-URI = "https:" "//" authority path-abempty [ "?" query ] [ "#" fragment ]
struct https_URI : if_must<ascii::istring<'h', 't', 't', 'p', 's', ':', '/', '/'>, uri::authority,
                           uri::path_abempty, opt_must<one<'?'>, uri::query>,
                           opt_must<one<'#'>, uri::fragment>> {};

// message-body = *OCTET
struct message_body : star<abnf::OCTET> {};

// OWS            = *( SP / HTAB )
//                ; optional whitespace
using OWS = star<abnf::WSP>;
// RWS            = 1*( SP / HTAB )
//                ; required whitespace
using RWS = plus<abnf::WSP>;
// BWS            = OWS
//                ; "bad" whitespace
using BWS = OWS;

template<typename T>
using make_comma_list = seq<star<one<','>, OWS>, T, star<OWS, one<','>, opt<OWS, T>>>;

// obs-text       = %x80-FF
using obs_text = not_range<0x00, 0x7F>;

// ctext          = HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text
struct ctext
    : sor<abnf::HTAB, abnf::SP, range<0x21, 0x27>, range<0x2A, 0x5B>, range<0x5d, 0x7e>, obs_text> {
};

// qdtext         = HTAB / SP /%x21 / %x23-5B / %x5D-7E / obs-text
struct qdtext
    : sor<abnf::HTAB, abnf::SP, one<0x21>, range<0x23, 0x5B>, range<0x5d, 0x7e>, obs_text> {};

// quoted-pair    = "\" ( HTAB / SP / VCHAR / obs-text )
struct quoted_pair : if_must<one<'\\'>, sor<abnf::VCHAR, obs_text, abnf::WSP>> {};

// quoted-string  = DQUOTE *( qdtext / quoted-pair ) DQUOTE
struct quoted_string : seq<abnf::DQUOTE, star<sor<qdtext, quoted_pair>, abnf::DQUOTE>> {};

// comment        = "(" *( ctext / quoted-pair / comment ) ")"
struct comment : if_must<one<'('>, until<one<')'>, sor<comment, quoted_pair, ctext>>> {};

// tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
//                / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
//                / DIGIT / ALPHA
//                ; any VCHAR, except delimiters
struct tchar : sor<one<'!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~'>,
                   abnf::DIGIT, abnf::ALPHA> {};
// token          = 1*tchar
struct token : plus<tchar> {};

// obs-fold       = CRLF 1*( SP / HTAB )
//                ; obsolete line folding
//                ; see Section 3.2.4
struct obs_fold : seq<abnf::CRLF, plus<sor<abnf::SP, abnf::HTAB>>> {};

// field-name     = token
// field-value    = *( field-content / obs-fold )
// field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
// field-vchar    = VCHAR / obs-text
struct field_name : token {};
struct field_vhar : sor<abnf::VCHAR, obs_text> {};
struct field_content : list<field_vhar, plus<sor<abnf::SP, abnf::HTAB>>> {};
struct field_value : star<sor<field_content, obs_fold>> {};

// header-field   = field-name ":" OWS field-value OWS
struct header_field : seq<field_name, one<':'>, OWS, field_value, OWS> {};

// reason-phrase  = *( HTAB / SP / VCHAR / obs-text )
struct reason_phrase : star<sor<abnf::VCHAR, obs_text, abnf::WSP>> {};

// status-code    = 3DIGIT
struct status_code : rep<3, abnf::DIGIT> {};

// status-line = HTTP-version SP status-code SP reason-phrase CRLF
struct status_line
    : if_must<HTTP_version, abnf::SP, status_code, abnf::SP, reason_phrase, abnf::CRLF> {};

// method         = token
struct methon : token {};

// origin-form    = absolute-path [ "?" query ]
// absolute-form  = absolute-URI
// authority-form = authority
// asterisk-form  = "*"
struct absolute_path : plus<one<'/'>, uri::segment> {};
struct origin_form : seq<absolute_path, opt_must<one<'?'>, uri::query>> {};
struct absolute_form : uri::absolute_URI {};
struct authority_form : uri::authority {};
struct asterisk_form : one<'*'> {};

// request-target = origin-form
//                / absolute-form
//                / authority-form
//                / asterisk-form
struct request_target : sor<origin_form, absolute_form, authority_form, asterisk_form> {};

// request-line   = method SP request-target SP HTTP-version CRLF
struct request_line : seq<methon, abnf::SP, request_target, abnf::SP, HTTP_version, abnf::CRLF> {};
// start-line     = request-line / status-line
struct start_line : sor<status_line, request_line> {};

struct header_field_line : seq<header_field, abnf::CRLF> {};

// HTTP-message   = start-line
//                  *( header-field CRLF )
//                  CRLF
//                  [ message-body ]
struct HTTP_message : seq<start_line, star<header_field_line>, abnf::CRLF, opt<message_body>> {};

// transfer-parameter = token BWS "=" BWS ( token / quoted-string )
struct transfer_parameter : seq<token, BWS, one<'='>, sor<token, quoted_pair>> {};
// transfer-extension = token *( OWS ";" OWS transfer-parameter )
struct transfer_extension : seq<token, star<OWS, one<';'>, OWS, transfer_parameter>> {};
// transfer-coding    = "chunked" ; Section 4.1
//                    / "compress" ; Section 4.2.1
//                    / "deflate" ; Section 4.2.2
//                    / "gzip" ; Section 4.2.3
//                    / transfer-extension
struct transfer_coding : sor<istring<'c', 'h', 'u', 'n', 'k', 'e', 'd'>,
                             istring<'c', 'o', 'm', 'p', 'r', 'e', 's', 's'>,
                             istring<'d', 'e', 'f', 'l', 'a', 't', 'e'>,
                             istring<'g', 'z', 'i', 'p'>, transfer_extension> {};
// Transfer-Encoding = 1#transfer-coding
struct Transfer_Encoding : make_comma_list<transfer_coding> {};
// Content-Length = 1*DIGIT
struct Content_Length : plus<abnf::DIGIT> {};

// chunk-size     = 1*HEXDIG
// chunk-ext-name = token
// chunk-ext-val  = token / quoted-string
// chunk-ext      = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
struct chunk_size : plus<abnf::HEXDIG> {};
struct chunk_ext_name : token {};
struct chunk_ext_val : sor<token, quoted_string> {};
struct chunk_ext : star<one<';'>, chunk_ext_name, if_must<one<'='>, chunk_ext_val>> {};

// last-chunk     = 1*("0") [ chunk-ext ] CRLF
struct last_chunk : seq<plus<one<'0'>>, not_at<digit>, chunk_ext, abnf::CRLF> {};

// chunk-data     = 1*OCTET ; a sequence of chunk-size octets
struct chunk_data {
    using rule_t = star<abnf::OCTET>::rule_t;

    template<apply_mode A, rewind_mode M, template<typename...> class Action,
             template<typename...> class Control, typename ParseInput, typename... States>
    [[nodiscard]] static bool match(ParseInput& in, const std::size_t size,
                                    States&&... /*unused*/) {
        if (in.size(size) >= size) {
            in.bump(size);
            return true;
        }
        return false;
    }
};

namespace internal::chunk_helper
{
template<typename Base>
struct control;

template<template<typename...> class Control, typename Rule>
struct control<Control<Rule>> : Control<Rule> {
    template<apply_mode A, rewind_mode M, template<typename...> class Action,
             template<typename...> class, typename ParseInput, typename State, typename... States>
    [[nodiscard]] static bool match(ParseInput& in, State&& /*unused*/, States&&... st) {
        return Control<Rule>::template match<A, M, Action, Control>(in, st...);
    }
};

template<template<typename...> class Control>
struct control<Control<chunk_size>> : remove_first_state<Control<chunk_size>> {};

template<template<typename...> class Control>
struct control<Control<chunk_data>> : remove_first_state<Control<chunk_data>> {};

template<template<typename...> class Control>
struct bind {
    template<typename Rule>
    using type = control<Control<Rule>>;
};

} // namespace internal::chunk_helper

// chunk          = chunk-size [ chunk-ext ] CRLF
struct chunk {
    using impl = seq<chunk_size, chunk_ext, abnf::CRLF, chunk_data, abnf::CRLF>;

    using rule_t = impl::rule_t;

    template<apply_mode A, rewind_mode M, template<typename...> class Action,
             template<typename...> class Control, typename ParseInput, typename... States>
    [[nodiscard]] static bool match(ParseInput& in, States&&... st) {
        std::size_t size {};
        return impl::
            template match<A, M, Action, internal::chunk_helper::bind<Control>::template type>(
                in, size, st...);
    }
};

// trailer-part   = *( header-field CRLF )
struct trailer_part : star<header_field, abnf::CRLF> {};

// chunked-body   = *chunk
//                  last-chunk
//                  trailer-part
//                  CRLF
//
//
struct chunked_body : seq<until<last_chunk, chunk>, trailer_part, abnf::CRLF> {};

// TE        = #t-codings
// t-codings = "trailers" / ( transfer-coding [ t-ranking ] )
// t-ranking = OWS ";" OWS "q=" rank
// rank      = ( "0" [ "." 0*3DIGIT ] )
//            / ( "1" [ "." 0*3("0") ] )
// Trailer = 1#field-name
struct Trailer : make_comma_list<field_name> {};
struct rank : sor<seq<one<'0'>, opt<one<'.'>, rep_opt<3, abnf::DIGIT>>>,
                  seq<one<'1'>, opt<one<'.'>, rep_opt<3, one<'0'>>>>> {};
struct t_ranking : seq<OWS, one<';'>, OWS, one<'q', 'Q'>, one<'='>, rank> {};
struct t_codings
    : sor<istring<'t', 'r', 'a', 'i', 'l', 'e', 'r', 's'>, seq<transfer_coding, opt<t_ranking>>> {};
struct TE : opt<sor<one<','>, t_codings>, star<OWS, one<','>, opt<OWS, t_codings>>> {};

// Upgrade          = 1#protocol
// protocol         = protocol-name ["/" protocol-version]
// protocol-name    = token
// protocol-version = token
struct protocol_name : token {};
struct protocol_version : token {};
struct protocol : seq<protocol_name, opt<one<'/'>, protocol_version>> {};
struct Upgrade : make_comma_list<protocol> {};

// Via = 1#( received-protocol RWS received-by [ RWS comment ] )
//
// received-protocol = [ protocol-name "/" ] protocol-version
//                     ; see Section 6.7
// received-by       = ( uri-host [ ":" port ] ) / pseudonym
// pseudonym         = token
struct received_protocol : seq<opt<protocol_name, one<'/'>>, protocol_version> {};
struct pseudonym : token {};
struct received_by : sor<seq<uri::host, opt<one<':'>, uri::port>>, pseudonym> {};
struct Via : make_comma_list<seq<received_protocol, RWS, received_by, opt<RWS, comment>>> {};

// Connection        = 1#connection-option
// connection-option = token
struct connection_option : token {};
struct Connection : make_comma_list<connection_option> {};

} // namespace grammer_http