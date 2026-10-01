// scanner.hpp

#pragma once

#include <string>
#include "token.hpp"

class Scanner
{
public:
    explicit Scanner(std::string source);
    Token next();

private:
    std::string src;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    void advance(size_t n);
};
