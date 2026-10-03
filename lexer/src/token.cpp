#include "token.hpp"

#include <ostream>
#include <unordered_map>

namespace octac::lexer {

    namespace {

        const std::unordered_map<std::string, TokenType> kKeywords = {
            {"for", TokenType::For},
            {"do", TokenType::Do},
            {"until", TokenType::Until},
            {"continue", TokenType::Continue},
            {"function", TokenType::Function},
            {"return", TokenType::Return},
            {"if", TokenType::If},
            {"else", TokenType::Else},
            {"elseif", TokenType::ElseIf},
            {"caseof", TokenType::CaseOf},
            {"case", TokenType::Case},
            {"default", TokenType::Default},
            {"and", TokenType::And},
            {"or", TokenType::Or},
            {"not", TokenType::Not},
            {"is", TokenType::Is},
            {"in", TokenType::In},
            {"break", TokenType::Break},

            {"i64", TokenType::Scalar},
            {"i32", TokenType::Scalar},
            {"i16", TokenType::Scalar},
            {"f64", TokenType::Scalar},
            {"f32", TokenType::Scalar},
            {"f16", TokenType::Scalar},
            {"bool", TokenType::Scalar},

            {"vector", TokenType::Vector},
            {"matrix", TokenType::Matrix},
            {"string", TokenType::String},

            {"true", TokenType::BoolLit},
            {"false", TokenType::BoolLit},
        };

        const std::unordered_map<std::string, TokenType> kOperators = {
            {"{", TokenType::LBrace},       {"}", TokenType::RBrace},
            {"(", TokenType::LParen},       {")", TokenType::RParen},
            {"[", TokenType::LBracket},     {"]", TokenType::RBracket},
            {";", TokenType::Semicolon},    {"->", TokenType::Arrow},
            {",", TokenType::Comma},        {"...", TokenType::Ellipsis},
            {":", TokenType::Colon},

            {"+", TokenType::PlusMinus},    {"-", TokenType::PlusMinus},
            {"*", TokenType::StarSlashMod}, {"/", TokenType::StarSlashMod},
            {"%", TokenType::StarSlashMod},

            {"'", TokenType::Transpose},    {".*", TokenType::DotStar},
            {"./", TokenType::DotSlash},

            {"<", TokenType::Less},         {">", TokenType::Greater},
            {"<=", TokenType::LessEq},      {">=", TokenType::GreaterEq},

            {"=", TokenType::Assignment},   {"+=", TokenType::Assignment},
            {"-=", TokenType::Assignment},  {"*=", TokenType::Assignment},
            {"/=", TokenType::Assignment},
        };

    }  // namespace

    const char *tokenTypeName(TokenType type) {
        switch (type) {
        case TokenType::For:           return "FOR";
        case TokenType::Do:            return "DO";
        case TokenType::Until:         return "UNTIL";
        case TokenType::Continue:      return "CONTINUE";
        case TokenType::Function:      return "FUNCTION";
        case TokenType::Return:        return "RETURN";
        case TokenType::If:            return "IF";
        case TokenType::Else:          return "ELSE";
        case TokenType::ElseIf:        return "ELSEIF";
        case TokenType::CaseOf:        return "CASEOF";
        case TokenType::Case:          return "CASE";
        case TokenType::Default:       return "DEFAULT";
        case TokenType::And:           return "AND";
        case TokenType::Or:            return "OR";
        case TokenType::Not:           return "NOT";
        case TokenType::Is:            return "IS";
        case TokenType::In:            return "IN";
        case TokenType::Break:         return "BREAK";
        case TokenType::Scalar:        return "SCALAR";
        case TokenType::Vector:        return "VECTOR";
        case TokenType::Matrix:        return "MATRIX";
        case TokenType::String:        return "STRING";
        case TokenType::LBrace:        return "LBRACE";
        case TokenType::RBrace:        return "RBRACE";
        case TokenType::LParen:        return "LPAREN";
        case TokenType::RParen:        return "RPAREN";
        case TokenType::LBracket:      return "LBRACKET";
        case TokenType::RBracket:      return "RBRACKET";
        case TokenType::Semicolon:     return "SEMICOLON";
        case TokenType::Arrow:         return "ARROW";
        case TokenType::Comma:         return "COMMA";
        case TokenType::Ellipsis:      return "ELLIPSIS";
        case TokenType::Colon:         return "COLON";
        case TokenType::PlusMinus:     return "PLUSMINUS";
        case TokenType::StarSlashMod:  return "STAR_SLASH_MOD";
        case TokenType::Transpose:     return "TRANSPOSE";
        case TokenType::DotStar:       return "DOT_STAR";
        case TokenType::DotSlash:      return "DOT_SLASH";
        case TokenType::Less:          return "LESS";
        case TokenType::Greater:       return "GREATER";
        case TokenType::LessEq:        return "LESS_EQ";
        case TokenType::GreaterEq:     return "GREATER_EQ";
        case TokenType::Assignment:    return "ASSIGNMENT";
        case TokenType::Identifier:    return "IDENTIFIER";
        case TokenType::BoolLit:       return "BOOL_LIT";
        case TokenType::Int:           return "INT";
        case TokenType::Float:         return "FLOAT";
        case TokenType::StringLit:     return "STRING_LIT";
        case TokenType::CompilerEof:   return "COMPILER_EOF";
        case TokenType::CompilerError: return "COMPILER_ERROR";
        }
        return "UNKNOWN";
    }

