export module qcm.backend.macro;

import std;
import pmacro;

namespace
{
using Output = std::expected<pmacro::TokenStream, pmacro::Error>;

struct Error {
    pmacro::Span span;
    std::string  message;
};
template<class T>
using Checked = std::expected<T, Error>;

auto render(pmacro::TokenTree token) -> std::string;
auto render(pmacro::TokenStream tokens) -> std::string {
    std::string result;
    for (auto token : tokens) {
        if (! result.empty()) result += ' ';
        result += render(token);
    }
    return result;
}
auto render(pmacro::TokenTree token) -> std::string {
    if (token.kind() != pmacro::TokenKind::Group) return std::string(token.spelling());
    auto body = render(token.children());
    switch (token.delimiter()) {
    case pmacro::Delimiter::Parenthesis: return '(' + body + ')';
    case pmacro::Delimiter::Brace: return '{' + body + '}';
    case pmacro::Delimiter::Bracket: return '[' + body + ']';
    case pmacro::Delimiter::None: return body;
    }
    std::unreachable();
}
auto punct(pmacro::TokenTree token, std::string_view text) -> bool {
    return token.kind() == pmacro::TokenKind::Punct && token.spelling() == text;
}
auto group(pmacro::TokenTree token, pmacro::Delimiter delimiter) -> bool {
    return token.kind() == pmacro::TokenKind::Group && token.delimiter() == delimiter;
}
auto fail(pmacro::TokenTree token, std::string message) -> std::unexpected<Error> {
    return std::unexpected(Error { token.span(), std::move(message) });
}

struct Field {
    std::string name;
    std::string type;
    std::string converter;
};
struct Entity {
    std::string        input;
    std::string        fields;
    std::string        model;
    std::vector<Field> members;
    std::vector<Field> extras;
};

auto fields(pmacro::TokenTree body, std::set<std::string>& names, bool extra)
    -> Checked<std::vector<Field>> {
    if (! group(body, pmacro::Delimiter::Brace)) return fail(body, "expected a field block");
    auto               tokens = body.children();
    std::vector<Field> result;
    for (std::size_t index = 0; index < tokens.size();) {
        auto name = tokens[index++];
        if (name.kind() != pmacro::TokenKind::Ident) return fail(name, "expected a field name");
        Field field { std::string(name.spelling()), {}, {} };
        if (! names.insert(field.name).second)
            return fail(name, "duplicate or reserved field '" + field.name + "'");
        if (index == tokens.size() || ! punct(tokens[index], ":"))
            return fail(name, "expected ':' after field name");
        ++index;
        int  depth     = 0;
        bool converted = false;
        while (index < tokens.size()) {
            auto token = tokens[index];
            if (depth == 0 && punct(token, ",")) break;
            if (depth == 0 && group(token, pmacro::Delimiter::Bracket)) {
                const auto options = token.children();
                if (extra || converted || options.size() != 2 ||
                    options[0].kind() != pmacro::TokenKind::Ident ||
                    options[0].spelling() != "convert" ||
                    ! group(options[1], pmacro::Delimiter::Parenthesis) ||
                    options[1].children().size() == 0)
                    return fail(token, "expected one [convert(Type)] on a mapped field");
                field.converter = render(options[1].children());
                converted       = true;
                ++index;
                if (index < tokens.size() && ! punct(tokens[index], ","))
                    return fail(tokens[index], "expected ',' after field conversion");
                break;
            }
            if (punct(token, "<")) ++depth;
            if (punct(token, ">")) --depth;
            if (punct(token, ">>")) depth -= 2;
            if (depth < 0) return fail(token, "unbalanced template arguments in field type");
            if (! field.type.empty()) field.type += ' ';
            field.type += render(token);
            ++index;
        }
        if (field.type.empty()) return fail(name, "expected a field type");
        if (depth != 0) return fail(name, "unbalanced template arguments in field type");
        result.push_back(std::move(field));
        if (index < tokens.size()) ++index;
    }
    return result;
}

auto parse(pmacro::FunctionInput input) -> Checked<Entity> {
    const auto tokens = input.arguments;
    if (tokens.size() < 7)
        return std::unexpected(
            Error { input.invocation_span, "expected Input, Fields, Model, { fields }" });
    Entity      result;
    std::size_t index = 0;
    for (auto* name : { &result.input, &result.fields }) {
        auto token = tokens[index++];
        if (token.kind() != pmacro::TokenKind::Ident)
            return fail(token, "input and fields names must be identifiers");
        *name = token.spelling();
        if (! punct(tokens[index], ",")) return fail(tokens[index], "expected ',' after type name");
        ++index;
    }
    if (result.input == result.fields)
        return fail(tokens[2], "input and fields types must have different names");
    bool identifier = false;
    while (index < tokens.size() && ! punct(tokens[index], ",")) {
        auto token = tokens[index++];
        if (token.kind() == pmacro::TokenKind::Ident && ! identifier) {
            identifier = true;
        } else if (punct(token, "::") && (identifier || result.model.empty())) {
            identifier = false;
        } else {
            return fail(token, "model must be a qualified type name");
        }
        result.model += token.spelling();
    }
    if (! identifier || index == tokens.size())
        return std::unexpected(Error { input.invocation_span, "expected Model, { fields }" });
    ++index;
    if (index == tokens.size())
        return std::unexpected(Error { input.invocation_span, "expected a field block" });
    std::set<std::string> names { "item",          "id",
                                  "on_insert",     "apply",
                                  "validate_mask", "validate_patches" };
    auto                  members = fields(tokens[index++], names, false);
    if (! members) return std::unexpected(members.error());
    result.members = std::move(*members);
    if (index < tokens.size()) {
        if (! punct(tokens[index], ",")) return fail(tokens[index], "expected ',' after fields");
        if (++index < tokens.size()) {
            auto extras = fields(tokens[index++], names, true);
            if (! extras) return std::unexpected(extras.error());
            result.extras = std::move(*extras);
            if (index < tokens.size() && punct(tokens[index], ",")) ++index;
        }
    }
    if (index != tokens.size()) return fail(tokens[index], "unexpected entity_input argument");
    return result;
}

auto mapped_fields(const Entity& entity, const std::string& type) -> std::string {
    std::string result = "::qcm::backend::sync_schema::Fields<" + type;
    for (const auto& field : entity.members) {
        result += ",::qcm::backend::sync_schema::Field<&" + type + "::" + field.name + ",&" +
                  entity.model + "::" + field.name + ",\"" + field.name + '"';
        if (! field.converter.empty()) result += ',' + field.converter;
        result += '>';
    }
    return result + '>';
}
auto record_attributes(const std::string& type) -> std::string {
    return "[[pmacro::derive(\"serde::derive\"),pmacro::helper(\"serde::derive\",decode_only,"
           "validate(" +
           type + "::validate_patches))]]";
}
auto validator(const Entity& entity, const std::string& type, bool extras) -> std::string {
    std::string source = "inline auto " + type + "::validate_patches(const " + type +
                         "& value)->::rstd::Result<::rstd::empty,::rstd::ref<::rstd::str>>{"
                         "return ::qcm::backend::sync_schema::validate_patches(";
    bool        first  = true;
    auto        append = [&](const Field& field) {
        if (! first) source += ',';
        first = false;
        source += "value." + field.name;
    };
    for (const auto& field : entity.members) append(field);
    if (extras)
        for (const auto& field : entity.extras) append(field);
    return source + ");}";
}
auto generate(const Entity& entity) -> std::string {
    constexpr auto defaulted       = "[[pmacro::helper(\"serde::derive\",default)]]";
    constexpr auto validate_return = "->::rstd::Result<::rstd::empty,::rstd::ref<::rstd::str>>;";
    std::string    source = "struct " + record_attributes(entity.fields) + entity.fields + "{";
    for (const auto& field : entity.members)
        source += std::string(defaulted) + "::qcm::backend::Patch<" + field.type + "> " +
                  field.name + ';';
    source += "static auto validate_patches(const " + entity.fields + "&)" + validate_return +
              "};struct " + record_attributes(entity.input) + entity.input +
              "{[[pmacro::helper(\"serde::derive\",alias(\"id\"))]]"
              "::qcm::backend::ItemRef item;" +
              defaulted + "::rstd::Option<" + entity.fields + "> on_insert;";
    for (const auto& field : entity.members)
        source += std::string(defaulted) + "::qcm::backend::Patch<" + field.type + "> " +
                  field.name + ';';
    for (const auto& field : entity.extras)
        source += std::string(defaulted) + field.type + ' ' + field.name + "{};";
    source += "static auto validate_patches(const " + entity.input + "&)" + validate_return +
              "static auto validate_mask(const ::qcm::backend::Mask& mask)"
              " -> ::qcm::backend::DbWriteResult<::rstd::empty>;auto apply(" +
              entity.model +
              "& model,bool inserted,const ::qcm::backend::Mask& mask) const"
              " -> ::qcm::backend::DbWriteResult<::rstd::empty>;};";
    source += "constexpr auto qcm_sync_definition(" + entity.fields + "*){return " +
              mapped_fields(entity, entity.fields) + "{};}";
    source += "constexpr auto qcm_sync_definition(" + entity.input +
              "*){return ::qcm::backend::sync_schema::Entity<" + entity.input + ',' +
              entity.fields + ',' + mapped_fields(entity, entity.input);
    for (const auto& field : entity.extras)
        source += ",::qcm::backend::sync_schema::Extra<&" + entity.input + "::" + field.name +
                  ",\"" + field.name + "\">";
    source +=
        ">{};}" + validator(entity, entity.fields, false) + validator(entity, entity.input, true);
    source += "inline auto " + entity.input +
              "::validate_mask(const ::qcm::backend::Mask& mask)"
              " -> ::qcm::backend::DbWriteResult<::rstd::empty>{return mask.validate<" +
              entity.input + ">();}inline auto " + entity.input + "::apply(" + entity.model +
              "& model,bool inserted,const ::qcm::backend::Mask& mask) const"
              " -> ::qcm::backend::DbWriteResult<::rstd::empty>{return "
              "::qcm::backend::sync_schema::Definition<" +
              entity.input + ">::apply(model,*this,mask,inserted);}";
    return source;
}
} // namespace

[[pmacro::define("qcm")]]
auto entity_input(pmacro::FunctionInput input, pmacro::Context& context) noexcept -> Output {
    if (input.scope != pmacro::DeclarationScope::Namespace) {
        context.diagnostic(pmacro::DiagnosticLevel::Error,
                           input.invocation_span,
                           "qcm::entity_input requires namespace scope");
        return std::unexpected(pmacro::Error::Failure);
    }
    auto entity = parse(input);
    if (! entity) {
        context.diagnostic(pmacro::DiagnosticLevel::Error,
                           entity.error().span,
                           "qcm::entity_input: " + entity.error().message);
        return std::unexpected(pmacro::Error::Failure);
    }
    auto source = generate(*entity);
    if (input.after_item_export == pmacro::AfterItemExport::Required)
        source = "export {" + source + '}';
    return context.parse(source, input.invocation_span);
}
