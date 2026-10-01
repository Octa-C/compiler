// token.cpp
#include "token.hpp"
#include <unordered_map>

static const char *Names[] = {
    "FOR",
    "DO",
    "UNTIL",
    "CONTINUE",
    "FUNCTION",
    "RETURN",
    "IF",
    "ELSE",
    "ELSEIF",
    "CASEOF",
    "CASE",
    "DEFAULT",
    "AND",
    "OR",
    "NOT",
    "IS",
    "IN",
    "BREAK",
    "SCALAR",
    "VECTOR",
    "MATRIX",
    "STRING",
    "LBRACE",
    "RBRACE",
    "LPAREN",
    "RPAREN",
    "LBRACKET",
    "RBRACKET",
    "SEMICOLON",
    "ARROW",
    "COMMA",
    "ELLIPSIS",
    "COLON",
    "PLUSMINUS",
    "STAR_SLASH_MOD",
    "TRANSPOSE",
    "DOT_STAR",
    "DOT_SLASH",
    "LESS",
    "GREATER",
    "LESS_EQ",
    "GREATER_EQ",
    "ASSIGNMENT",
    "IDENTIFIER",
    "BOOL",
    "INT",
    "FLOAT",
    "STRING_LIT",
    "COMPILER_EOF",
    "COMPILER_ERROR",
};


static_assert(sizeof(Names) / sizeof(Names[0]) == NUM_TOKEN_TYPES,
              "Names[] is out of sync with enum TokenType");
              
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"for", FOR},
    {"do", DO},
    {"until", UNTIL},
    {"continue", CONTINUE},
    {"function", FUNCTION},
    {"return", RETURN},
    {"if", IF},
    {"else", ELSE},
    {"elseif", ELSEIF},
    {"caseof", CASEOF},
    {"case", CASE},
    {"default", DEFAULT},
    {"and", AND},
    {"or", OR},
    {"not", NOT},
    {"is", IS},
    {"in", IN},
    {"break", BREAK},

    {"i64", SCALAR},
    {"i32", SCALAR},
    {"i16", SCALAR},
    {"f64", SCALAR},
    {"f32", SCALAR},
    {"f16", SCALAR},
    {"bool", SCALAR},

    {"vector", VECTOR},
    {"matrix", MATRIX},
    {"string", STRING},
};

static const std::unordered_map<std::string, TokenType> OPERATORS = {
    {"{", LBRACE},
    {"}", RBRACE},
    {"(", LPAREN},
    {")", RPAREN},
    {"[", LBRACKET},
    {"]", RBRACKET},
    {";", SEMICOLON},
    {"->", ARROW},
    {",", COMMA},
    {"...", ELLIPSIS},
    {":", COLON},

    {"+", PLUSMINUS},
    {"-", PLUSMINUS},
    {"*", STAR_SLASH_MOD},
    {"/", STAR_SLASH_MOD},
    {"%", STAR_SLASH_MOD},

    {"'", TRANSPOSE},
    {".*", DOT_STAR},
    {"./", DOT_SLASH},

    {"<", LESS},
    {">", GREATER},
    {"<=", LESS_EQ},
    {">=", GREATER_EQ},

    {"=",   ASSIGNMENT},
    {"+=",  ASSIGNMENT},
    {"-=",  ASSIGNMENT},
    {"*=",  ASSIGNMENT},
    {"/=",  ASSIGNMENT},

};

const char *tokenTypeName(TokenType type)
{
    return Names[type];
}

TokenType lookupKeyword(const std::string &lexeme)
{
    auto it = KEYWORDS.find(lexeme);
    return it == KEYWORDS.end() ? IDENTIFIER : it->second;
}

TokenType lookupOperator(const std::string &lexeme)
{
    auto it = OPERATORS.find(lexeme);
    return it == OPERATORS.end() ? COMPILER_ERROR : it->second;
}

bool hasValuePart(TokenType type)
{
    switch (type) {
        case IDENTIFIER:
        case INT:
        case FLOAT:
        case STRING_LIT:
        case SCALAR:
        case ASSIGNMENT:
        case PLUSMINUS:
        case STAR_SLASH_MOD:
            return true;
        default:
            return false;
    }
}
