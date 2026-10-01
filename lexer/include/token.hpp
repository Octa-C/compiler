// token.hpp
#include <string>
#pragma once
enum TokenType
{
    FOR,                                       // for
    DO,                                        // dp
    UNTIL,                                     // until
    CONTINUE,                                  // continue
    FUNCTION,                                   // function
    RETURN,                                     // return
    IF,                                  // if
    ELSE,                                // else
    ELSEIF,                              // elseif
    CASEOF,                              // caseof
    CASE,                                // case
    DEFAULT,                             // defualt
    AND,                      // and
    OR,                       // or
    NOT,                      // not
    IS,                       // is
    IN,                       // in
    BREAK,                                      // break
    SCALAR,                                          // i64 i32 i16 f64 f32 f16 bool
    VECTOR,                                          // vector
    MATRIX,                                          // matrix
    STRING,                                          // string
    LBRACE,                                 // {
    RBRACE,                                 // }
    LPAREN,                                 // (
    RPAREN,                                 //)
    LBRACKET,                               // [
    RBRACKET,                               // ]
    SEMICOLON,                              //;
    ARROW,                                  // ->
    COMMA,                                  // ,
    ELLIPSIS,                               // ...
    COLON,                                  // :
    PLUSMINUS,                       // + -
    STAR_SLASH_MOD,                  // * / %
    TRANSPOSE,                         // '
    DOT_STAR,                          // .*
    DOT_SLASH,                         // ./
    LESS,       // <
    GREATER,    // >
    LESS_EQ,    // <=
    GREATER_EQ, // >=
    ASSIGNMENT,                               //=
    IDENTIFIER,                                         //[a - zA - Z][a - zA - Z0 - 9_] *
    BOOL,                            // 0 or 1 not produced by lexer, but by parser
    INT,                              //[0 - 9] +
    FLOAT,                            //[0 - 9] + . [0 - 9] +
    STRING_LIT,                           //" [^"\n]* "   (no escapes, one line)
    COMPILER_EOF,                                       // End of file
    COMPILER_ERROR,                                     // unrecognised lexeme; value part is
                                                        // the bad text, for the error report
    NUM_TOKEN_TYPES   // sentinel = 50, number of token types;
    };

struct Token
{
    TokenType type; //class part
    std::string value;// value part
    int line; // 1-based line number of the token in the source file
    int column;
};

const char *tokenTypeName(TokenType type);
TokenType lookupKeyword(const std::string &lexeme);
TokenType lookupOperator(const std::string &lexeme);
bool hasValuePart(TokenType type);