    const char *tokenTypeDescription(TokenType type) {
        switch (type) {
        case TokenType::For:           return "'for'";
        case TokenType::Do:            return "'do'";
        case TokenType::Until:         return "'until'";
        case TokenType::Continue:      return "'continue'";
        case TokenType::Function:      return "'function'";
        case TokenType::Return:        return "'return'";
        case TokenType::If:            return "'if'";
        case TokenType::Else:          return "'else'";
        case TokenType::ElseIf:        return "'elseif'";
        case TokenType::CaseOf:        return "'caseof'";
        case TokenType::Case:          return "'case'";
        case TokenType::Default:       return "'default'";
        case TokenType::And:           return "'and'";
        case TokenType::Or:            return "'or'";
        case TokenType::Not:           return "'not'";
        case TokenType::Is:            return "'is'";
        case TokenType::In:            return "'in'";
        case TokenType::Break:         return "'break'";
        case TokenType::Scalar:        return "a scalar type";
        case TokenType::Vector:        return "'vector'";
        case TokenType::Matrix:        return "'matrix'";
        case TokenType::String:        return "'string'";
        case TokenType::LBrace:        return "'{'";
        case TokenType::RBrace:        return "'}'";
        case TokenType::LParen:        return "'('";
        case TokenType::RParen:        return "')'";
        case TokenType::LBracket:      return "'['";
        case TokenType::RBracket:      return "']'";
        case TokenType::Semicolon:     return "';'";
        case TokenType::Arrow:         return "'->'";
        case TokenType::Comma:         return "','";
        case TokenType::Ellipsis:      return "'...'";
        case TokenType::Colon:         return "':'";
        case TokenType::PlusMinus:     return "'+' or '-'";
        case TokenType::StarSlashMod:  return "'*', '/' or '%'";
        case TokenType::Transpose:     return "the transpose operator";
        case TokenType::DotStar:       return "'.*'";
        case TokenType::DotSlash:      return "'./'";
        case TokenType::Less:          return "'<'";
        case TokenType::Greater:       return "'>'";
        case TokenType::LessEq:        return "'<='";
        case TokenType::GreaterEq:     return "'>='";
        case TokenType::Assignment:    return "an assignment operator";
        case TokenType::Identifier:    return "an identifier";
        case TokenType::BoolLit:       return "a boolean literal";
        case TokenType::Int:           return "an integer literal";
        case TokenType::Float:         return "a float literal";
        case TokenType::StringLit:     return "a string literal";
        case TokenType::CompilerEof:   return "end of input";
        case TokenType::CompilerError: return "an invalid token";
        }
        return "an unknown token";
    }

    TokenType lookupKeyword(const std::string &lexeme) {
        const auto it = kKeywords.find(lexeme);
        return it == kKeywords.end() ? TokenType::Identifier : it->second;
    }

    TokenType lookupOperator(const std::string &lexeme) {
        const auto it = kOperators.find(lexeme);
        return it == kOperators.end() ? TokenType::CompilerError : it->second;
    }

    bool hasValuePart(TokenType type) {
        switch (type) {
        case TokenType::Identifier:
        case TokenType::BoolLit:
        case TokenType::Int:
        case TokenType::Float:
        case TokenType::StringLit:
        case TokenType::Scalar:
        case TokenType::Assignment:
        case TokenType::PlusMinus:
        case TokenType::StarSlashMod: return true;
        default:                      return false;
        }
    }

    std::ostream &operator<<(std::ostream &out, const Token &token) {
        out << '<' << tokenTypeName(token.type) << ',';
        if (hasValuePart(token.type)) {
            out << ' ' << token.value;
        }
        return out << '>';
    }

}  // namespace octac::lexer
