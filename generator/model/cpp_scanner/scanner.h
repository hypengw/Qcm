
#include <iostream>
#include <string>

#ifndef YY_FLEX_MAJOR_VERSION
#    include <FlexLexer.h>
#endif

#include "cpp_scanner/token.h"

class MyLexer : public yyFlexLexer {
public:
    MyLexer(std::istream* in = nullptr): yyFlexLexer(in) {}

    void printToken(const std::string& type, const std::string& value) {
        std::cout << "Token: " << type << " Value: " << value << std::endl;
    }
};
