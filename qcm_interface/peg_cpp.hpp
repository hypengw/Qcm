#pragma once

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/integer.hpp>

namespace grammer_cpp
{
using namespace tao::pegtl;

struct kw_struct : string<'s', 't', 'r', 'u', 'c', 't'> {};
struct kw_class : string<'c', 'l', 'a', 's', 's'> {};
struct kw_using : string<'u', 's', 'i', 'n', 'g'> {};
struct kw_namespace : string<'n', 'a', 'm', 'e', 's', 'p', 'a', 'c', 'e'> {};
struct kw_public : string<'p', 'u', 'b', 'l', 'i', 'c'> {};
struct kw_private : string<'p', 'r', 'i', 'v', 'a', 't', 'e'> {};
struct kw_protected : string<'p', 'r', 'o', 't', 'e', 'c', 't', 'e', 'd'> {};

struct _ : star<sor<abnf::WSP, abnf::CR, abnf::LF>> {};

struct name : seq<abnf::ALPHA, star<sor<abnf::ALPHA, abnf::DIGIT, one<'_'>>>> {};
struct namespace_name : name {};
struct ntemplate_class_name : name {};
struct class_name;
struct class_template_params : seq<class_name, _, star<seq<one<','>, _, class_name>>> {};
struct class_name
    : seq<ntemplate_class_name, opt<one<'<'>, _, class_template_params, _, one<'>'>>> {};

struct dec_namespace_name : seq<namespace_name, star<seq<string<':', ':'>, namespace_name>>> {};

namespace end
{
struct end {};
struct dec_namespace : one<'{'>, end {};
struct dec_class : one<'{'>, end {};
} // namespace end

struct dec_namespace : seq<kw_namespace, _, dec_namespace_name, _, end::dec_namespace> {};

namespace alias
{
struct dec_class_inherit_name : class_name {};
struct dec_class_name : class_name {};
} // namespace alias
struct inherit : seq<one<':'>, _, opt<sor<kw_public, kw_private, kw_protected>>, _,
                     alias::dec_class_inherit_name> {};

struct dec_class
    : seq<sor<kw_class, kw_struct>, _, alias::dec_class_name, _, opt<inherit>, _, end::dec_class> {
};

struct text : sor<dec_namespace, dec_class> {};

} // namespace grammer_cpp