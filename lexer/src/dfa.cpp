// dfa.cpp
#include "dfa.hpp"

CharClass charClassOf(unsigned char c)
{
    static CharClass table[256];
    static bool built = false;

    if (!built)
    {
        for (int i = 0; i < 256; ++i)
        {
            table[i] = CC_OTHER;
        }
        for (int i = 'a'; i <= 'z'; ++i)
        {
            table[i] = CC_LETTER;
        }
        for (int i = 'A'; i <= 'Z'; ++i)
        {
            table[i] = CC_LETTER;
        }
        table['e'] = CC_EXP;
        table['E'] = CC_EXP;

        for (int i = '0'; i <= '9'; ++i)
        {
            table[i] = CC_DIGIT;
        }
        table['_'] = CC_UNDERSCORE;
        table[' '] = CC_WS;
        table['\t'] = CC_WS;
        table['\r'] = CC_WS;
        table['\f'] = CC_WS;
        table['\v'] = CC_WS;
        table['\n'] = CC_NEWLINE;
        table['"'] = CC_QUOTE;
        table['.'] = CC_DOT;
        table['-'] = CC_MINUS;
        table['<'] = CC_LT;
        table['>'] = CC_GT;
        table['='] = CC_EQ;
        table['+'] = CC_PLUS;
        table['-'] = CC_MINUS;
        table['*'] = CC_STAR;
        table['/'] = CC_SLASH;
        for (const char *p = "{}()[];,:%'"; *p; p++)
            table[(unsigned char)*p] = CC_SIMPLE;

        built = true;
    }
    return table[c];
}

const State TRANSITION[NUM_STATES][NUM_CLASSES] = {
    //                 CC_LETTER CC_EXP CC_DIGIT CC_UNDERSCORE CC_WS CC_NEWLINE CC_QUOTE CC_DOT CC_PLUS CC_MINUS CC_STAR CC_SLASH CC_LT CC_GT CC_EQ CC_SIMPLE CC_OTHER
    /* S_DEAD      */ {},
    /* S_START     */ {S_IDENT, S_IDENT, S_INT, S_DEAD, S_WS, S_WS, S_STR_BODY, S_DOT, S_OP_EQ, S_MINUS, S_OP_EQ, S_SLASH, S_OP_EQ, S_OP_EQ, S_OP_DONE, S_OP_DONE, S_DEAD},
    /* S_IDENT     */ {S_IDENT, S_IDENT, S_IDENT, S_IDENT, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_INT       */ {S_DEAD, S_EXP, S_INT, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_INT_DOT, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_INT_DOT   */ {S_DEAD, S_DEAD, S_FLOAT, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_FLOAT     */ {S_DEAD, S_EXP, S_FLOAT, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_EXP       */ {S_DEAD, S_DEAD, S_FLOAT_EXP, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_EXP_SIGN, S_EXP_SIGN, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_EXP_SIGN  */ {S_DEAD, S_DEAD, S_FLOAT_EXP, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_FLOAT_EXP */ {S_DEAD, S_DEAD, S_FLOAT_EXP, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_OP_DONE   */ {},
    /* S_MINUS     */ {S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_OP_DONE, S_OP_DONE, S_DEAD, S_DEAD},
    /* S_OP_EQ     */ {S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_OP_DONE, S_DEAD, S_DEAD},
    /* S_SLASH     */ {S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_COMMENT, S_DEAD, S_DEAD, S_OP_DONE, S_DEAD, S_DEAD},
    /* S_COMMENT   */ {S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_DEAD, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT, S_COMMENT},
    /* S_DOT       */ {S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DOT_DOT, S_DEAD, S_DEAD, S_OP_DONE, S_OP_DONE, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_DOT_DOT   */ {S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_OP_DONE, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
    /* S_STR_BODY  */ {S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_DEAD, S_STR_END, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY, S_STR_BODY},
    /* S_STR_END   */ {},
    /* S_WS        */ {S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_WS, S_WS, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD, S_DEAD},
};

const Action ACTION[NUM_STATES] = {
    A_NONE,   // S_DEAD
    A_NONE,   // S_START
    A_WORD,   // S_IDENT      keyword table, else IDENTIFIER
    A_INT,    // S_INT
    A_NONE,   // S_INT_DOT    "12." is not a number yet
    A_FLOAT,  // S_FLOAT
    A_NONE,   // S_EXP
    A_NONE,   // S_EXP_SIGN
    A_FLOAT,  // S_FLOAT_EXP
    A_OP,     // S_OP_DONE    operator table
    A_OP,     // S_MINUS      "-" is complete on its own
    A_OP,     // S_OP_EQ        "<" / ">" are complete on their own
    A_OP,     // S_SLASH
    A_SKIP,   // S_COMMENT
    A_NONE,   // S_DOT        "." alone is nothing
    A_NONE,   // S_DOT_DOT    ".." alone is nothing
    A_NONE,   // S_STR_BODY   no closing quote yet
    A_STRING, // S_STR_END
    A_SKIP,   // S_WS         accepting, but no token
};

const char *STATE_NAMES[] = {
    "DEAD",
    "START",
    "IDENT",
    "INT",
    "INT_DOT",
    "FLOAT",
    "EXP",
    "EXP_SIGN",
    "FLOAT_EXP",
    "OP_DONE",
    "MINUS",
    "OP_EQ",
    "SLASH",
    "COMMENT",
    "DOT",
    "DOT_DOT",
    "STR_BODY",
    "STR_END",
    "WS",
};

static_assert(sizeof(STATE_NAMES) / sizeof(STATE_NAMES[0]) == NUM_STATES,
              "STATE_NAMES is out of sync with enum State");

const char *CLASS_NAMES[] = {
    "LETTER",
    "EXP",
    "DIGIT",
    "_",
    "WS",
    "NL",
    "QUOTE",
    "DOT",
    "PLUS",
    "MINUS",
    "STAR",
    "SLASH",
    "LT",
    "GT",
    "EQ",
    "SIMPLE",
    "OTHER",
};

static_assert(sizeof(CLASS_NAMES) / sizeof(CLASS_NAMES[0]) == NUM_CLASSES,
              "CLASS_NAMES is out of sync with enum CharClass");
