#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>

namespace octac::lexer {

    /**
     * @brief The class of a token.
     */
    enum class TokenType {
        For,       ///< for
        Do,        ///< do
        Until,     ///< until
        Continue,  ///< continue
        Function,  ///< function
        Return,    ///< return
        If,        ///< if
        Else,      ///< else
        ElseIf,    ///< elseif
        CaseOf,    ///< caseof
        Case,      ///< case
        Default,   ///< default
        And,       ///< and
        Or,        ///< or
        Not,       ///< not
        Is,        ///< is
        In,        ///< in
        Break,     ///< break

        Scalar,  ///< i64 i32 i16 f64 f32 f16 bool
        Vector,  ///< vector
        Matrix,  ///< matrix
        String,  ///< string

        LBrace,     ///< {
        RBrace,     ///< }
        LParen,     ///< (
        RParen,     ///< )
        LBracket,   ///< [
        RBracket,   ///< ]
        Semicolon,  ///< ;
        Arrow,      ///< ->
        Comma,      ///< ,
        Ellipsis,   ///< ...
        Colon,      ///< :

        PlusMinus,     ///< + -
        StarSlashMod,  ///< * / %
        Transpose,     ///< '
        DotStar,       ///< .*
        DotSlash,      ///< ./
        Less,          ///< <
        Greater,       ///< >
        LessEq,        ///< <=
        GreaterEq,     ///< >=
        Assignment,    ///< = += -= *= /=

        Identifier,  ///< [a-zA-Z][a-zA-Z0-9_]*
        BoolLit,     ///< true false
        Int,         ///< [0-9]+
        Float,       ///< [0-9]+\.[0-9]+ or [0-9]+(\.[0-9]+)?[eE][+-]?[0-9]+
        StringLit,   ///< "[^"\n]*"

        CompilerEof,   ///< the end of the input
        CompilerError  ///< an unrecognized lexeme. The token value holds the offending text.
    };

    /**
     * @brief A lexical token.
     *
     * The type is the class part and the value is the value part. Only some types carry a value,
     * see hasValuePart().
     */
    struct Token {
        TokenType type;      ///< the class of the token
        std::string value;   ///< the lexeme as written in the source
        std::size_t line;    ///< 1-based line of the first character
        std::size_t column;  ///< 1-based column of the first character
    };

    /**
     * @brief Returns the upper case name of a token type, for example "STAR_SLASH_MOD".
     *
     * @param type The token type.
     * @return A static string naming @p type.
     */
    const char *tokenTypeName(TokenType type);

    /**
     * @brief Describes a token type for use in a message, for example "';'" or "an identifier".
     *
     * @param type The token type.
     * @return A static string such as "'for'", "'+' or '-'" or "an integer literal".
     */
    const char *tokenTypeDescription(TokenType type);

    /**
     * @brief Resolves a word to a keyword, scalar type, bool literal or identifier.
     *
     * @param lexeme A word matching [a-zA-Z][a-zA-Z0-9_]*.
     * @return The keyword type of @p lexeme, or TokenType::Identifier when it is not reserved.
     */
    TokenType lookupKeyword(const std::string &lexeme);

    /**
     * @brief Resolves an operator or punctuator lexeme to its token type.
     *
     * @param lexeme The operator text, for example "<=".
     * @return The type of @p lexeme, or TokenType::CompilerError when it is not an operator.
     */
    TokenType lookupOperator(const std::string &lexeme);

    /**
     * @brief Tells whether tokens of a type carry a value part.
     *
     * @param type The token type.
     * @return true for identifiers, literals, scalar types, and operators that share a type.
     */
    bool hasValuePart(TokenType type);

    /**
     * @brief Writes a token as `<TYPE, value>`, or `<TYPE,>` when the type has no value part.
     */
    std::ostream &operator<<(std::ostream &out, const Token &token);

}  // namespace octac::lexer
