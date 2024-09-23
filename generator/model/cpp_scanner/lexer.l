%{
#include "cpp_scanner/scanner.h"
%}

%option c++
%option noyywrap

ws                              [ \f\v\t]

digit                           [0-9]
hex                             [0-9A-Fa-f]
letter                          [A-Z_a-z]
simple_escape_sequence          (\\\'|\\\"|\\\?|\\\\|\\a|\\b|\\f|\\n|\\r|\\t|\\v)
octal_escape_sequence           (\\[0-7]|\\[0-7][0-7]|\\[0-7][0-7][0-7])
hexadecimal_escape_sequence     (\\x{hex}+)
escape_sequence                 ({simple_escape_sequence}|{octal_escape_sequence}|{hexadecimal_escape_sequence})
universal_character_name        (\\u{hex}{hex}{hex}{hex}|\\U{hex}{hex}{hex}{hex}{hex}{hex}{hex}{hex})
non_digit                       ({letter}|{universal_character_name})
identifier                      ({non_digit}({non_digit}|{digit})*)

character_lit                   (L?\'([^\'\\\n]|\\.)*)
character_literal               ({character_lit}\')

string_lit                      (L?\"([^\"\\\n]|\\.)*)
string_literal                  ({string_lit}\")

pp_number                       (\.?{digit}({digit}|{non_digit}|[eE][-+]|\.)*)


%%
^.*\n                           { LEX_SAVE_LINE(yytext, yyleng); REJECT; }
^{ws}*"#".*                     { /* Throw away preprocessor lines - hopefully only #line and equivalent. */ }

{character_lit}\'               { LEX_CHARACTER_TOKEN(yytext, yyleng-1); };
{character_lit}\\               { ERRMSG("End of line assumed to terminate character with trailing escape.");
                                  LEX_CHARACTER_TOKEN(yytext, yyleng-1); };
{character_lit}                 { ERRMSG("End of line assumed to terminate character.");
                                  LEX_CHARACTER_TOKEN(yytext, yyleng); };

{string_lit}\"                  { LEX_STRING_TOKEN(yytext, yyleng-1); };
{string_lit}\\                  { ERRMSG("End of line assumed to terminate string with trailing escape.");
                                  LEX_STRING_TOKEN(yytext, yyleng-1); };
{string_lit}                    { ERRMSG("End of line assumed to terminate string.");
                                  LEX_STRING_TOKEN(yytext, yyleng); };

"asm"                           { LEX_STATIC_TOKEN(ASM); }
"auto"                          { LEX_STATIC_TOKEN(AUTO); }
"bool"                          { LEX_C_STATIC_TOKEN(BOOL); }
"break"                         { LEX_STATIC_TOKEN(BREAK); }
"case"                          { LEX_STATIC_TOKEN(CASE); }
"catch"                         { LEX_C_STATIC_TOKEN(CATCH); }
"char"                          { LEX_STATIC_TOKEN(CHAR); }
"class"                         { LEX_C_STATIC_TOKEN(CLASS); }
"const"                         { LEX_STATIC_TOKEN(CONST); }
"const_cast"                    { LEX_C_STATIC_TOKEN(CONST_CAST); }
"continue"                      { LEX_STATIC_TOKEN(CONTINUE); }
"default"                       { LEX_STATIC_TOKEN(DEFAULT); }
"delete"                        { LEX_C_STATIC_TOKEN(DELETE); }
"do"                            { LEX_STATIC_TOKEN(DO); }
"double"                        { LEX_STATIC_TOKEN(DOUBLE); }
"dynamic_cast"                  { LEX_C_STATIC_TOKEN(DYNAMIC_CAST); }
"else"                          { LEX_STATIC_TOKEN(ELSE); }
"enum"                          { LEX_STATIC_TOKEN(ENUM); }
"explicit"                      { LEX_C_STATIC_TOKEN(EXPLICIT); }
"export"                        { LEX_C_STATIC_TOKEN(EXPORT); }
"extern"                        { LEX_STATIC_TOKEN(EXTERN); }
"false"                         { LEX_C_STATIC_TOKEN(FALSE); }
"float"                         { LEX_STATIC_TOKEN(FLOAT); }
"for"                           { LEX_STATIC_TOKEN(FOR); }
"friend"                        { LEX_STATIC_TOKEN(FRIEND); }
"goto"                          { LEX_STATIC_TOKEN(GOTO); }
"if"                            { LEX_STATIC_TOKEN(IF); }
"inline"                        { LEX_C_STATIC_TOKEN(INLINE); }
"int"                           { LEX_STATIC_TOKEN(INT); }
"long"                          { LEX_STATIC_TOKEN(LONG); }
"mutable"                       { LEX_C_STATIC_TOKEN(MUTABLE); }
"namespace"                     { LEX_C_STATIC_TOKEN(NAMESPACE); }
"new"                           { LEX_C_STATIC_TOKEN(NEW); }
"operator"                      { LEX_C_STATIC_TOKEN(OPERATOR); }
"private"                       { LEX_C_STATIC_TOKEN(PRIVATE); }
"protected"                     { LEX_C_STATIC_TOKEN(PROTECTED); }
"public"                        { LEX_C_STATIC_TOKEN(PUBLIC); }
"register"                      { LEX_STATIC_TOKEN(REGISTER); }
"reinterpret_cast"              { LEX_C_STATIC_TOKEN(REINTERPRET_CAST); }
"return"                        { LEX_STATIC_TOKEN(RETURN); }
"short"                         { LEX_STATIC_TOKEN(SHORT); }
"signed"                        { LEX_C_STATIC_TOKEN(SIGNED); }
"sizeof"                        { LEX_STATIC_TOKEN(SIZEOF); }
"static"                        { LEX_STATIC_TOKEN(STATIC); }
"static_cast"                   { LEX_C_STATIC_TOKEN(STATIC_CAST); }
"struct"                        { LEX_STATIC_TOKEN(STRUCT); }
"switch"                        { LEX_STATIC_TOKEN(SWITCH); }
"template"                      { LEX_C_STATIC_TOKEN(TEMPLATE); }
"this"                          { LEX_C_STATIC_TOKEN(THIS); }
"throw"                         { LEX_C_STATIC_TOKEN(THROW); }
"true"                          { LEX_C_STATIC_TOKEN(TRUE); }
"try"                           { LEX_C_STATIC_TOKEN(TRY); }
"typedef"                       { LEX_STATIC_TOKEN(TYPEDEF); }
"typeid"                        { LEX_C_STATIC_TOKEN(TYPEID); }
"typename"                      { LEX_C_STATIC_TOKEN(TYPENAME); }
"union"                         { LEX_STATIC_TOKEN(UNION); }
"unsigned"                      { LEX_STATIC_TOKEN(UNSIGNED); }
"using"                         { LEX_C_STATIC_TOKEN(USING); }
"virtual"                       { LEX_STATIC_TOKEN(VIRTUAL); }
"void"                          { LEX_STATIC_TOKEN(VOID); }
"volatile"                      { LEX_STATIC_TOKEN(VOLATILE); }
"wchar_t"                       { LEX_C_STATIC_TOKEN(WCHAR_T); }
"while"                         { LEX_STATIC_TOKEN(WHILE); }

"::"                            { LEX_C_STATIC_TOKEN(SCOPE); }
"..."                           { LEX_STATIC_TOKEN(ELLIPSIS); }
"<<"                            { LEX_STATIC_TOKEN(SHL); }
">>"                            { LEX_STATIC_TOKEN(SHR); }
"=="                            { LEX_STATIC_TOKEN(EQ); }
"!="                            { LEX_STATIC_TOKEN(NE); }
"<="                            { LEX_STATIC_TOKEN(LE); }
">="                            { LEX_STATIC_TOKEN(GE); }
"&&"                            { LEX_STATIC_TOKEN(LOG_AND); }
"||"                            { LEX_STATIC_TOKEN(LOG_OR); }
"++"                            { LEX_STATIC_TOKEN(INC); }
"--"                            { LEX_STATIC_TOKEN(DEC); }
"->*"                           { LEX_STATIC_TOKEN(ARROW_STAR); }
"->"                            { LEX_STATIC_TOKEN(ARROW); }
".*"                            { LEX_STATIC_TOKEN(DOT_STAR); }
"+="                            { LEX_STATIC_TOKEN(ASS_ADD); }
"-="                            { LEX_STATIC_TOKEN(ASS_SUB); }
"*="                            { LEX_STATIC_TOKEN(ASS_MUL); }
"/="                            { LEX_STATIC_TOKEN(ASS_DIV); }
"%="                            { LEX_STATIC_TOKEN(ASS_MOD); }
"^="                            { LEX_STATIC_TOKEN(ASS_XOR); }
"&="                            { LEX_STATIC_TOKEN(ASS_AND); }
"|="                            { LEX_STATIC_TOKEN(ASS_OR); }
">>="                           { LEX_STATIC_TOKEN(ASS_SHR); }
"<<="                           { LEX_STATIC_TOKEN(ASS_SHL); }

{pp_number}                     { LEX_NUMBER_TOKEN(yytext, yyleng); }

{identifier}                    { LEX_IDENTIFIER_TOKEN(yytext, yyleng); }

{escape_sequence}               |
{universal_character_name}      { LEX_ESCAPED_TOKEN(yytext, yyleng); }

\n                              |
{ws}+                           { /* Throw away whitespace */ }
.                               { LEX_ASCII_TOKEN(yytext[0]); }
%%